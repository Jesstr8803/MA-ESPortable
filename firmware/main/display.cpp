// display.cpp — SH8601 AMOLED (QSPI) + LVGL bring-up for MA-ESPortable.
// Adapted from Waveshare's ESP32-S3-AMOLED-1.91 ESP-IDF demo, retargeted to our
// board_pins.h + ui_init().
//
// HARD-WON BRING-UP NOTES (2026-06-06, validated on real hardware):
//  - QSPI pins from the Waveshare demo: PCLK=47, D0=18, D1=7, D2=48, D3=5,
//    CS=6, RST=17. (Deriving these from schematic net-names = wrong; use these.)
//  - MUST set CONFIG_LV_COLOR_16_SWAP=y (RGB565 byte swap) or the screen is
//    black. Confirmed by the factory demo's sdkconfig.
//  - Panel does NOT support hardware swap_xy (driver logs an error). LVGL's own
//    sw_rotate (ROT_90/270) produced a broken 240x240 doubled-widget artifact.
//    => We do PORTRAIT by ROTATING MANUALLY in flush_cb, with LVGL in
//       full_refresh mode (whole 240x536 frame each flush => no partial-area
//       stride/skew). This is the combination that finally rendered correctly.
//  - LVGL task on core 1 + CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU*=n (GUI
//    render starves IDLE; that's expected, don't let the WDT trip).
// TODO(perf): rotation buffer currently falls back to PSRAM (full frame won't
//    fit internal DMA RAM). Works, but for speed rotate in internal-RAM bands.
#include "display.h"
#include "board_pins.h"
#include "ui.h"
#include "ui_theme.h"   // UI_W / UI_H
#include <cstring>      // memcpy

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "lvgl.h"
#include "esp_lcd_sh8601.h"
#include "touch.h"

static const char *TAG = "display";

// Set to 1 to paint a solid-red LVGL screen instead of the UI (bring-up bisect).
// First light CONFIRMED 2026-06-06 (red showed) -> off, show the real UI.
#define DISPLAY_REDTEST 0

#define LCD_HOST            SPI2_HOST
#define LCD_BIT_PER_PIXEL   16              // RGB565 (CONFIG_LV_COLOR_DEPTH=16)
#define PANEL_W             536             // native landscape
#define PANEL_H             240
#define LVGL_BUF_LINES      (PANEL_H / 4)   // ~1/4 screen draw buffer
#define LVGL_TICK_MS        2

static esp_lcd_panel_handle_t s_panel = nullptr;
static SemaphoreHandle_t      s_lvgl_mux = nullptr;
static lv_disp_draw_buf_t     s_draw_buf;
static lv_disp_drv_t          s_disp_drv;

// Panel init sequence (from Waveshare demo; tuned for this exact SH8601 panel).
static const sh8601_lcd_init_cmd_t s_init_cmds[] = {
    {0x11, (uint8_t[]){0x00}, 0, 120},
    {0x36, (uint8_t[]){0xF0}, 1, 0},
    {0x3A, (uint8_t[]){0x55}, 1, 0},                            // 16-bit RGB565
    {0x2A, (uint8_t[]){0x00, 0x00, 0x02, 0x17}, 4, 0},
    {0x2B, (uint8_t[]){0x00, 0x00, 0x00, 0xEF}, 4, 0},
    {0x51, (uint8_t[]){0x00}, 1, 10},
    {0x29, (uint8_t[]){0x00}, 0, 10},
    {0x51, (uint8_t[]){0xFF}, 1, 0},                            // brightness max
};

// MANUAL portrait rotation in the flush (LVGL itself runs un-rotated at
// 240x536). LVGL gives us a portrait area + pixels; we rotate them 90 deg into
// a DMA scratch and draw to the mapped rectangle on the native 536x240 panel.
// This avoids LVGL sw_rotate (which produced a 240x240 doubled artifact) and
// the no-hw-swap_xy limitation.
static lv_color_t *s_rot = nullptr;   // full rotated frame (PSRAM ok)
static int         s_rot_px = 0;
static lv_color_t *s_band = nullptr;  // small internal-DMA band for output
static int         s_band_px = 0;
static SemaphoreHandle_t s_tx_done = nullptr;  // signals one band's DMA finished

// esp_lcd color-transfer-done callback: give the semaphore so flush_cb can
// safely reuse the band buffer for the next chunk.
static bool color_done_cb(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *) {
    BaseType_t hp = pdFALSE;
    xSemaphoreGiveFromISR(s_tx_done, &hp);
    return hp == pdTRUE;
}

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *px) {
    auto panel = (esp_lcd_panel_handle_t)drv->user_data;
    const int aw = area->x2 - area->x1 + 1;   // portrait area width  (<=240)
    const int ah = area->y2 - area->y1 + 1;   // portrait area height
    if (!s_rot || !s_band || aw * ah > s_rot_px) { lv_disp_flush_ready(drv); return; }

    // Rotate 90 deg CCW: portrait (px,py) -> rotated area of size (ah x aw).
    // dst[i,j] where dst width = ah, dst height = aw.
    //   dst_x = py_local ; dst_y = (aw-1) - px_local   (CCW)
    for (int py = 0; py < ah; py++) {
        for (int pxc = 0; pxc < aw; pxc++) {
            lv_color_t c = px[py * aw + pxc];
            int dx = py;
            int dy = (aw - 1) - pxc;
            s_rot[dy * ah + dx] = c;
        }
    }
    // Map portrait area -> panel (native 536x240) + emit in horizontal BANDS
    // through a small internal-DMA buffer (a single full-frame draw from PSRAM
    // truncates). draw_bitmap is ASYNC, so wait on s_tx_done before reusing the
    // band buffer (else the next memcpy corrupts the in-flight DMA).
    int px1 = area->y1;                 // panel x start (width = ah)
    int py1 = (UI_W - 1) - area->x2;    // panel y start (height = aw)
    const int rot_w = ah;
    int rows_per = s_band_px / rot_w;
    if (rows_per < 1) rows_per = 1;
    for (int r = 0; r < aw; r += rows_per) {
        int rh = (r + rows_per <= aw) ? rows_per : (aw - r);
        memcpy(s_band, &s_rot[(size_t)r * rot_w], (size_t)rh * rot_w * sizeof(lv_color_t));
        esp_lcd_panel_draw_bitmap(panel, px1, py1 + r, px1 + rot_w, py1 + r + rh, s_band);
        xSemaphoreTake(s_tx_done, pdMS_TO_TICKS(100));   // wait for this band's DMA
    }
    lv_disp_flush_ready(drv);
}

// QSPI needs even coordinate boundaries.
static void rounder_cb(lv_disp_drv_t *, lv_area_t *area) {
    area->x1 = (area->x1 >> 1) << 1;
    area->y1 = (area->y1 >> 1) << 1;
    area->x2 = ((area->x2 >> 1) << 1) + 1;
    area->y2 = ((area->y2 >> 1) << 1) + 1;
}

// Apply rotation to the physical panel when LVGL rotation changes.
// (Currently unused — kept for when we switch to portrait. Suppress warning.)
__attribute__((unused))
static void update_cb(lv_disp_drv_t *drv) {
    auto panel = (esp_lcd_panel_handle_t)drv->user_data;
    switch (drv->rotated) {
        case LV_DISP_ROT_NONE:  esp_lcd_panel_swap_xy(panel, false); esp_lcd_panel_mirror(panel, true,  false); break;
        case LV_DISP_ROT_90:    esp_lcd_panel_swap_xy(panel, true);  esp_lcd_panel_mirror(panel, true,  true);  break;
        case LV_DISP_ROT_180:   esp_lcd_panel_swap_xy(panel, false); esp_lcd_panel_mirror(panel, false, true);  break;
        case LV_DISP_ROT_270:   esp_lcd_panel_swap_xy(panel, true);  esp_lcd_panel_mirror(panel, false, false); break;
    }
}

static void tick_cb(void *) { lv_tick_inc(LVGL_TICK_MS); }

// LVGL touch read callback: pull a portrait-mapped point from the FT3168.
static void touch_read_cb(lv_indev_drv_t *, lv_indev_data_t *data) {
    uint16_t x, y;
    if (touch_read(&x, &y)) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

bool display_lock(int timeout_ms) {
    const TickType_t t = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(s_lvgl_mux, t) == pdTRUE;
}
void display_unlock(void) { xSemaphoreGive(s_lvgl_mux); }

static void lvgl_task(void *) {
    ESP_LOGI(TAG, "LVGL task started");
    uint32_t delay_ms = 500;
    while (true) {
        if (display_lock(-1)) {
            delay_ms = lv_timer_handler();
            display_unlock();
        }
        if (delay_ms > 500) delay_ms = 500;
        else if (delay_ms < 1) delay_ms = 1;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

void display_init(void) {
    ESP_LOGI(TAG, "init SPI (QSPI) bus");
    // NOTE: the SH8601_PANEL_BUS_QSPI_CONFIG macro uses designated initializers
    // out of struct-declaration order, which C++ rejects — build it by hand.
    // Pins from the Waveshare demo: PCLK=47, D0=18, D1=7, D2=48, D3=5.
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num  = PIN_LCD_PCLK;     // 47
    buscfg.data0_io_num = PIN_LCD_DATA0;    // 18
    buscfg.data1_io_num = PIN_LCD_DATA1;    // 7
    buscfg.data2_io_num = PIN_LCD_DATA2;    // 48
    buscfg.data3_io_num = PIN_LCD_DATA3;    // 5
    buscfg.max_transfer_sz = PANEL_W * PANEL_H * LCD_BIT_PER_PIXEL / 8;
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "install panel IO");
    esp_lcd_panel_io_handle_t io = nullptr;
    s_tx_done = xSemaphoreCreateBinary();
    const esp_lcd_panel_io_spi_config_t io_cfg =
        SH8601_PANEL_IO_QSPI_CONFIG(PIN_LCD_CS, color_done_cb, nullptr);
    sh8601_vendor_config_t vendor = {
        .init_cmds = s_init_cmds,
        .init_cmds_size = sizeof(s_init_cmds) / sizeof(s_init_cmds[0]),
        .flags = { .use_qspi_interface = 1 },
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_cfg, &io));

    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor,
    };
    ESP_LOGI(TAG, "install SH8601 driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(io, &panel_cfg, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    // This panel does NOT support hardware swap_xy (driver logs an error), so
    // portrait is done entirely in SOFTWARE by LVGL (sw_rotate below). Keep the
    // native orientation here (mirror only — the one that filled red full-screen).
    esp_lcd_panel_mirror(s_panel, true, false);
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    // Shared I2C bus (touch FT3168 + IMU + future carrier chips). 300 kHz like
    // the Waveshare demo. SDA=40, SCL=39 (board_pins.h).
    i2c_config_t i2c = {};
    i2c.mode = I2C_MODE_MASTER;
    i2c.sda_io_num = PIN_I2C_SDA;
    i2c.scl_io_num = PIN_I2C_SCL;
    i2c.sda_pullup_en = GPIO_PULLUP_ENABLE;
    i2c.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c.master.clk_speed = 300 * 1000;
    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &i2c));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, i2c.mode, 0, 0, 0));
    touch_init();

    ESP_LOGI(TAG, "init LVGL");
    lv_init();
    // FULL_REFRESH portrait: LVGL renders the ENTIRE 240x536 screen into one
    // buffer each frame, so flush_cb always gets the full screen (no partial
    // areas, no stride/rounder ambiguity -> no diagonal skew). flush_cb rotates
    // full_refresh: LVGL hands flush_cb the entire 240x536 frame each time.
    // Draw buffer + full rotated frame both in PSRAM (251 KB each, won't fit
    // internal). flush_cb rotates into s_rot, then pushes to the panel in small
    // internal-DMA BANDS (one big PSRAM->panel DMA truncates).
    size_t full_px = UI_W * UI_H;               // 240*536
    lv_color_t *b1 = (lv_color_t *)heap_caps_malloc(full_px * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(b1);
    lv_disp_draw_buf_init(&s_draw_buf, b1, nullptr, full_px);

    s_rot_px = full_px;
    s_rot = (lv_color_t *)heap_caps_malloc(s_rot_px * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(s_rot);

    // Small internal-DMA band buffer to push the rotated frame to the panel in
    // chunks (a single full-frame draw_bitmap from PSRAM truncates).
    s_band_px = PANEL_W * 40;          // 536*40*2 ~= 42 KB internal
    s_band = (lv_color_t *)heap_caps_malloc(s_band_px * sizeof(lv_color_t),
                                            MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    assert(s_band);

    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res = UI_W;          // 240 portrait (logical)
    s_disp_drv.ver_res = UI_H;          // 536
    s_disp_drv.flush_cb = flush_cb;
    s_disp_drv.draw_buf = &s_draw_buf;
    s_disp_drv.user_data = s_panel;
    s_disp_drv.full_refresh = 1;        // always flush the whole 240x536 frame
    lv_disp_drv_register(&s_disp_drv);

    // Register the FT3168 touch as an LVGL pointer input device.
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&indev_drv);

    const esp_timer_create_args_t tick_args = { .callback = &tick_cb, .name = "lvgl_tick" };
    esp_timer_handle_t tick = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick, LVGL_TICK_MS * 1000));

    s_lvgl_mux = xSemaphoreCreateMutex();
    // Pin LVGL to core 1 (currently free). On core 0 it starved the idle task
    // (main idle loop + LVGL both on core 0) -> task_wdt reset. When audio lands
    // on core 1, revisit: give LVGL a yield/priority that co-exists, or move it.
    xTaskCreatePinnedToCore(lvgl_task, "LVGL", 6 * 1024, nullptr, 2, nullptr, 1);  // core 1

    // Build our UI under the lock.
    if (display_lock(-1)) {
#if DISPLAY_REDTEST
        // BISECT: paint the active screen solid red via LVGL itself (uses the
        // correctly-sized LVGL buffers + flush path). If THIS shows red, the
        // panel/LVGL path is good and any later blankness is our UI/rotation.
        lv_obj_t *scr = lv_scr_act();
        lv_obj_set_style_bg_color(scr, lv_color_make(0xFF, 0x00, 0x00), 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
        ESP_LOGW(TAG, "DISPLAY_REDTEST: painting screen red");
#else
        ui_init();
#endif
        display_unlock();
    }
    ESP_LOGI(TAG, "display ready");
}
