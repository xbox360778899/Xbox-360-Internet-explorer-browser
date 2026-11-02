# Google Search on Xbox 360: QuickJS port diagnostic

**Status: Not working yet.** This document records real PowerPC compilation experiments. It does not announce a working modern browser engine.

## What was tested

- Xbox 360 SDK 2010-01, Visual Studio 2010, and the Xenon PowerPC compiler, which successfully build the existing X360Browser.
- QuickJSpp (https://github.com/c-smile/quickjspp), a QuickJS port for Microsoft Visual C++ intended for VS2017 and VS2019.
- An isolated XEX project with `quickjs.c`, `libregexp.c`, `libunicode.c`, `cutils.c`, and a small modern JavaScript test using `const`, arrow functions, and `Array.map`.
- Experimental changes for `inttypes.h`, `fenv.h`, `windows.h`, `intrin.h`, Xbox big-endian byte order, and C `inline`.

**Result:** The QuickJS prototype **did not compile** with the Xbox 360 PowerPC SDK. After fixing the initial missing headers, the C compilation path produced at least **102 errors** before reaching the compiler's error limit, including C99 declaration and type incompatibilities. The C++ path produced at least **126 errors**, including invalid `void*` conversions and missing `_BitScan*` and `_msize` functions. These are incomplete counts because compilation stopped at the error limit.

No failed experimental code was linked into the production `default.xex`. Duktape remains the active JavaScript engine, and no modern replacement has been validated.

## Reproducible Google Search test

From the project root, run:

```bash
python tools/verify_google_results.py
```

On November 1, 2026, the native-style direct request to `https://www.google.com/search?q=Xbox%20360` returned HTTP 200 and approximately 93 KB of HTML, but required JavaScript through `/httpservice/retry/enablejs` and did not include genuine organic results. The test correctly returned **FAIL**. When a Lynx browser identifier was used, Google instead returned an unsupported-browser warning, also without search results. Changing the User-Agent alone is not a solution.

## What is required for actual Google Search support

1. A modern compiler targeting the **correct Xenon/XDK ABI**, PowerPC big-endian code, and a compatible C runtime, followed by a port of an ES2020+ engine.
2. A minimal native XEX running the engine on a **physical Xbox 360 RGH/JTAG**, with verified execution and memory limits.
3. A persistent DOM, `window`/`document`, events, CSSOM, promises, and script modules instead of the current HTML preprocessor.
4. Secure browser Web APIs: fetch/XHR, URL handling, cookies, storage, origin restrictions, and TLS.
5. Updated HTML/CSS rendering with true Flexbox, Grid, responsive layout, and font support.
6. A final hardware test showing **real Google result titles and links** rendered and clickable on the Xbox itself. Bing results, screenshots, and remotely rendered pages do not qualify.

An LLVM toolchain with **Xenon ABI support**, such as experimental OXDK-related compiler work, is a possible research direction, but has **not** been installed or validated here. Generic LLVM PowerPC support does not automatically match the proprietary XDK ABI.

**Until the requirements above are met, actual Google Search results remain unsupported.** The browser itself must continue running directly on the console without any PC, remote browser, or intermediary server.
