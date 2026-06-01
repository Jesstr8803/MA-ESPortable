// ui_theme.h — shared style tokens for the MA-ESPortable UI (240x536 AMOLED).
// Mirrors the web prototype (ui-prototype/) so the look is consistent.
#pragma once

// Palette (true-black AMOLED background = pixels off = power saved)
#define UI_COL_BG        lv_color_hex(0x000000)
#define UI_COL_FG        lv_color_hex(0xFFFFFF)
#define UI_COL_DIM       lv_color_hex(0xB9B9C1)   // artist line
#define UI_COL_DIM2      lv_color_hex(0x6A6A70)   // album / tertiary
#define UI_COL_ACCENT    lv_color_hex(0x5AC8FA)   // progress / active
#define UI_COL_CARD      lv_color_hex(0x16161B)   // tiles / rows

// Panel geometry (portrait)
#define UI_W   240
#define UI_H   536

// Touch-target sizing (verified small-screen rules from the prototype)
#define UI_ROW_H        76     // list rows (~6 mm)
#define UI_PLAY_D       80     // play button diameter
#define UI_HIT          60     // transport hit-area
