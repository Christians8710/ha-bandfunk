"""Check the compiler's actual ESP8266 HTTP handler stack frame."""
from pathlib import Path
import sys

report=Path(sys.argv[1])
frames=[]
for line in report.read_text().splitlines():
    columns=line.split('\t')
    if 'void sendStatus()' in columns[0]:
        frames.append(int(columns[1]))
if len(frames)!=1 or frames[0]>1024:
    raise SystemExit(f'Unzulaessiger oder fehlender sendStatus-Stackrahmen: {frames}')
print(f'ESP8266 sendStatus: {frames[0]} Byte Stackrahmen (Limit 1024)')
