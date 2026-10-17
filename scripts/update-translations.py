"""Extract Qt strings and retain dynamic template titles in the translation catalog.

Run with --lupdate /path/to/Qt/bin/lupdate when Qt is not on PATH.
"""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import xml.etree.ElementTree as ET


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lupdate', default=shutil.which('lupdate'))
    args = parser.parse_args()
    if not args.lupdate:
        parser.error('Qt lupdate was not found; provide --lupdate.')
    root = Path(__file__).resolve().parents[1]
    target = root / 'translations/desktop_zh_CN.ts'
    subprocess.run([args.lupdate, 'qml', 'src', '-ts', str(target)], cwd=root, check=True)
    tree = ET.parse(target)
    context = next((c for c in tree.getroot().findall('context')
                    if c.findtext('name') == 'TemplateCatalog'), None)
    if context is None:
        context = ET.SubElement(tree.getroot(), 'context')
        ET.SubElement(context, 'name').text = 'TemplateCatalog'
    messages = {m.findtext('source'): m for m in context.findall('message')}
    catalog = json.loads((root / 'assets/templates/catalog.json').read_text(encoding='utf-8'))
    titles = {template['title'] for template in catalog['templates']}
    for title in sorted(titles):
        if title in messages:
            translation = messages[title].find('translation')
            if translation is not None and translation.get('type') in ('vanished', 'obsolete'):
                translation.attrib.pop('type')
                if not translation.text:
                    translation.set('type', 'unfinished')
        else:
            message = ET.SubElement(context, 'message')
            ET.SubElement(message, 'source').text = title
            ET.SubElement(message, 'translation', type='unfinished')
    for title, message in messages.items():
        if title not in titles:
            translation = message.find('translation')
            if translation is not None:
                translation.set('type', 'vanished')
    # Match lupdate's indentation to keep subsequent catalog diffs readable.
    tree.getroot().text = '\n'
    for item in tree.getroot():
        ET.indent(item, space='    ')
        item.tail = '\n'
    xml = ET.tostring(tree.getroot(), encoding='unicode', short_empty_elements=True).replace(' />', '/>')
    target.write_text('<?xml version="1.0" encoding="utf-8"?>\n<!DOCTYPE TS>\n' + xml + '\n', encoding='utf-8')
    print(f'Retained {len(titles)} dynamic template titles in TemplateCatalog.')


if __name__ == '__main__':
    main()
