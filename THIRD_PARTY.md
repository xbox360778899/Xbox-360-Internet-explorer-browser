# Third-party code and attribution

| Component | Source | License | Usage and changes |
|---|---|---|---|
| Orbita legacy 0.21.44 XUI | User-provided `orbita-main/app/h264play/source/ui` | GPL-3.0, original top-level LICENSE retained | `vendor/orbita/xui.h`, `xui_gfx.c`, `xui_text.c`, `xui_assets.h`. Reused CPU drawing primitives and font rasterization. Original comments removed for English-only source; attribution added; compiled as C++ for the VS2010 compiler. Original symbols retained. New Xbox XEX rendering and network layer authored separately. |
| Inter 4.1 font | The Inter Project Authors, 2016; supplied under `orbita-main/third_party/inter-4.1` | SIL Open Font License 1.1 | Rasterized glyphs embedded by Orbita. Full license at `vendor/orbita/Inter-OFL.txt`. |
| Playwright Python | Microsoft, https://github.com/microsoft/playwright-python | Apache-2.0 | Separately installed PC dependency. Controls locally installed Edge/Chrome. No browser binaries vendored. |
| Pillow | Python Pillow contributors, https://github.com/python-pillow/Pillow | MIT-CMU / HPND, see upstream license | Separately installed PC dependency. Converts rendered page pixels to Xbox network byte order. |

The SHA-256 hashes of the original user-supplied Orbita files, before adapting comments, are recorded in `vendor/orbita/source-manifest.json`. An upstream repository URL or original Git commit was not supplied; none is invented here. Orbita branding and logo are not displayed by X360Browser.

The original legacy source archive is not copied wholesale. No original Orbita ELF binaries, beta services, keys, authentication flows or firmware are shipped.

In 0.1.1 the browser layout follows Orbita's 64-pixel TV margins and measured font baselines. Drawing primitives and typography still come directly from the supplied XUI modules; layout, navigation controls, configuration parsing and diagnostics are implemented in `src/browser_ui.h` and `src/browser_config.h`.
