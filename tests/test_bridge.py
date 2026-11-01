"""Protocol and navigation regressions; run with python -m unittest discover -s tests."""
import sys
import threading
import unittest
import urllib.error
import urllib.request
from pathlib import Path
from http.server import HTTPServer

sys.path.insert(0, str(Path(__file__).parents[1] / 'bridge'))
from server import navigation_url, handler_for


class FakeRenderer:
    def __init__(self):
        self.calls = []

    def frame(self):
        return bytes([255, 15, 30, 60]) * (1280 * 560)

    def action(self, params):
        self.calls.append(params)


class ProtocolTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.renderer = FakeRenderer()
        cls.server = HTTPServer(('127.0.0.1', 0), handler_for(cls.renderer, 'a' * 48))
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()
        cls.base = 'http://127.0.0.1:' + str(cls.server.server_port)

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.thread.join()
        cls.server.server_close()

    def get(self, path, token='a' * 48):
        return urllib.request.urlopen(urllib.request.Request(self.base + path,
                                      headers={'X-X360Browser-Token': token}), timeout=5)

    def test_unauthorized_requests_do_not_reach_renderer(self):
        before = len(self.renderer.calls)
        with self.assertRaises(urllib.error.HTTPError) as caught:
            self.get('/action?type=navigate&value=private', 'wrong')
        self.assertEqual(caught.exception.code, 401)
        self.assertEqual(len(self.renderer.calls), before)

    def test_frame_length_and_xbox_byte_order(self):
        with self.get('/frame') as response:
            frame = response.read()
            self.assertEqual(response.headers['Content-Type'], 'application/x-x360-argb')
            self.assertEqual(len(frame), 1280 * 560 * 4)
            self.assertEqual(frame[:4], b'\xff\x0f\x1e\x3c')

    def test_unicode_query_round_trip(self):
        with self.get('/action?type=navigate&value=caf%C3%A9%20xbox') as response:
            self.assertEqual(response.status, 200)
        self.assertEqual(self.renderer.calls[-1]['value'], ['café xbox'])

    def test_navigation(self):
        self.assertEqual(navigation_url('xbox 360'), 'https://www.google.com/search?q=xbox+360')
        self.assertEqual(navigation_url('example.com'), 'https://example.com')
        self.assertEqual(navigation_url('https://example.com/a?q=b'), 'https://example.com/a?q=b')
        for unsafe in ['file:///C:/Windows', 'javascript:alert(1)', 'data:text/html,hi']:
            with self.assertRaises(ValueError):
                navigation_url(unsafe)


if __name__ == '__main__':
    unittest.main()
