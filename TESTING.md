# Native Xbox 360 test guide - 0.8.0-js-layout-preview

On startup the preview opens its local browser:home with Google, X360HUB, Wikipedia, and Test website buttons, without a PC. During loading expect a darkened screen and spinner. Click the number beside the address bar to create/switch tabs; verify the original URL, page and scroll are retained. When the toolbar hides the page fills 1280x720. The watermark remains above the page. The dim lower-left indicator reports SDK physical RAM in MB and percentage, with no frame or CPU utilization values; CPU/GPU temperatures should appear after the SMC query when valid, otherwise N/A. Revisit the CSS layout fixture to test cache and progressive image loading. Up to four tabs can be created (closing tabs is not implemented in this preview).



Copy only default.xex to the console. No PC service or INI is required.

1. Start the browser; it should open its local home screen with the X360HUB shortcut while the PC bridge is stopped.
2. Move the pointer with the left stick and click the address field with A.
3. Enter https://example.com/ and check that readable text appears.
4. Press Y/X, type a regular text query and confirm the URL begins with https://www.google.com/search?q=. Home/BACK returns to the local shortcuts screen.
5. Click a link with A. Check B/LB back, RB forward and trigger scrolling.
6. Wait 4.5 seconds without input: toolbar and pointer should hide, while the watermark stays visible. Move the stick to reveal them.
7. Open the three-dot menu, or press L3 and use D-pad up/down plus A. B closes it.
8. Open https://raw.githubusercontent.com/heltindev/X360Browser/main/tests/fixtures/media.html . Check red/blue PNG, JPEG, GIF and BMP images.
9. Open your own direct HTTPS WMV/XMV file, at most 64 MB. Wait for buffering; test picture/audio, A pause/resume and B stop. Keep an image of any decoder error.
10. Open https://raw.githubusercontent.com/heltindev/X360Browser/main/tests/fixtures/layout.html . Expect a green banner, two bordered columns, a tinted inline-style box and a red/blue image. Click the link in the first column.

The litehtml renderer handles a legacy subset of CSS layout, external stylesheets, style blocks and inline styling. It does not execute JavaScript or implement modern Flexbox/Grid and the full CSS platform. Search providers can return verification pages. MP4/H.264, HLS/DASH, DRM and JavaScript video sites are not implemented. Native WMV/XMV playback depends on the file's codec and still requires hardware testing.

| Failure | Report |
|---|---|
| Network startup | Both startup numbers |
| Connection | URL, step and hexadecimal error |
| TLS | TLS number, network error, console date/time |
| HTTP | Status and address |
| Empty page | Whether the site requires JavaScript or verification |

Physical USB mouse support is not implemented; the mouse pointer uses the gamepad. A physical Xbox test has not been performed during development.

## Hardware regression tests for 0.8

Open example.com, then X360HUB. Record final CSS N OK/N failed and JS N OK/N failed stats. Test a local basic script with document.write to confirm JS renders on Xbox. Compare horizontal flex rows and grid card positions. MP4/YouTube remains unsupported. A host test is not a hardware pass.

## 0.9 live X360HUB catalog: required Xbox 360 test

1. Replace the previous default.xex with Release/default.xex and launch from Aurora.
2. Open the native X360HUB shortcut and select Explore catalog (ports.html).
3. A labeled native compatibility catalog should display the published titles, horizontal covers, and original source/download links fetched from data/catalog.json over native HTTPS.
4. Use Next page and Previous page; the live feed is divided into groups of eight.
5. If this still fails, note the exact URL, on-screen network status and screenshot. The build/host-network tests alone cannot certify the physical console.
6. JavaScript-heavy unrelated sites and MP4/H.264 remain unsupported; do not record those as working.

## Google homepage - 0.9.1 Xbox hardware test

1. Copy Release/default.xex to the RGH/JTAG console, open from Aurora, and select Google on the native X360Browser home screen.
2. The page must download the official Google HTML, CSS, logo and links via console HTTPS. The colorful 272x92 Google wordmark should be centered. This is NOT a screenshot of Google.
3. Click the native outlined Google search field with A, type text on the Xbox keyboard and submit. Verify it navigates to https://www.google.com/search?q=YOUR_QUERY. A dotted query such as x360hub.com must remain a Google query.
4. Wait for the toolbar to retract, then click the same search field and ensure the hitbox moves with the displayed form. Try both toolbar states.
5. Report the console's actual result (screenshot/error and URL). Passing the Windows-host test does not mean the physical Xbox passed. Modern Google search result pages may still be incomplete because the litehtml + Duktape browser engine is not Chrome.
