# Changelog

## 0.9.0-live-catalog-preview - 2026-11-01

- Identify X360HUB's JS-generated ports.html catalog as an initially empty modern JS shell.
- Render actual X360HUB live catalog data on device from data/catalog.json via direct native HTTPS.
- Use Duktape to parse bounded JSON and generate escaped native CSS/HTML cards with original image, source and download URLs.
- Paginate up to 8 items per page and expose Next/Previous links.
- Add URL validation, escaping, live JSON/network/image and pagination host tests.
- Explicitly label the compatibility view. This does NOT enable arbitrary modern websites, complete DOM events, Flexbox/Grid, MP4 or YouTube.
- Physical Xbox console test remains pending; the previous hardware builds were reported broken.


## 0.8.0-js-layout-preview - 2026-11-01

- Vendor and compile Duktape 2.7 into the Xbox 360 XEX and Windows host test.
- Execute bounded inline and 3 same-origin external scripts, with basic document.write/ID setters.
- Constrain JavaScript heap to 6 MiB and bytecode timeout to 300 ms; test infinite loops.
- Render basic horizontal flex rows and 2-4 column CSS grids via float fallbacks; pixel checks pass.
- Add on-console CSS/JS success and failure counts.
- Windows host X360HUB HTML/CSS passes; both external X360HUB scripts still cause syntax errors.
- Full DOM/events, modern JavaScript frameworks, true Flexbox/Grid and MP4/YouTube NOT implemented.
- Physical Xbox hardware validation PENDING; older builds failed owner tests.



## 0.7.0-css-network-preview - 2026-11-01

- Vendor and compile upstream zlib 1.3.1 into both PowerPC XEX and Windows host build; request/decode gzip/deflate HTTP content under output-size limits.
- Set resource-specific HTTP Accept for CSS and image requests; cap stylesheet inactivity to 5.5 seconds and skip Google Fonts CSS downloads since WOFF is unsupported.
- Resolve simple :root CSS variables (var(--name)) including static fallback, and preprocess CSS only within style elements rather than the complete HTML text.
- Add real X360HUB live CSS fetch/paint tests and compressed response decoding fixtures; fix the network CSS draw regression where an incorrect width was silently skipped.
- Studied NetSurf and the PowerPC MacSurf port. Neither is a drop-in XEX dependency; HTML5 JavaScript/DOM, real Flex/Grid and MP4 remain unsupported in this build.
- Host/network tests pass; hardware test still required, no false claims of full browser compatibility.


## 0.6.0-hardware-preview - 2026-11-01

- Respond to unsuccessful physical Xbox 360 test of 0.5: prioritize page loading, responsiveness, CSS and reduced stutter.
- Start on native shortcut screen; X360HUB, Google, Wikipedia and network test. Text queries still use Google.
- Show GlobalMemoryStatus physical-memory MB/%; remove CPU% and frame metrics.
- Use processor affinity for network worker; add frame loop yield; reduce network inactivity timeout.
- Avoid repeated text/image scaling, hover DOM traversal and per-image reflows.
- Accept bigger styled HTML/CSS and add basic media-width/flex/grid readable fallback (not full modern CSS).
- Report CSS fetch failures on-screen for console diagnostics.
- Host/XEX compilation and HTTPS/CSS/image fixture tests pass. **Physical console validation pending.**


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
