# Native Xbox 360 test guide - 0.3.0

Copy only default.xex to the console. No PC service or INI is required.

1. Open the native home page while the PC bridge is stopped.
2. Move the pointer with the left stick and click the address field with A.
3. Enter https://example.com/ and check that readable text appears.
4. Press Y for an HTTPS address or Bing query; X starts a Google query.
5. Click a link with A. Check B/LB back, RB forward and trigger scrolling.
6. Wait 4.5 seconds without input: toolbar and pointer should hide, while the watermark stays visible. Move the stick to reveal them.
7. Open the three-dot menu, or press L3 and use D-pad up/down plus A. B closes it.
8. Open https://raw.githubusercontent.com/heltindev/X360Browser/main/tests/fixtures/media.html . Check red/blue PNG, JPEG, GIF and BMP images.
9. Open your own direct HTTPS WMV/XMV file, at most 64 MB. Wait for buffering; test picture/audio, A pause/resume and B stop. Keep an image of any decoder error.

The reader does not execute JavaScript or reproduce full CSS layouts. Search providers can return verification pages. MP4/H.264, HLS/DASH, DRM and JavaScript video sites are not implemented. Native WMV/XMV playback depends on the file's codec and still requires hardware testing.

| Failure | Report |
|---|---|
| Network startup | Both startup numbers |
| Connection | URL, step and hexadecimal error |
| TLS | TLS number, network error, console date/time |
| HTTP | Status and address |
| Empty page | Whether the site requires JavaScript or verification |

Physical USB mouse support is not implemented; the mouse pointer uses the gamepad. A physical Xbox test has not been performed during development.
