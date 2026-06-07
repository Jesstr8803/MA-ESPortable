# Vendored-SDK patches

`managed_components/` is gitignored (ESP-IDF refetches it from the component
registry), so any local edits to vendored SDKs are lost on a clean checkout.
Patches that must persist live here and are re-applied after `idf.py reconfigure`
pulls the components.

## sendspin-httpd-stack.patch

**Problem:** The `sendspin-cpp` SDK starts its WebSocket server with
`HTTPD_DEFAULT_CONFIG()`, whose default task stack is **4096 bytes**. During the
Music Assistant connection handshake the `httpd` task overflows that stack →
`***ERROR*** A stack overflow in task httpd has been detected.` → reboot loop
(MA logs `close_code=1006`). The SDK exposes `httpd_psram_stack` (stack *location*)
but **no `stack_size` knob**, so we bump it directly.

**Fix:** in `managed_components/sendspin__sendspin-cpp/src/esp/ws_server.cpp`,
right after `httpd_config_t config = HTTPD_DEFAULT_CONFIG();`, add:

```cpp
    config.stack_size = 10240;   // default 4096 overflows on the MA handshake
```

Apply with `git apply firmware/patches/sendspin-httpd-stack.patch` from the repo
root (or just re-add the one line). TODO: upstream a `httpd_stack_size` config
field to the SDK so this patch becomes unnecessary.
