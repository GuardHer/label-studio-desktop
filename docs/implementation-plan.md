# Native desktop implementation plan

**Goal:** Deliver a runnable Material Qt Quick client for Label Studio image bounding boxes while retaining Django.

**Architecture:** C++ owns network and annotation state. QML contains native presentation and pointer interaction. The desktop project is a sibling of the existing backend checkout.

**Tech stack:** C++17, Qt >= 6.6.3 (Core, Gui, Network, Quick, QuickControls2, Test), CMake, CTest.

1. [x] Establish CMake targets and failing Qt Test cases for configuration compatibility, bounded rectangles, serialization, history and unsupported-data protection. Files: CMakeLists.txt, tests/tst_document.cpp, src/annotationdocument.{h,cpp}.
2. [x] Implement AnnotationDocument and run its tests. Use normalized percentage coordinates and snapshot history; preserve IDs; reject unsupported controls/results rather than silently filtering them.
3. [x] Add local HTTP integration tests and ApiClient. Files: tests/tst_api.cpp, src/apiclient.{h,cpp}, src/imageprovider.h. Verify Token/JWT, pagination, authenticated media and POST versus PATCH.
4. [x] Create Material interface and native rectangle canvas. Files: src/main.cpp, qml/Main.qml, qml/AnnotationCanvas.qml, qml/ConnectionDialog.qml, qml/Theme.qml. Verify by build and offscreen launch; exercise demo and save/switch safeguards.
5. [x] Add build/run scripts, demo fixture and Chinese README. Run CTest, QML smoke checks and screenshot inspection; record live-backend limitations.

Execution stays in this session. Build artifacts and local build utilities are ignored. Existing Django source is retained in its own checkout.

## 本机验证记录（2026-10-16）

- 用户最终指定：MSVC，Qt Quick Controls Material style。
- Visual Studio 2026 Insiders，MSVC 19.52.37025，Qt 6.12.0 msvc2022_64。
- `build.ps1 -Test -Deploy`：成功；CTest document、api、native_ui 全部通过。
- 界面事件验证：绘制、移动、四角缩放、撤销/重做、脏状态退出确认、跨项目标签同步、连接对话框提交与清空凭证。
- API 验证：Token / JWT、分页、受保护图片、跨源不发送凭证、POST/PATCH、保存失败保留修改、缺失结果保护、JPEG EXIF=6 方向与尺寸。
- 独立代码审查发现 EXIF 解码问题，回归测试先复现 120×80 而非 80×120，现已修复并复审。
- 部署程序在仅保留 Windows 系统 PATH、清除 QML/Qt 插件环境变量后，使用默认 Windows 图形后端启动并生成截图，退出码 0。
- 原 Django 仓库 `git status --short` 为空。没有运行带真实账户凭证的 Django 联调。
