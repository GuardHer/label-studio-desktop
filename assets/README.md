# Source assets

Copied from the adjacent Label Studio repository for this native desktop port:

- `logo*.svg`: Label Studio branding from its static assets; SVG `currentColor`
  values resolved for the native light/dark theme.
- `icons/`: `web/libs/ui/src/assets/icons/`; theme colors resolved in SVG copies.
- `bounding-boxes.png`, `polygons.png`: `label_studio/core/static/templates/`.
- `templates/`: local template thumbnails and all 69 YAML template definitions
  from `label_studio/annotation_templates/`, bundled in `catalog.json` by
  `scripts/sync-template-catalog.py`; `sample.jpg` is the upstream sample image.
- `opossum_looking.png`: original Label Studio static illustration.
- `Figtree-*.ttf`: `label_studio/core/static/fonts/`; Figtree by Erik Kennedy,
  licensed under the SIL Open Font License 1.1 (font metadata and OFL.txt).

Label Studio sources: `https://github.com/HumanSignal/label-studio`.
Original repository license and notice: LABEL-STUDIO-LICENSE and
LABEL-STUDIO-NOTICE. Names and branding remain their respective owners'.
