# MA_Portable

A new project, building on the lineage of the **SendspinZero / sendspin-xiao** native firmware
(the working Music Assistant / Sendspin audio endpoint built on the Seeed XIAO ESP32-S3).

> The idea is still being shaped — this README is a placeholder anchor and will be replaced
> once scope is locked.

## Lineage & cross-links

This project can reach back into the predecessor project for code, design history, and the
hard-won fixes that made the endpoint work. Configured via `.claude/settings.local.json`
(`additionalDirectories` + read permissions):

| Resource | Path |
|---|---|
| Working firmware code | `c:\Users\Jess_\DropBox\Personal\ESP MA Endpoint` |
| Project memory (facts/decisions) | `…\ESP-MA-Endpoint\memory` |
| Past Claude sessions (transcripts) | `c:\Users\Jess_\.claude\projects\c--Users-Jess--DropBox-Personal-ESP-MA-Endpoint` |

Predecessor highlights to draw on: native ESP-IDF Sendspin client, captive-portal WiFi
provisioning, OTA, PCM5102A I2S sink, Kalman time-sync tuning, XSMT hardware mute,
`/status` metrics. (Predecessor shipped at **v0.2.14**, 2026-05-01.)

## Git workflow

- Work on `feat/*` branches, merge to `main` with `--no-ff`.
- Tag releases only on `main`.
- GitHub remote/project to be added later.
