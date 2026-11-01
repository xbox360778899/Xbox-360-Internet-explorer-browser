"""Exercise installed Edge and Chrome without touching personal browser profiles."""
import json
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parents[1] / 'bridge'))
from server import Renderer, WIDTH, HEIGHT
from PIL import Image

output = Path(__file__).parents[1] / 'docs'
reports = []
for engine in ['msedge', 'chrome']:
    renderer = Renderer(engine)
    try:
        assert renderer.page.title() == 'X360Browser — New tab'
        assert len(renderer.frame()) == WIDTH * HEIGHT * 4
        renderer.page.screenshot(path=str(output / f'{engine}-home.png'))
        # Local deterministic interaction verifies actual input and rendered pixel conversion.
        renderer.page.set_content('<html><title>Input test</title><body style="background:rgb(15,30,60);margin:0;height:2000px"><input id="q" style="position:absolute;left:20px;top:20px"/><button id="b" onclick="document.title=\'Clicked\'" style="position:absolute;left:20px;top:70px">Go</button></body></html>')
        renderer.action({'type': ['click'], 'x': ['40'], 'y': ['30']})
        renderer.action({'type': ['text'], 'value': ['Xbox 360 café']})
        assert renderer.page.locator('#q').input_value() == 'Xbox 360 café'
        renderer.action({'type': ['click'], 'x': ['35'], 'y': ['80']})
        assert renderer.page.title() == 'Clicked'
        frame = renderer.frame()
        offset = ((500 * WIDTH) + 1100) * 4
        assert frame[offset:offset + 4] == bytes([255, 15, 30, 60])
        renderer.action({'type': ['scroll'], 'value': ['420']})
        assert renderer.page.evaluate('scrollY') > 0
        renderer.home()
        renderer.action({'type': ['navigate'], 'value': ['https://example.com']})
        assert 'Example Domain' in renderer.page.title()
        renderer.action({'type': ['back']})
        assert renderer.page.title() == 'X360Browser — New tab'
        renderer.action({'type': ['forward']})
        assert 'Example Domain' in renderer.page.title()
        renderer.action({'type': ['navigate'], 'value': ['Xbox 360 homebrew']})
        renderer.page.wait_for_timeout(1200)
        google_captcha = '/sorry/' in renderer.page.url
        reports.append({'browser': engine, 'version': renderer.browser.version,
                        'input': 'passed', 'pixel_order': 'passed', 'scroll': 'passed',
                        'history': 'passed', 'https': 'passed',
                        'google_navigation': 'CAPTCHA shown' if google_captcha else 'Search page loaded'})
    finally:
        renderer.close()
report = {'console_hardware_test': 'not performed', 'browsers': reports}
(output / 'browser-test-results.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report, indent=2))
