"""Embed the project icon so device UI stays fully local and works offline."""
from pathlib import Path
import html
Import('env')
root=Path(env['PROJECT_DIR']).parent

style=' '.join((root/'web/assets/bandfunk.css').read_text(encoding='utf-8').splitlines())
(root/'firmware/include/ui_style.h').write_text('#pragma once\n#define BANDFUNK_CSS R"BFSTYLE('+style+')BFSTYLE"\n',encoding='utf-8')
data=(root/'custom_components/bandfunk/brand/icon.png').read_bytes()
(root/'firmware/include/app_icon.h').write_text('#pragma once\nconst uint8_t APP_ICON[] PROGMEM = {'+','.join(str(b) for b in data)+'};\n')
sections=[('BandFunk – MIT-Lizenz','LICENSE'),('Drittanbieter und Zuordnung','THIRD_PARTY_NOTICES.md'),('CrowdSync_RF – MIT-Lizenz','licenses/CrowdSync_RF-MIT.txt'),('WiFiManager – MIT-Lizenz','licenses/WiFiManager-MIT.txt')]
page='<section class="card"><h2>Copyright und Lizenzen</h2><p>BandFunk ist ein unabhängiges Projekt. Fremde Produktnamen dienen der technischen Zuordnung.</p>'
for title,name in sections:
    page+='<h3>'+html.escape(title)+'</h3><pre style="white-space:pre-wrap;overflow-wrap:anywhere">'+html.escape((root/name).read_text(encoding='utf-8'))+'</pre>'
page+='</section><p><a href="/">Zurück zur Steuerung</a></p>'
(root/'firmware/include/legal_page.h').write_text('#pragma once\nconst char LEGAL_PAGE[] PROGMEM=R"LEGAL('+page+')LEGAL";\n',encoding='utf-8')
