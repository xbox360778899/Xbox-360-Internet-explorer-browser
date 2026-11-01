# Changelog

## 0.1.1 - 2026-11-01

- Replace CRT INI loading with native Xbox file reads and launch/HDD/USB path fallback.
- Accept BOM, UTF-16LE, CRLF/LF and whitespace; validate settings and identify malformed fields.
- Separate settings-loaded, token-mismatch, TCP connection and file-read states.
- Remove the unconditional instruction to copy the INI while the app is connecting.
- Add START to reload settings and retry without restarting the app.
- Rebuild the UI using Orbita font metrics, baseline placement, safe margins and measured controller badges.
- Add working, focusable toolbar controls through L3, D-pad and A.
- Include an administrator-run firewall helper limited to one Xbox IP and TCP 36080.
- Verify config parsing with native host tests and inspect shared-code UI renders.
- Compile a new XEX2 test binary. Physical-console verification is still pending.

## 0.1.0 - 2026-11-01

- Initial PC-assisted Xbox 360 browser preview.
