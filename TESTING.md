# Native Xbox 360 test guide - 0.2.0

Copy only default.xex to the console. No PC service or INI is required.

1. Open the native home page while the PC bridge is stopped.
2. Move the pointer with the left stick and click the address field with A.
3. Enter https://example.com/ and check that readable text appears.
4. Press Y for an HTTPS address or Bing query; X starts a Google query.
5. Click a link with A. Check B/LB back, RB forward and trigger scrolling.
6. Click the toolbar, or press L3 and use D-pad/A.

The reader does not execute JavaScript or reproduce CSS layouts. Search providers can return verification pages. This is a native engine compatibility limit, not a PC requirement.

| Failure | Report |
|---|---|
| Network startup | Both startup numbers |
| Connection | URL, step and hexadecimal error |
| TLS | TLS number, network error, console date/time |
| HTTP | Status and address |
| Empty page | Whether the site requires JavaScript or verification |

Physical USB mouse support is not implemented; the mouse pointer uses the gamepad. A physical Xbox test has not been performed during development.
