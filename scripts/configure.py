"""Generate local console settings without committing private LAN configuration."""
from pathlib import Path
import secrets
import socket

root = Path(__file__).parents[1]
settings = root / 'Release/browser.ini'
settings.parent.mkdir(exist_ok=True)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
try:
    sock.connect(('192.0.2.1', 80))
    host = sock.getsockname()[0]
except OSError:
    host = input('Enter the PC IPv4 address: ').strip()
finally:
    sock.close()
if not settings.exists():
    settings.write_text(f'host={host}\nport=36080\ntoken={secrets.token_hex(24)}\n', encoding='ascii')
print(f'Console configuration: {settings}')
print(f'PC address: {host}. Copy browser.ini beside default.xex on the Xbox.')
