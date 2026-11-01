# X360Browser 0.6 hardware test notes

The previous 0.5 preview compiled and passed Windows host tests but the owner tested the physical Xbox 360 and reported that pages were slow, styles absent, and navigation unreliable. This is a hardware-derived blocker, **not** a verified success.

## Changes in this build

- Local start page, with direct X360HUB link, Google, Wikipedia, and a simple https://example.com connectivity test.
- Search queries use Google; Home controls open the local page.
- Memory indicator uses SDK GlobalMemoryStatus (physical RAM used/total and %) instead of cache bytes, frame time or CPU percentage. The SDK-reported usable total may be smaller than the console's advertised 512 MiB.
- Worker assigned to hardware thread 2 via XSetThreadProcessor. Rendering loop yields longer instead of spinning.
- CSS accepts up to 1 MiB of styled HTML and 10 stylesheets; import cap 512 KiB each/1.5 MiB total. Includes simple width-based @media flattening for a 1280px screen; flex/grid properties degrade to stacked block layouts, **not full Flexbox/Grid**.
- Avoid repeat hover DOM traversal, repeated image resizing, and large transient text allocations. Coalesce some expensive image reflows.
- Network inactivity timeout is 10 seconds per request instead of 30 seconds, to avoid waiting indefinitely on a single resource. Slow hosts may time out.
- A discrete CSS failure badge shows how many external stylesheets failed.

## Limits and researched engines

NetSurf: https://www.netsurf-browser.org/ — designed for limited-memory systems and supports a subset of CSS including some Flexbox, but its ports and dependencies are not an Xbox 360 XDK/XEX drop-in.
Duktape: https://duktape.org/ — small ES5-compatible JavaScript interpreter, but installing it does not provide DOM, fetch, media pipeline, events, or a modern browser engine.
A complete modern Chromium/Edge engine, full modern Flex/Grid, YouTube/MP4/HLS/DASH/DRM, and arbitrary browser JavaScript are **not implemented** and should not be advertised as working.
No PC, relay or PowerPC emulator required: this project compiles native Xbox 360 PowerPC code. The XDK handles native PPC; emulating a PowerPC CPU on the console itself is not a feature.

## Validation

Build: Release/default.xex should contain XEX2.
Windows host: tests/build-native-host.ps1 and obj/NativeHost.exe --network.
**Physical console regression test: pending**. The only confirmed physical outcome available to us is the previous build's reported failure.

Start at home. Test https://example.com/ first to rule out TLS/network problems. Then test the repo's tests/fixtures/layout.html and Google Search. Photograph any error/blank page, and note whether CSS failure count appears. Differentiate a TLS/HTTPS failure from lack of JavaScript/modern CSS. Check that the RAM indicator reports sane numbers and X360HUB shortcut opens.
