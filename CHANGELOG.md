# Changelog

## 0.2.0 - 2026-11-01

- Replace the PC-assisted renderer with native HTTPS sockets, BearSSL TLS 1.2 and a bounded HTML text/link reader.
- Remove the PC bridge, Python runtime requirement, INI loading and firewall helpers from the current version.
- Add an Edge-inspired native home screen with direct Bing/Google queries.
- Add a gamepad mouse pointer, hover feedback, clickable links/toolbar, scroll and history.
- Preserve certificate-chain, hostname and validity checks. HTTP addresses are upgraded to HTTPS.
- State current engine limits explicitly: no CSS, JavaScript, images/media, cookies, forms or physical USB mouse support.
- Test parser/navigation/rendering and the same HTTPS transport through a Windows host adapter.
- Physical Xbox testing remains pending; this is an experimental native reader, not the Microsoft Edge engine.

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
