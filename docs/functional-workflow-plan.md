# Native functional workflow

Continue the approved native Qt Quick Material/MSVC client against Django. No embedded browser.

1. Add regression tests before implementation for server query parameters, cross-page movement, failed submission preservation, successful submit-and-next, real skip payloads, explicit bulk selection, annotation deletion and ground truth.
2. Implement a task workflow API. Keep requests serialized through the existing busy guard. Use internal fetch helpers for chained operations. Preserve edits on failed writes. Bulk actions accept explicit selected IDs only and refresh counters/tasks on success.
3. Wire native controls: server search/status/order, selection and confirmed bulk actions, cross-page arrows, real Skip, optional submit-and-next, annotation management, settings dirty guards and successful-navigation signals.
4. Exercise native UI and real Django with disposable projects only: upload/import, filter/order across 50 tasks, save/next/skip, settings, delete annotation, bulk clear/delete, export. Never alter existing user projects.
5. Review, fix findings, run all required checks, deploy the MSVC executable, and document supported workflows and remaining annotation-type limits.

Draft persistence and additional annotation families are separate work; no inert controls will claim to support them.

## Implementation and verification record

- Added taskworkflow.cpp, server query state/rollback, explicit selection, cross-page navigation and save/skip continuation, annotation delete/ground truth, summary refresh and native controls.
- Settings edits are guarded and disabled during a request; success reloads canonical server values. Project/task routes open after successful API retrieval.
- Regression tests first reproduced missing operations, last-page HTTP 404, ground-truth cache uniqueness, failed next-page navigation, and failed submit-next navigation. Fixes now have executable coverage.
- Independent code review findings were fixed and re-reviewed without remaining important findings in these flows.
- Real Django verification uses disposable projects, imports 53 tasks, clicks native controls, verifies persisted annotations/settings/skip/ground truth, tests cross-page selection and deletion, exports JSON and deletes its own projects.
