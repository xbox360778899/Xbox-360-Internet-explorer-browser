# Browser engine porting research — X360Browser 0.7

User's physical Xbox 360 still shows incorrect layout, slow page loads, and missing CSS in 0.6. Do not equate a successful host fetch with a successful Xbox page.

## Current working transport and page renderer

- Native XDK PowerPC executable with direct TLS 1.2 BearSSL and HTTP/1.1; no PC relay.
- zlib 1.3.1 added in 0.7 for gzip/deflate HTTP payloads, with decompressed size limits.
- External CSS downloads include the proper Accept header. Windows-host **live X360HUB** check returned one CSS stylesheet, zero CSS download failures and correctly colored CSS pixels, but this was not tested on the real console.
- Existing litehtml v0.3 is only a limited static layout engine, not a full browser. It lacks a JavaScript execution/DOM subsystem, full CSS Grid/Flexbox layout, actual image/video web elements and modern web APIs. Small CSS-variable and media-query compatibility transformations are NOT a replacement for a full renderer.
- XMedia2 WMV/XMV player remains experimental; an MP4 video tag, H.264 streaming, MSE, DRM, YouTube etc. are not implemented.

## Engines researched

1. https://www.netsurf-browser.org/about/ — NetSurf has a low-resource framebuffer frontend with its own fetch, DOM, HTML and CSS; it has partial JS binding facilities. The correct long-term Xbox route is a separate NetSurf frontend with an XDK HTTP/TLS and native framebuffer adapter, not swapping a header file.
2. https://github.com/mplsllc/macsurf — real PowerPC Mac OS native browser adapting NetSurf and a QuickJS-based engine with stronger CSS Grid/Flex and web features. But Carbon/QuickDraw/Open Transport/CodeWarrior APIs and code licensing must be evaluated; it does not compile as an Xbox XEX simply by copying libraries.
3. https://duktape.org/guide — small ES5 interpreter; embedding its .c alone does not provide DOM, CSSOM, fetch, event handlers, video, forms, modern ES modules, or sufficient CPU/memory safety for arbitrary website scripts.
4. https://github.com/koukic223/NovaBrowser-Xbox-360-RGH-JTAG — exploratory Xbox NetSurf project, explicitly says its XEX build is gated on XDK, NetSurf sources, and the full bindings. Do not claim it is already runnable.

## Next engine milestone and acceptance gates

- Prototype NetSurf framebuffer-backed XDK frontend in an isolated project, include third-party license and source-version checksums. Verify link compatibility and native XEX format.
- Use measured console HTTP header timings, redirected URLs, decompression and PNG/CSS fetch logs; verify at least one fully styled real HTTPS page on **physical Xbox hardware**.
- Add real JavaScript only with a supported DOM binding plus strict execution/memory time limits, then confirm form interactions. No simulated JS API placeholders.
- Add media only after licensing a codec and testing codec/audio/synchronization in the XEX on original hardware.
- Treat CSS layout fidelity and JavaScript compatibility as separate pass/fail gates. 512 MB shared RAM requires bounded buffers and resources.

Current 0.7 goal is a faster, better-supported static page renderer and an honest foundation for the new engine, NOT a claim of parity with Edge or Chrome.
