"""Embed the project icon so device UI stays fully local and works offline."""
from pathlib import Path
Import('env')
root=Path(env['PROJECT_DIR']).parent
data=(root/'custom_components/bandfunk/brand/icon.png').read_bytes()
(root/'firmware/include/app_icon.h').write_text('#pragma once\nconst uint8_t APP_ICON[] PROGMEM = {'+','.join(str(b) for b in data)+'};\n')
