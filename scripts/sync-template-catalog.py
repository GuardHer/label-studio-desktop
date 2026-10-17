"""Bundle the installed Label Studio template catalogue and its local thumbnails."""
import json
import shutil
import sys
from pathlib import Path
from xml.sax.saxutils import escape
import yaml

backend = Path(sys.argv[1]).resolve()
desktop = Path(__file__).resolve().parents[1]
source = backend / 'label_studio/annotation_templates'
destination = desktop / 'assets/templates'
destination.mkdir(exist_ok=True)
catalog = []
for path in sorted(source.glob('**/*.yml')):
    record = yaml.safe_load(path.read_text(encoding='utf-8'))
    if not isinstance(record, dict) or not isinstance(record.get('config'), str):
        continue
    record['slug'] = path.parent.relative_to(source).as_posix()
    image = backend / 'label_studio/core' / record.get('image', '').lstrip('/')
    if image.is_file():
        shutil.copyfile(image, destination / image.name)
        record['image'] = 'qrc:/templates/' + image.name
    else:
        record['image'] = ''
    catalog.append({k: record.get(k) for k in ('slug', 'title', 'group', 'type', 'order', 'image', 'config')})
groups = source.joinpath('groups.txt').read_text(encoding='utf-8').splitlines()
(destination / 'catalog.json').write_text(json.dumps({'groups': groups, 'templates': catalog}, ensure_ascii=False, indent=2), encoding='utf-8')
sample = backend / 'label_studio/core/static/samples/sample.jpg'
shutil.copyfile(sample, destination / 'sample.jpg')
files = sorted(p.name for p in destination.iterdir() if p.is_file())
qrc = '<RCC><qresource prefix="/templates">\n' + ''.join(f'  <file alias="{escape(p)}">templates/{escape(p)}</file>\n' for p in files) + '</qresource></RCC>\n'
(desktop / 'assets/template-catalog.qrc').write_text(qrc, encoding='utf-8')
print(f'Bundled {len(catalog)} templates in {len(groups)} categories.')
