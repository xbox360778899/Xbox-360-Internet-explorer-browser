"""Strict acceptance check: did Google deliver actual search result titles and links?

This is a development diagnostic for the native-console browser, NOT an API,
proxy, scraper fallback, or replacement for running JS/DOM on Xbox hardware.
Exit 0 only if the fetched Google HTML contains multiple actual linked results.
"""
import html
import sys
import urllib.parse
import urllib.request
from html.parser import HTMLParser

SEARCH_URL = "https://www.google.com/search?q=Xbox%20360"
USER_AGENT = "X360Browser/0.10.0 (Xbox 360; NativeBrowser)"


class GoogleResultParser(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.href = ""
        self.in_anchor = False
        self.heading_depth = 0
        self.heading = ""
        self.results = []

    def handle_starttag(self, tag, attrs):
        attributes = dict(attrs)
        if tag == "a":
            self.in_anchor = True
            self.href = attributes.get("href", "")
            self.heading = ""
        elif tag == "h3" and self.in_anchor:
            self.heading_depth += 1

    def handle_data(self, data):
        if self.in_anchor and self.heading_depth:
            self.heading += data

    def handle_endtag(self, tag):
        if tag == "h3" and self.heading_depth:
            self.heading_depth -= 1
        elif tag == "a" and self.in_anchor:
            candidate = urllib.parse.urljoin("https://www.google.com/", self.href)
            parsed = urllib.parse.urlparse(candidate)
            if parsed.path == "/url" and parsed.hostname == "www.google.com":
                params = urllib.parse.parse_qs(parsed.query)
                candidate = params.get("q", [""])[0]
                parsed = urllib.parse.urlparse(candidate)
            if (
                self.heading.strip()
                and parsed.scheme in ("http", "https")
                and parsed.hostname
                and not parsed.hostname.endswith(".google.com")
                and parsed.hostname != "google.com"
            ):
                self.results.append((html.unescape(self.heading.strip()), candidate))
            self.in_anchor = False
            self.heading = ""


def main():
    req = urllib.request.Request(
        SEARCH_URL,
        headers={
            "User-Agent": USER_AGENT,
            "Accept": "text/html,application/xhtml+xml",
            "Accept-Language": "en-US,en;q=0.9",
        },
    )
    try:
        with urllib.request.urlopen(req, timeout=15) as response:
            payload = response.read(1024 * 1024 + 1)
            if len(payload) > 1024 * 1024:
                print("FAIL: Google returned more than the 1 MiB HTML limit.")
                return 1
            print(f"HTTP={response.status} BYTES={len(payload)}")
    except Exception as exc:
        print(f"FAIL: Google network request failed: {exc}")
        return 1

    text = payload.decode("utf-8", "replace")
    if "/httpservice/retry/enablejs" in text:
        print("FAIL: Real Google responded with a JavaScript-required interstitial, not results.")
        return 1
    if "Your browser is no longer supported" in text or "Please update your browser" in text:
        print("FAIL: Real Google rejected this browser as unsupported.")
        return 1

    parser = GoogleResultParser()
    parser.feed(text)
    unique = list({url: (title, url) for title, url in parser.results}.values())
    if len(unique) < 3:
        print(f"FAIL: only {len(unique)} external links with real Google result headings.")
        return 1
    for title, url in unique[:3]:
        print(f"RESULT: {title[:100]} -> {url[:180]}")
    print("PASS: Google returned at least 3 actual titled result links.")
    print("NOTE: still requires console DOM/JS render and physical Xbox verification.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
