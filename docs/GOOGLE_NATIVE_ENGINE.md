# Native Google Search / modern web compatibility

## What is verified

As of 2026-11-01 the Xbox browser is an **XDK 2010 PowerPC XEX**, not a port of Chromium, Firefox or Edge. Its HTML/CSS engine is litehtml 0.3; its JavaScript engine is Duktape 2.7 with a small custom DOM shim.

A real HTTP 200 response from `https://www.google.com/search?q=Xbox%20360` was downloaded using the same native transport on the Windows host. The ~93 KB reply contained `/httpservice/retry/enablejs` and **no actual results**. An older Lynx-compatible User-Agent returned ~2.4 KB of **"Atualize o navegador"**, again **without results**. Chrome/Android/iPhone user agents still yielded the JS-required reply. A fake UI, Bing response, User-Agent substitution, proxy screenshot, or overriding CSS does **not** make the Google Search results genuinely render.

## DOM foundation now implemented

The on-console Duktape bridge can construct `document.createElement` nodes, set safe attributes and text, append detached descendants, append to document.body or element IDs, call `insertAdjacentHTML('beforeend', ...)`, and process `DOMContentLoaded` and `window.addEventListener('load', ...)`/onload before rendering. The HTML output is regenerated before litehtml paints it. The existing script time (300 ms), heap (6 MiB), script count and output bounds still apply. The host test renders actual output from these callbacks. This is **NOT** a standards-compliant persistent DOM: mutations are not automatically linked to live layout, and subsequent controller click events do not yet invoke JavaScript event listeners.

## What must be developed before native Google Search can work

1. A **modern ECMAScript runtime**, with ES2022+ syntax and built-ins, executable in a PowerPC big-endian Xbox 360 XEX. Duktape only supports part of ES6 and cannot parse current Google scripts. QuickJS is a research candidate, **not integrated**: Microsoft Visual Studio 2010/XDK lacks many newer C features required by current JS runtimes. Building a Windows-host runtime alone proves nothing about XEX compatibility.
2. A **persistent, standards-oriented HTML5 DOM** with document/window, CSSOM, event dispatch, timers, scripts, script modules, proper node mutation/reflow and load lifecycle. The current bootstrap only handles a narrow set of classic DOM actions.
3. **Web Platform APIs**, including fetch/XHR, URL, Promise, TextEncoder, storage/cookies/same-origin rules, security and request policies. The current TLS client loads HTML/CSS/images but does not provide those APIs to JS.
4. An updated **CSS/HTML rendering engine** supporting actual Flexbox/Grid, font shaping, media and browser viewport behavior instead of the simple litehtml float shim.
5. Real Xbox hardware profiling: memory, large JS execution, TLS capabilities and controller interaction. Windows-host tests are necessary but not a console test.

Google's server may also change responses or require authentication/verification even after these features work.

## Acceptance tests

- On the **physical Xbox** open Google Search, submit `Xbox 360`, and see real result titles and source URLs **from a Google response**, not Bing, fabricated results, another remote browser or a PC bridge.
- Click two different real search results and successfully open their pages; use back/history to return.
- Demonstrate actual script-generated DOM updates on console, with bounded memory and no freeze.
- Verify HTTPS certificate validation, console date, headers/cookies and resource failures.
- Until the first two conditions pass, do **not** describe the Google result page as working.

The existing 0.9.2 notice/opt-in Bing fallback remains clearly labeled for the JS-required response.
