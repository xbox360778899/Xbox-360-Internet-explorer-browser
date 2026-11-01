# X360Browser

## 0.8.0-js-layout-preview changes

Duktape 2.7.0 now compiles inside the native Xbox PowerPC XEX. It executes bounded ES5-compatible inline scripts and up to three same-origin external scripts. The limited DOM bridge supports document.write, document.getElementById(id).innerHTML/textContent setters and querySelector('#id'). The JavaScript engine uses a 6 MiB heap limit and a 300 ms bytecode execution limit, both tested on the Windows host. It is NOT a full DOM/event/browser API environment, modern ES module loader, or Chrome-compatible runtime.

Horizontal Flexbox and simple grid-template-columns:repeat(2-4,1fr) styles now receive float-based layout fallbacks tested by pixel comparisons. Those are NOT complete CSS Flexbox or Grid layouts.

The Windows host fetched X360HUB HTML and one stylesheet successfully, but both external X360HUB JavaScript files still fail syntax parsing in Duktape. This console-native browser cannot yet render all features of modern JS websites. MP4/H264/YouTube/streaming remains unimplemented; the experimental native WMV/XMV player is unchanged. The 0.8 XEX has NOT yet been tested on physical Xbox hardware.



This revision includes zlib 1.3.1 gzip/deflate HTTP decoding, an HTTP Accept header suitable for CSS/image requests, 5.5-second per-resource inactivity timeout, and limited constant :root CSS variable substitution for visual compatibility. It reduces repeated Google Fonts requests that this renderer cannot use. No full modern DOM/JavaScript, Flexbox/Grid or MP4 browser engine is claimed.

The native home screen opens first (Google, X360HUB, Wikipedia and connection-test shortcuts). Search queries always go to Google; the small tab number at the top-right opens the tab picker. Create or switch among up to four tabs, each retaining its page, history and scrolling. The page fills the screen when the toolbar automatically hides. During navigation, an animated spinner appears over a dark overlay. The preview watermark is painted over the webpage.

HTML and external/inline CSS are rendered before images, which load progressively. A 6 MiB RAM-only response cache speeds up revisits within 180 seconds; it is wiped when the application exits, and explicitly private/no-store/no-cache responses are not stored. CSS supports common CSS content types. This legacy litehtml engine still lacks complete modern CSS and JavaScript-driven interactivity.

The dim bottom-left statistics display the physical RAM usage reported by the Xbox SDK, as MB and percentage of reported physical memory. The RAM indicator uses GlobalMemoryStatus to show SDK-reported physical memory used/available in MB and percent, not response cache size. CPU utilisation and frame-time metrics are hidden. CPU/GPU temperatures are queried read-only from the console SMC and displayed only when within a plausible range. This sensor path is not yet tested on physical hardware. This is system-wide physical RAM pressure, not just memory used by the browser. No PC helper, browser.ini or PowerPC simulator is required: the XEX is compiled natively for the console. The older builds had owner-reported physical-console failures. The 0.8 build passed SDK and Windows host tests; physical-console compatibility remains unverified.



Experimental native browser for Xbox 360 RGH/JTAG. **No PC, browser.ini or relay is required.** Version 0.8.0-js-layout-preview adds full-screen pages, four switchable tabs, a dark loading animation, progressive images, a bounded RAM cache and Google as the default search provider, with a native shortcut home screen. All UI text is in English.

![LG-inspired browser](docs/green-home.png)

## Install

For the 0.8.0-js-layout-preview build use the local Release/default.xex on the build PC, copy it to USB/HDD and launch through Aurora. The existing linked 0.4.0 preview release does not contain this version. Keep the console connected to the Internet and its date/time correct for HTTPS certificates. Old INI files are ignored.

## Interface and controls

The green top bar, green focus outlines, recommended-site tiles and right-side menu are inspired by the supplied LG browser references. No LG branding/assets are used. The toolbar and pointer disappear after **4.5 seconds without input**; moving the stick or pressing a button reveals them. Open menus and the keyboard keep controls visible.

- Left stick / D-pad: mouse pointer. A: click.
- LT/RT or right stick: scroll. B/LB: back. RB: forward.
- Y/X: address or Google query. START/R3: reload (bypasses the current page cache entry). BACK/Home: native shortcuts page.
- L3: open/close the menu. D-pad up/down and A select an item; B closes it.
- During video: A pauses/resumes, B stops.

The cursor is an original anti-aliased image icon with hover coloring. It uses the gamepad; physical USB mouse input is not implemented.

## Images and video

| Content | Current implementation |
|---|---|
| HTML and supported CSS | Native litehtml layout; external/inline styles, simple responsive-width media rules, constant :root CSS variables, boxes and links; simple float-based horizontal Flex/Grid compatibility, not full standards-conformant layout |
| JPEG, PNG, BMP | Downloaded directly and decoded with stb_image |
| GIF | First frame only |
| Page images | Up to eight HTTPS img resources; includes basic data-src lazy-image URLs |
| Direct image address | Native image view |
| Direct WMV/XMV video | Buffered in RAM and handed to native XMedia2 with XAudio2 audio; codec/hardware verification pending |
| HTML video/source tags | Clickable video cards for direct URLs |
| MP4/H.264, HLS/DASH, DRM, YouTube players | Not implemented |
| JavaScript ES5 | Bounded Duktape integration with partial document.write/getElementById and same-origin external script support. Not full DOM. |
| Modern JS, full Flexbox/Grid, cookies/login, interactive forms | Not implemented |
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

The fixture at tests/fixtures/layout.html uses an external stylesheet, inline styles, two columns, linked images and clickable DOM links. The same native rendering code is used by the Xbox binary and host checks. CSS resources are limited to ten unique stylesheets, 512 KB per stylesheet and 1.5 MB total; styled HTML is capped at 1 MB, with a reader fallback above that. Images use a bounded native cache.

Both supplied files were reviewed in full: [video/GPU review](docs/UPLOADED-CODE-REVIEW.md). They inform video/graphics work, not CSS parsing. This release does not claim new H.264 playback.
