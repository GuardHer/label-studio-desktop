# Web reference parity — 2026-10-16

Continue the approved native Qt Quick / Material / MSVC architecture. The running
Django site is the visual reference. Reuse its Figtree fonts, logo and local sample
assets under the original Apache-2.0 notices. Default to the site's light palette,
48 px navigation, compact outlined controls, project cards, full-window creation
dialog and tabular task manager. Keep a dark appearance available.

Native pages: email/password login, Home/Projects, project creation (Project Name,
Data Import, Labeling Setup), project task manager (list/grid, filter, sort,
pagination), import files/URL, project settings (title, description, labeling XML),
account summary and rectangle and polygon annotation. Retain token authentication. All data
operations use the existing Django APIs. Session authentication uses Django's login
form, cookie jar and CSRF headers; no password, session or API token is persisted.
Foreign-origin media uses a separate anonymous network manager.

The creation UI offers a working bounding-box and polygon templates and custom XML. Unsupported
label configurations can be stored but are explicitly protected in the editor.
Only functional actions are shown; broader browser integrations and annotation
types are outside this iteration. Project removal requires explicit confirmation.

Verify session/CSRF, invalid login, project CRUD, import and export via local HTTP
fixtures; retain document and canvas regression tests. Run an opt-in live Django
round trip using process-environment credentials and a disposable project. Build,
deploy with MSVC and compare native screenshots with the visible web reference.

The user's current project uses PolygonLabels. Support polygon percentage points,
vertex editing and whole-region movement, with draft guards before navigation.
Do not change the existing project's configuration or annotations during checks.
