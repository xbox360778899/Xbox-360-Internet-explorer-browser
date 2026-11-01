# Changelog

## 0.5.0-preview - 2026-11-01

- Full-screen webpage rendering with retracting toolbar and green dimming spinner.
- Preserve Developed by @heltonleiras on top of pages and video overlays.
- Implement up to four tabs with independent URLs, history, scroll state and page content.
- Google as both startup page and default search/Home.
- Fix CSS resource MIME acceptance; keep legacy litehtml stylesheet support and layout tests.
- Render CSS before images; progressively load images with oversized-image limits.
- Add a 6 MiB, 180-second in-memory response cache respecting no-store/private/no-cache directives.
- Show real cache bytes and frame time; query CPU/GPU temperatures read-only from SMC with range checks (hardware test pending). CPU load remains N/A.
- Compile native PowerPC XEX2, run Windows host and HTTPS/CSS fixture tests.
- Not yet supported: modern JS, full CSS Grid/Flexbox, MP4/H.264, persistent cache, tab closing, reliable CPU% or temperatures, USB mouse.
- Physical Xbox 360 hardware validation pending.



## 0.4.0 - 2026-11-01

- Add native HTML/CSS layout using a port of litehtml v0.3, compatible with VS2010/XDK.
- Fetch external stylesheets, support style blocks/inline styling and CSS-positioned images/backgrounds through the native HTTPS callback.
- Render supported CSS box layout, floats, tables, margins, padding, borders, fonts and colors; dispatch pointer events through the rendered DOM.
- Preserve a text-reader fallback for pages outside renderer limits. Full modern CSS and JavaScript compatibility is not claimed.
- Apply green chrome/cursor accents and remove the home media guide, control legend and normal status footer.
- Replace reload, search and navigation symbols with original antialiased masks on a shared grid.
- Synchronize the GPU before rewriting the shared UI texture.
- Review both supplied video/GPU files in full; record porting requirements without claiming new H.264 support.

## 0.3.0 - 2026-11-01

- Add an LG webOS-inspired gray toolbar, pink focus outlines and a right-side menu.
- Hide toolbar/pointer after 4.5 seconds without input and restore them on interaction.
- Replace the primitive pointer with original anti-aliased cursor icons.
- Add the prerelease watermark Developed by @heltonleiras on pages and video overlays.
- Fetch/decode JPEG, PNG, BMP and GIF first-frame images, with bounded memory and image layout.
- Recognize HTML img, video and source tags and direct media URLs.
- Add RAM-buffered direct WMV/XMV playback through native XMedia2 and XAudio2. Video/audio hardware testing remains pending.
- Keep unsupported MP4, streaming/DRM, JavaScript and full CSS limitations explicit.
- Add tests for image decoding, malformed data, media parsing, menu hit tests, auto-hide and watermark persistence.

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
