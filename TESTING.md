# Native Xbox 360 test guide - 0.5.0-preview

On startup the new preview navigates to Google without a PC. During loading expect a darkened screen and spinner. Click the number beside the address bar to create/switch tabs; verify the original URL, page and scroll are retained. When the toolbar hides the page fills 1280x720. The watermark remains above the page. The dim lower-left statistics contain measured cache usage and frame time, but CPU utilisation remains N/A; CPU/GPU temperatures should appear after the SMC query when valid, otherwise N/A. Revisit the CSS layout fixture to test cache and progressive image loading. Up to four tabs can be created (closing tabs is not implemented in this preview).



Copy only default.xex to the console. No PC service or INI is required.

1. Start the browser; it should open Google automatically with the PC bridge stopped.
2. Move the pointer with the left stick and click the address field with A.
3. Enter https://example.com/ and check that readable text appears.
4. Press Y or X for an HTTPS address or Google query; Home/BACK returns to Google.
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
