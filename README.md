# Label Studio 原生 QML 桌面端

使用 **C++17 + Qt Quick + Qt Quick Controls Material style**，通过 REST API 连接现有 Label Studio Django 后端。界面和图片标注画布均为原生 Qt Quick。

本版本根据运行中的 Label Studio 网页重建登录、账户与组织、首页、项目卡片、创建项目、任务列表/网格、项目设置和图片矩形/多边形标注工作区，使用原站 logo、Figtree 字体及图标，并调整颜色、间距与布局。尚未覆盖 Label Studio 的全部标注类型与管理功能。

## 直接运行

本机已生成包含 Qt 运行库的程序目录：

`E:\projects\QML\label-studio-desktop\build-msvc\dist\LabelStudioDesktop.exe`

双击运行，在登录页输入 Django 地址、邮箱和密码。也可点击 “Open example” 体验离线示例，示例修改仅保存在当前会话。

登录页右上角、登录后顶部工具栏均提供 **简体中文 / English** 选择框。切换立即生效并记住选择，下次启动自动恢复；首次启动跟随系统语言。切换保留当前页面、输入内容、未保存标注和撤销历史。项目名称、标签、XML 配置、文件路径及服务端返回的原始诊断文本保持原文，已显示的临时通知在下一条消息到来时更新语言。

也可在本目录运行：

```powershell
.\run.ps1 -Demo
```

## 构建

已在本机验证 **Visual Studio 2026 Insiders / MSVC 19.52 + Qt 6.12.0 msvc2022_64 + Windows SDK 10.0.26100.0**。

```powershell
.\build.ps1 -Test -Deploy
```

脚本通过 `vswhere -prerelease` 自动发现包括 Insiders 在内的 Visual Studio，使用 x64 MSVC 和 Visual Studio 自带的 CMake/Ninja。默认 Qt 路径为 `E:\Qt\Qt6.12\6.12.0\msvc2022_64`，可传入 `-QtRoot` 指定其他 MSVC x64 套件，也可用 `-VisualStudio` 显式指定 VS 安装目录。

工程最低要求 Qt 6.6.3，当前仅在 Qt 6.12 上实际编译运行。请勿在同一构建目录混用 MinGW、ARM64 和 MSVC 套件。Qt Creator 中打开 `CMakeLists.txt` 并选择对应 MSVC x64 Kit 也可构建。

`-Deploy` 使用 `windeployqt` 生成 `build-msvc/dist`。分发时应复制整个目录，目标机器需要兼容的 Microsoft Visual C++ x64 运行库。

## 连接 Django

1. 启动现有 Django 后端，本机地址为 `http://localhost:8080`。
2. 在桌面登录页填写地址、邮箱和密码，点击 **Log in**。
3. 从 Home 或 Projects 打开项目，或使用 **Create Project** 创建项目。
4. 使用 **Import** 上传图片/JSON/CSV 或导入数据集 URL，打开任务后绘制矩形或多边形，点击 **Submit / Update** 保存，勾选 **Next after submit** 后自动继续；**Skip** 记录真正的跳过；**Export** 导出 Label Studio JSON。

邮箱密码通过 Django 原生登录表单交换会话，写请求携带 CSRF。也支持传统 `Authorization: Token …`、JWT Refresh Token、JWT Access Token（登录页的 API token 入口）。密码、Token 和会话 Cookie 仅保留在进程内存，不写入配置文件；记住服务器地址、主题及显式选择的界面语言。当前版本由用户独立启动 Django 服务。

桌面端直接连接 Django API，不依赖 Vite。开发网页时 `8010` 是前端资源/HMR 端口，完整网页入口仍为 `8080`，见原仓库 `scripts/run-backend-dev.ps1` 与 `web/README.md`。

任务列表使用 `/api/tasks/?project=…&page=…&page_size=…&query=…` 进行服务端筛选排序，支持页大小选择，兼容该仓库返回的 `tasks` 和 `total` 字段。已有标注使用 `PATCH /api/annotations/{id}/`，新标注使用 `POST /api/tasks/{id}/annotations/`。

## 支持范围

- 邮箱密码 / Token / JWT 登录、原生注册、Django 会话退出。
- 个人资料、头像、修改密码、邮件偏好、成员资格和贡献概要。
- 组织信息、成员分页/详情/移除、邀请链接复制及重置。
- PAT 创建/轮换/撤销、Legacy 加载/重置、组织令牌策略；账户和项目快捷键配置、JSON 导入/导出/恢复。详细范围见 [账户与组织交付记录](docs/account-organization.md)。
- Home 持久化最近访问项目；Projects 搜索、五种排序、完整分页目录及卡片快捷菜单；三步创建支持未配置标注界面的空项目。
- 项目名称/描述/颜色/标注配置及顺序/随机采样设置；后端采样队列、历史导航、提交或跳过后继续。
- 二次确认删除项目、重置标签缓存、丢弃后端全部数据管理标签页。详细范围及 F019–F032 对照见[项目管理交付记录](docs/project-management.md)。
- 多文件上传、URL 数据集导入、JSON 标注导出。
- 任务分页、逐字段同源鉴权缩略图、列表/网格；动态列、字段排序、AND/OR 类型筛选、筛选 JSON 与固定侧栏、Django 保存标签页、列宽/顺序/密度。
- 全结果集选择与排除、按标注人删除、检索预测、预测转标注、去重及所选任务采样队列；并排任务快速查看、原始 JSON 和详情。
- 69 个本机源码模板、11 个分类、搜索/缩略图；XML/可视化标签编辑、颜色/alias/hotkey/数据映射、语法高亮及补全、Django 验证和真实原生预览。逐项状态见[标注配置与数据管理器交付记录](docs/labeling-data-manager.md)。
- 跨分页前后切换，提交/更新并继续；跳过通过 `was_cancelled=true` 保存到 Django，不覆盖已有标注。
- 勾选当前页或单条任务，跨页保留选择；二次确认批量删除任务、清空标注/草稿、清空预测。筛选变化会清空选择；请求失败保留选择。
- 删除单份标注、设置/取消 Ground truth，同步 Django 的单任务 Ground truth 唯一性。
- 标注或设置的未保存修改确认；保存中禁止继续编辑设置，服务端成功后读取保存后的值。
- 删除/提交使末页失效时自动回到有效页码；跨页加载失败保留当前任务并允许重试。
- 多图片/图片列表、多控件的矩形框绘制、选择、移动、四角缩放、标签修改和删除。
- 多边形逐点绘制、点击首点或 Enter 闭合、顶点编辑、整体移动、改标签与删除；Esc 取消草稿。
- 图片缩放、平移，标注读取与提交，撤销和重做。
- 多份标注选择，未保存修改确认，保存失败保留修改。
- 同源受保护图片鉴权；跨源图片请求不附带后端凭证。
- Material 浅色/深色主题。
- 简体中文/英文即时切换、语言偏好记忆及本地化日期显示。

标注配置支持多个 Image、RectangleLabels/PolygonLabels，以及 View、Header、Text、Choices、TextArea、Rating、Number 的原生子集。基础配置例如：

```xml
<View>
  <Image name="image" value="$image"/>
  <RectangleLabels name="bbox" toName="image">
    <Label value="Car" background="#8B7EFF"/>
    <Label value="Person" background="#F5B85A"/>
  </RectangleLabels>
</View>
```

图片字段名会从配置解析；标注按 Label Studio 的百分比坐标保存。支持矩形和多边形混用、动态标签/alias、必填/数值范围校验及多项文本输入。旋转矩形、未知控件/语义、重复区域 ID、越界坐标等不支持的数据会触发整份标注的编辑保护。任意 CSS、条件显隐/perRegion、像素掩膜分割、音视频、完整通用标签引擎、云存储管理、评论/历史、草稿持久化和后台进程打包尚未实现。模板目录完整不代表其全部标注类型可原生执行。上传批次上限为 64 MB。

## 快捷键

以下为默认值。账户设置可修改已支持的原生动作，并与 Django 中的账户默认配置及个人项目覆盖同步；网页其他动作的配置会保留。

| 操作 | 快捷键 |
|---|---|
| 绘制形状 / 选择 / 平移 | R / V / H |
| 多边形闭合 / 取消 | Enter 或点击首点 / Esc |
| 缩放 / 平移 | 滚轮 / 鼠标中键 |
| 保存 | Ctrl+S |
| 撤销 / 重做 | Ctrl+Z / Ctrl+Shift+Z 或 Ctrl+Y |
| 删除选中区域 | Delete |

## 验证与代码

`build.ps1 -Test` 执行 Qt Test 的 language、document、api、native_ui、live_django 五组测试，其中 live_django 默认跳过。API 测试用进程内 HTTP 服务验证会话/CSRF、令牌、分页、图片与缩略图凭据隔离、批量导入、URL 表单编码、设置及保存。界面测试通过鼠标和键盘检查画框、移动、缩放、撤销及未保存保护；语言测试检查即时切换、重启记忆、标准按钮和本地错误提示翻译，以及输入、配置、标注、下拉选择的保留。

项目管理可运行 `.\tests\run-project-live.ps1`：自动建立并清理独立 QA 组织，验证空项目、颜色/配置、顺序/随机采样、历史中提交、跳过、缓存与标签页维护，并直接检查 Django 数据库。账户回归使用 `.\tests\run-account-live.ps1`，两者均自动生成临时凭据。

配置和数据管理器使用 `.\tests\run-editor-data-live.ps1`，建立隔离组织和 65 条任务，验证组合筛选、保存视图、批量操作/队列，以及文本、多控件、多图片配置的保存重载；核验数据库后自动清理。模型检索已接通实际动作，但当前没有模型服务，未验证推理结果。

真实 Django 联调已验证：从 QML 登录、创建临时项目、多文件上传、保存矩形、重载并导出 JSON；导入至 53 条任务，验证服务端搜索/筛选/排序、跨页切换、提交并继续、跳过持久化、设置保存、Ground truth 和删除单份标注；通过原生鼠标点击复选框，跨页选择并确认批量清空/删除，验证末页失效恢复。另建临时多边形项目验证保存、重载和导出。测试只删除自己创建的临时项目。需要重跑时在当前进程设置 `QML_LIVE_EMAIL`、`QML_LIVE_PASSWORD`，可选 `QML_LIVE_SERVER` 与 `QML_LIVE_SCREENSHOTS`，然后执行 `build-msvc/tst_live.exe -o build-msvc/live-results.txt,txt`，结束后移除凭据环境变量。不要将实际凭据写入脚本。

构建脚本自动探测本机 MSVC 的 `/showIncludes` 语言，确保 Ninja 正确追踪头文件依赖。

真实联调可额外设置 `QML_LIVE_LANGUAGE=zh_CN` 使用中文运行，设置 `QML_LIVE_SCREENSHOTS` 后会保存两种语言、常规和最小窗口尺寸的页面截图。翻译更新方法见 [translations/README.md](translations/README.md)，Qt 套件需安装 Linguist Tools 和中文 Qt 翻译目录。

- `src/annotationdocument.*`：配置、结果 JSON、矩形编辑和历史。
- `src/apiclient.*`、`src/projectapi.cpp`、`src/projectmanagement.cpp`、`src/projectcatalog.*`、`src/taskworkflow.cpp`：网络、认证、项目目录与设置、采样、导入导出、图片、任务筛选/选择/切换和标注操作。
- `qml/Main.qml`：页面导航；`qml/Workspace.qml`：工作区；`qml/AnnotationCanvas.qml`：原生画布。
- `qml/Theme.qml`：统一主题；控件外观由 Qt Material style 提供。
- `docs/web-parity-design.md`、`docs/web-parity-plan.md`：网页对照设计与实施记录。

原站 logo、图标、模板预览、Figtree 字体来源见 `assets/README.md`，Label Studio 许可证及通知随资源附在 `assets/`。Qt 运行库及插件适用各自许可证。
