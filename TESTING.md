# Xbox 360 test guide

## PC setup

1. Install Python 3.11+ and Microsoft Edge or Google Chrome.
2. Open a terminal in the extracted project folder.
3. Run `python -m pip install -r bridge/requirements.txt`.
4. Run `python scripts/configure.py`.
5. Start `start-edge.cmd` first. To try Chrome, stop Edge's service with Ctrl+C and run `start-chrome.cmd`.

## Console setup

Copy `Release/default.xex` and the newly generated `Release/browser.ini` into the same folder on a USB drive or HDD. Run the XEX with Aurora. Keep the PC bridge running on the same network.

Press Y and enter `Xbox 360 homebrew`. Confirm the keyboard. Verify Google loads; it may show a cookie or verification prompt. Use A to click, X to type into a focused page field, START for Enter, and triggers to scroll.

## Report a problem

Include the on-screen status message, Edge/Chrome selection, whether the home page appears, whether the pointer moves, and a photo if the console screen is blank. Never post your `browser.ini` token.

| Message | Check |
|---|---|
| PC not reachable | PC bridge running, PC IPv4 in browser.ini, same network, TCP 36080 firewall |
| Bridge HTTP 401 | Copy the same generated browser.ini to the Xbox |
| Bridge HTTP 502 | Website timed out, unsupported navigation, or browser process stopped |
| Incomplete page frame | Ethernet connection and PC bridge status |
| browser.ini is missing | Generate settings, then copy beside default.xex and restart |

Physical Xbox 360 testing has not been performed during development. This is a test build, not a confirmed working hardware release.
