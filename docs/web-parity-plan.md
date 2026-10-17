# Native web parity implementation plan

**Goal:** Match the running Label Studio interface and connect the native project
management and rectangle annotation workflow to Django.

**Architecture:** Extend ApiClient for session authentication and management APIs;
split QML presentation into focused pages while preserving AnnotationDocument and
AnnotationCanvas. Native Qt Quick Controls retain the Material style.

**Tech stack:** C++17, Qt 6.12 MSVC x64, Qt Quick, Qt Network, Qt Test.

- [x] Extend `tests/tst_api.cpp`: require `login(url,email,password)`, CSRF cookies,
  rejection of invalid login, `createProject`, `updateProject`, `importFiles`,
  `importUrl`, `exportProject`, `deleteProject`; first run must fail for absent APIs.
- [x] Implement these APIs in `src/apiclient.h`, `src/apiclient.cpp` and
  `src/projectapi.cpp`. Preserve request timeouts and same-origin restrictions.
- [x] Copy attributed logo/fonts/template assets to `assets/`, register in
  `resources.qrc`; add shared `StudioButton`, `StudioField` and theme tokens.
- [x] Preserve the canvas workbench as `Workspace.qml`; introduce native
  `LoginPage`, `HomePage`, `ProjectPage`, `CreateProjectDialog`, `ImportDialog`
  and `ProjectSettings`. Wire them through `Main.qml` with unsaved-change guards.
- [x] Extend Qt UI tests to exercise sign-in/home/create dialog navigation and
  ensure the original canvas mouse, keyboard and discard protections still pass.
- [x] Add opt-in `tst_live` to verify real login, project creation, image upload,
  drawing result save, reload, export, and cleanup of its disposable project.
- [x] Run `build.ps1 -Test -Deploy`; save screenshots of login/home/create/tasks
  and workbench, inspect them, fix layout issues and document remaining limits.

No Git repository exists in this sibling desktop folder; changes remain local.

- [x] Adapt to the current project's PolygonLabels configuration. Verify native
  mouse drawing/vertex editing, serialization guards, Django save/reload/export.
- [x] Independently review session/project operations and polygon data protection;
  fix duplicate multipart fields and URL form encoding with failing regressions.
