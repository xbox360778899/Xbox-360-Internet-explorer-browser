"""X360Browser LAN renderer. Browser profiles are isolated from personal profiles."""
import argparse
import hmac
import io
import json
import logging
import socket
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlsplit, quote_plus

from PIL import Image
from playwright.sync_api import sync_playwright

WIDTH, HEIGHT = 1280, 560
HOME = Path(__file__).with_name('home.html')


def navigation_url(text):
    text = text.strip()
    if not text:
        return 'about:blank'
    parsed = urlsplit(text)
    if parsed.scheme in ('http', 'https') and parsed.hostname:
        return text
    if parsed.scheme:
        raise ValueError('Only HTTP and HTTPS addresses are supported.')
    if '.' in text and ' ' not in text:
        return 'https://' + text
    return 'https://www.google.com/search?q=' + quote_plus(text)


class Renderer:
    def __init__(self, engine, headed=False):
        self.runtime = sync_playwright().start()
        self.browser = self.runtime.chromium.launch(channel=engine, headless=not headed)
        self.context = self.browser.new_context(viewport={'width': WIDTH, 'height': HEIGHT},
                                                locale='en-US', accept_downloads=False)
        self.page = self.context.new_page()
        self.context.on('page', self.adopt_page)
        self.error = ''
        self.home()

    def adopt_page(self, page):
        previous = self.page
        self.page = page
        if previous != page and not previous.is_closed():
            previous.close()

    def home(self):
        self.page.goto(HOME.as_uri())

    def action(self, params):
        self.error = ''
        action = params.get('type', [''])[0]
        value = params.get('value', [''])[0]
        try:
            if action == 'navigate':
                self.page.goto(navigation_url(value), wait_until='domcontentloaded', timeout=15000)
            elif action == 'click':
                x, y = int(params['x'][0]), int(params['y'][0])
                if not (0 <= x < WIDTH and 0 <= y < HEIGHT):
                    raise ValueError('Pointer is outside the page.')
                self.page.mouse.click(x, y)
            elif action == 'scroll':
                self.page.mouse.wheel(0, max(-1000, min(1000, int(value))))
                self.page.wait_for_timeout(80)
            elif action == 'text':
                self.page.keyboard.insert_text(value[:1024])
            elif action == 'enter':
                self.page.keyboard.press('Enter')
            elif action == 'back':
                self.page.go_back(wait_until='domcontentloaded', timeout=15000)
            elif action == 'forward':
                self.page.go_forward(wait_until='domcontentloaded', timeout=15000)
            elif action == 'reload':
                self.page.reload(wait_until='domcontentloaded', timeout=15000)
            elif action == 'home':
                self.home()
            else:
                raise ValueError('Unknown action.')
        except Exception as exc:
            self.error = str(exc).splitlines()[0][:180]
            raise ValueError(self.error) from exc

    def frame(self):
        png = self.page.screenshot(type='png', timeout=8000)
        image = Image.open(io.BytesIO(png)).convert('RGBA')
        if image.size != (WIDTH, HEIGHT):
            raise ValueError('Unexpected browser viewport.')
        # Network bytes are A,R,G,B, matching big-endian Xbox uint32_t pixels.
        r, g, b, a = image.split()
        return Image.merge('RGBA', (a, r, g, b)).tobytes()

    def close(self):
        self.context.close()
        self.browser.close()
        self.runtime.stop()


def handler_for(renderer, token):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_):
            pass  # Never log address queries or the authentication header.

        def respond(self, status, data, content_type='text/plain'):
            self.send_response(status)
            self.send_header('Content-Type', content_type)
            self.send_header('Content-Length', str(len(data)))
            self.send_header('Cache-Control', 'no-store')
            self.end_headers()
            try:
                self.wfile.write(data)
            except (BrokenPipeError, ConnectionResetError):
                pass

        def do_GET(self):
            if not hmac.compare_digest(self.headers.get('X-X360Browser-Token', ''), token):
                return self.respond(401, b'Invalid bridge token.')
            if len(self.path) > 8192:
                return self.respond(414, b'Request too long.')
            route = urlsplit(self.path)
            try:
                if route.path == '/health':
                    return self.respond(200, b'X360Browser/0.1.0')
                if route.path == '/frame':
                    return self.respond(200, renderer.frame(), 'application/x-x360-argb')
                if route.path == '/action':
                    renderer.action(parse_qs(route.query, keep_blank_values=True))
                    return self.respond(200, b'OK')
                if route.path == '/state':
                    data = json.dumps({'url': renderer.page.url, 'title': renderer.page.title(),
                                       'error': renderer.error}).encode()
                    return self.respond(200, data, 'application/json')
                self.respond(404, b'Unknown route.')
            except Exception as exc:
                self.respond(502, str(exc).splitlines()[0][:180].encode('utf-8'))
    return Handler


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--browser', choices=['msedge', 'chrome'], default='msedge')
    parser.add_argument('--bind', default='0.0.0.0')
    parser.add_argument('--port', type=int, default=36080)
    parser.add_argument('--config', type=Path, default=Path(__file__).parents[1] / 'Release/browser.ini')
    parser.add_argument('--headed', action='store_true')
    args = parser.parse_args()
    config = dict(line.strip().split('=', 1) for line in args.config.read_text().splitlines()
                  if '=' in line and not line.startswith('#'))
    token = config['token']
    if len(token) < 32:
        parser.error('Run scripts/configure.py to generate a bridge token.')
    renderer = Renderer(args.browser, args.headed)
    server = HTTPServer((args.bind, args.port), handler_for(renderer, token))
    server.timeout = 1
    print(f'X360Browser: {args.browser}, port {args.port}. Keep this window open.', flush=True)
    print('Use only on your trusted home network. Press Ctrl+C to stop.', flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
        renderer.close()


if __name__ == '__main__':
    main()
