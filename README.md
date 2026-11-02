# X360Browser

**An experimental web browser that runs 100% natively on Xbox 360 RGH/JTAG.** X360Browser is a PowerPC application distributed as `default.xex`. It does **not** require a PC, remote browser, streaming service, proxy, or intermediary server.

Your Xbox 360 connects directly to websites over the internet. **An internet connection and the websites' own servers are still required** to access online pages. The X360HUB catalog reads publicly available data directly from its official website.

![X360Browser home screen](docs/green-home.png)

## Download and install

**Current version: 0.10.0 (experimental)**

[Download default.xex](https://github.com/heltindev/X360Browser/releases/download/v0.10.0-dom-foundation-preview/default.xex) | [Current release](https://github.com/heltindev/X360Browser/releases/tag/v0.10.0-dom-foundation-preview)

1. Download `default.xex` from the current release.
2. Copy the file to your Xbox 360 RGH/JTAG HDD or USB drive.
3. Launch the XEX with Aurora.

No PC application or self-hosted server is necessary. Make sure your console's date and time are correct for HTTPS certificates.

## Current features

- Dark and green Smart TV-inspired interface with an auto-hiding toolbar.
- Controller-operated mouse cursor, up to four tabs, history, and an address bar.
- Direct HTTP/HTTPS downloads of supported HTML, CSS, and images.
- Partial JavaScript and responsive layout support; JPEG, PNG, BMP, and the first frame of GIF images.
- Google homepage and a native search field; actual Google search results are **not working yet**.
- Optional, clearly identified Bing search results as a separate provider.
- A native view of the X360HUB catalog using public data fetched by the console.

**Important:** This is an experimental build. Many modern websites are not fully supported. Windows-host tests do not replace testing on real Xbox 360 hardware.

## Controller buttons

| Button | Action |
|---|---|
| Left stick / D-pad | Move the cursor |
| A | Click |
| LT / RT or right stick | Scroll |
| B / LB | Back |
| RB | Forward |
| Y / X | Address bar or search |
| START / R3 | Reload |
| BACK | Home screen |
| L3 | Open the menu |

## Development roadmap

- **Modern JavaScript engine:** Port a modern ECMAScript runtime to Xbox 360 PowerPC.
- **Complete DOM:** Implement persistent nodes, events, forms, and dynamic page updates.
- **Browser Web APIs:** Support fetch/XHR, cookies, storage, asynchronous tasks, and web security rules.
- **HTML/CSS:** Improve Flexbox, Grid, fonts, positioning, and responsive layouts.
- **Google Search:** Display and navigate actual Google results without a blank screen or a substitute provider.
- **Video:** Implement real MP4/H.264 and HTML5 video playback, with longer-term support for video websites.
- **Performance and reliability:** Fix crashes, memory limits, rendering speed, and hardware compatibility.

**All browser execution must remain local to the Xbox 360**, without a PC, remote browser, or intermediary server. Online websites still use their own internet servers.

## Building from source

The project is written in C/C++ for **Visual Studio 2010 + Xbox 360 SDK**. Run `build-release.cmd` to produce `Release/default.xex`.

The current build uses litehtml and Duktape, with known limitations. It is an independent project, not affiliated with Microsoft, LG, or Google. Licensed under GPL-3.0. See [THIRD_PARTY.md](THIRD_PARTY.md) for third-party notices.

**Developed by [@heltonleiras](https://github.com/heltindev).**
