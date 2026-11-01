# X360Browser

Experimental native browser for Xbox 360 RGH/JTAG. **No PC, browser.ini or relay is required.** Version 0.4.0 adds a green interface, clean home screen and native CSS layout. All UI text is in English.

![LG-inspired browser](docs/green-home.png)

## Install

Download [default.xex from the preview release](https://github.com/heltindev/X360Browser/releases/tag/v0.4.0-css-preview), copy it to a USB/HDD folder and open it through Aurora. Keep the console connected to the Internet and its date/time correct for HTTPS certificates. Old INI files are ignored.

## Interface and controls

The green top bar, green focus outlines, recommended-site tiles and right-side menu are inspired by the supplied LG browser references. No LG branding/assets are used. The toolbar and pointer disappear after **4.5 seconds without input**; moving the stick or pressing a button reveals them. Open menus and the keyboard keep controls visible.

- Left stick / D-pad: mouse pointer. A: click.
- LT/RT or right stick: scroll. B/LB: back. RB: forward.
- Y: address or Bing query. X: Google query. START/R3: reload. BACK: Home.
- L3: open/close the menu. D-pad up/down and A select an item; B closes it.
- During video: A pauses/resumes, B stops.

The cursor is an original anti-aliased image icon with hover coloring. It uses the gamepad; physical USB mouse input is not implemented.

## Images and video

| Content | Current implementation |
|---|---|
| HTML and supported CSS | Native litehtml layout, external stylesheets, style blocks, inline styles, boxes, floats, tables, fonts/colors/borders and DOM links |
| JPEG, PNG, BMP | Downloaded directly and decoded with stb_image |
| GIF | First frame only |
| Page images | Up to eight HTTPS img resources; includes basic data-src lazy-image URLs |
| Direct image address | Native image view |
| Direct WMV/XMV video | Buffered in RAM and handed to native XMedia2 with XAudio2 audio; codec/hardware verification pending |
| HTML video/source tags | Clickable video cards for direct URLs |
| MP4/H.264, HLS/DASH, DRM, YouTube players | Not implemented |
| JavaScript, modern Flexbox/Grid, cookies/login, interactive forms | Not implemented |
| SVG, WebP, CSS background images, animated GIF | Not implemented |

Limits: 2 MB per image, 4096 pixels per dimension and 4 million decoded pixels; images are scaled to fit. Video buffering is capped at 64 MB and completes before playback. Unsupported codecs/content show an error instead of a success message. The renderer is a VS2010-compatible port of legacy litehtml v0.3. It renders supported static HTML/CSS, not the complete modern web platform. Flexbox/Grid, JavaScript-driven pages, web fonts and advanced CSS can still render incompletely. Video format limits are unchanged.

## Preview watermark

**Developed by @heltonleiras** remains visible on pages and the video overlay, even when the toolbar is hidden. It is enabled by X360BROWSER_PRERELEASE=1 in src/lg_ui.h. A stable release can build with that definition set to 0; this release remains a preview.

## Build and verification

Visual Studio 2010 + locally installed Xbox 360 SDK: open X360Browser.vcxproj with Release | Xbox 360 or run build-release.cmd. Output: Release/default.xex. Proprietary SDK source/tools are not shipped.

Host tests exercise PNG/JPEG/GIF/BMP decoding, rejected malformed data, image/video HTML parsing, layout, pointer/menu hit tests, toolbar inactivity and watermark persistence. The same UI code produces docs screenshots. Network tests use the same HTTPS transport through a Windows adapter. **Physical Xbox testing, video/audio decoding and playback are still pending**; host tests do not prove console compatibility.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/build-native-host.ps1
.\obj\NativeHost.exe --network
```

A native image test page is available at https://raw.githubusercontent.com/heltindev/X360Browser/main/tests/fixtures/media.html . It contains original red/blue PNG, JPEG, GIF and BMP fixtures.

GPL-3.0. See LICENSE, THIRD_PARTY.md and docs/build-verification.json. Independent project, not affiliated with LG or Microsoft.

## CSS verification and source review

![CSS layout test](docs/css-browser.png)

The fixture at tests/fixtures/layout.html uses an external stylesheet, inline styles, two columns, linked images and clickable DOM links. The same native rendering code is used by the Xbox binary and host checks. CSS resources are limited to eight unique stylesheets, 256 KB per stylesheet and 512 KB total; styled HTML is capped at 512 KB, with a reader fallback above that. Images use a bounded native cache.

Both supplied files were reviewed in full: [video/GPU review](docs/UPLOADED-CODE-REVIEW.md). They inform video/graphics work, not CSS parsing. This release does not claim new H.264 playback.
