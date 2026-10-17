# Label Studio → 原生 QML 功能核查清单

> 本文件及同名 CSV/JSON 保留首次核查时的历史状态和统计。F001–F018 最新状态见[账户与组织交付记录](account-organization.md)；F019–F032 见[项目管理交付记录](project-management.md)；F033–F080 见[标注配置与数据管理器交付记录](labeling-data-manager.md)。

核查日期：2026-10-16。本机网页版本 **1.24.0.dev0**；桌面端为 `E:\projects\QML\label-studio-desktop` 当前源码和 `build-msvc/dist` 实现。此次仅核查和编写清单，没有补做业务功能。

当前桌面端是**单图片矩形/多边形标注的可用子集**，远未达到整个 Label Studio 前端的功能对等。它已接通部分真实 Django 流程，但主要缺口仍包括账户组织管理、通用标注引擎、多类型标注、完整数据管理、导出格式、存储、模型预测、Webhook 和高级编辑器工作流。

## 核查范围与状态口径

以用户指定的[添加用户文档](https://labelstudio.com.cn/guide/signup)为入口，读取该站 Guide 导航列出的功能文档、版本对照、企业功能文档、标签目录及模板分类页；对照已登录的 [本机网页](http://localhost:8080/)、原生 QML/C++ 源码和既有测试记录。覆盖功能模块和目录级功能项，**不把每个 XML 属性、API 参数、Webhook 事件或 Helm 配置键另拆成一个功能**。未来新增、未出现在此次目录中的功能不在本清单内。

本轮实际浏览：项目列表、项目 3 数据管理器、筛选字段和任务操作菜单、导入/导出弹窗、全部八个项目设置分区、账户资料/快捷键入口、组织成员与令牌策略、任务 20 标注工作区。没有提交设置、删除数据、创建邀请、生成令牌或提交标注；临时查看用的空筛选条件与任务勾选已撤销。

| 状态 | 含义 |
|---|---|
| 已实现 | 有原生 QML 交互及实际 C++/Django 路径；仅针对该行明确限定范围。 |
| 部分实现 | 只实现子集，不能视为该模块整体完成。 |
| 未实现 | 当前无对应原生页面、控件语义、状态或 API 操作。 |
| 仅网页跳转 | 可打开外部网页，**不计为 QML 已实现**。 |
| 仅后端已有 | 保留 Django/SDK 能力，不代表桌面已经提供 UI。 |
| 未验证 / 版本受限 | 说明证据边界；本机出现入口也不自动等同商业后端能力完整可用。 |

本清单分为：**207 项主清单、51 个标签、86 个模板、49 项商业能力对照、12 项后端/部署边界**。它们存在语义重叠，不能直接相加计算产品完成率。主清单包含桌面增强；统计为 已实现 51 项、部分实现 21 项、未实现 124 项、仅网页跳转 9 项、仅后端已有 1 项、未验证 1 项。

## 最重要的实际差异

1. **项目设置：网页 8 个分区，QML 只有 3 个分区且均非完整覆盖。** 缺 Annotation、Model、Predictions、Cloud Storage、Webhooks，General 缺颜色/采样，Danger Zone 缺 Reset Cache/Drop All Tabs。
2. **配置可编辑不等于能渲染。** `AnnotationDocument::configure` 要求一个 Image、一个 RectangleLabels 或 PolygonLabels，结果必须符合相应单标签结构；多图片、多控件、分类、OCR、文本、音视频等不能原生标注。
3. **模板预览为示例图。** 模板分类大多是无切换行为的文字；只有矩形与多边形两类基础模板。编辑任意 XML 不会产生对应真实预览。
4. **预测数不等于预测功能。** 可显示数量、批量删除预测，但 `fetchTask` 没有加载 predictions 到画布，也不能接受模型建议、选择预测版本或连接模型。
5. **数据管理远未完整。** 固定 Default 标签外观、固定查询、ID 排序、两项额外列；没有保存视图、复杂筛选、任意排序、选中任务标注队列与全结果集全选。
6. **账户和邀请主要是网页跳转。** 没有原生注册、成员列表、邀请链接生命周期、个人资料编辑或 Token 管理。
7. **本地撤销不是服务器历史。** 没有草稿自动保存、跨重启恢复、历史版本/审核/评论工作流；标签数字也未绑定数字快捷键。
8. **导出目前只有 JSON。** 不是“已支持网页导出模块”；COCO、YOLO、VOC、CSV 等格式与媒体打包都缺少。

## 主功能清单

### 账户与组织

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| F001 | 邮箱密码登录 | 已实现 | Django 会话登录和 CSRF；已有真实登录联调记录。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F002 | API Token / JWT 登录 | 已实现 | 支持旧 Token、JWT Refresh 交换和 Access Token；不等于支持令牌管理。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F003 | 退出登录 | 部分实现 | 清空桌面进程 Cookie 和凭据；未调用 Django logout 注销服务器会话。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F004 | 新用户注册 | 未实现 | 没有注册页面或注册 API 调用。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F005 | 成员列表 | 仅网页跳转 | QML 无组织成员页面，入口打开 organization 网页。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F006 | 查看成员活动与贡献 | 未实现 | 没有成员详情、最近活动和贡献统计页面。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F007 | 邀请成员与复制邀请链接 | 仅网页跳转 | Invite Members 打开说明框，再跳转组织网页；无原生获取/复制链接操作。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F008 | 重置邀请链接 | 未实现 | 没有邀请链接重置接口。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F009 | 组织名称和组织信息 | 未实现 | 只有账户邮箱及服务器地址概要，不属于组织信息；无原生组织信息页。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F010 | 修改名字、姓氏、电话 | 仅网页跳转 | 账户页面只读邮箱/服务器；资料修改须到网页。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F011 | 上传与修改头像 | 仅网页跳转 | 显示邮箱首字母圆标，未实现头像上传。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F012 | 修改账户密码 | 未实现 | 没有原生修改密码入口或对应 API 调用；本机网页是否提供单独入口需另核对。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F013 | 邮件偏好设置 | 仅网页跳转 | 本机 Account 有 Email Preferences，QML 没有对应表单。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F014 | 成员资格和贡献概要 | 仅网页跳转 | 本机 Membership Info 页面没有原生对应页面。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F015 | 创建、查看、撤销个人访问令牌 | 仅网页跳转 | 桌面能使用现有令牌，不能管理令牌生命周期。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F016 | 续期/重新生成传统令牌 | 未实现 | 本机组织当前禁用 Legacy Tokens；QML 也无管理接口。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F017 | 组织级允许 PAT/Legacy 策略 | 未实现 | 本机 API Tokens Settings 对话框可见，QML 未接入。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |
| F018 | 快捷键自定义、导入、导出、恢复 | 仅网页跳转 | 本机 Hotkeys Beta 页可见，QML 仅提供固定快捷键。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/signup)；qml/LoginPage.qml; qml/Main.qml; src/projectapi.cpp |

### 项目管理

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| F019 | 首页与项目卡片列表 | 已实现 | 读取真实项目；显示描述、任务/标注/跳过/预测数量。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup_project)；qml/HomePage.qml; qml/ProjectsPage.qml; qml/CreateProjectDialog.qml; src/projectapi.cpp |
| F020 | 刷新项目和打开项目 | 已实现 | 调用项目及任务 API；项目列表由客户端遍历服务端分页获取。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup_project)；qml/HomePage.qml; qml/ProjectsPage.qml; qml/CreateProjectDialog.qml; src/projectapi.cpp |
| F021 | 项目创建三步流程 | 已实现 | 名称/描述、导入、标注设置三步；创建后可继续导入。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup_project)；qml/HomePage.qml; qml/ProjectsPage.qml; qml/CreateProjectDialog.qml; src/projectapi.cpp |
| F022 | 只填写名称创建空项目 | 部分实现 | QML 总是附带默认矩形配置，不能还原网页中完全未配置标注界面的空项目状态。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup_project)；qml/HomePage.qml; qml/ProjectsPage.qml; qml/CreateProjectDialog.qml; src/projectapi.cpp |
| F023 | 修改项目名称与描述 | 已实现 | PATCH 项目，保存成功后刷新；失败保留编辑。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup_project)；qml/HomePage.qml; qml/ProjectsPage.qml; qml/CreateProjectDialog.qml; src/projectapi.cpp |
| F024 | 修改项目颜色 | 未实现 | 能显示服务端返回的 color，但无颜色选择或更新操作。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup_project)；qml/HomePage.qml; qml/ProjectsPage.qml; qml/CreateProjectDialog.qml; src/projectapi.cpp |
| F025 | 项目卡片快捷菜单 | 未实现 | QML 卡片只有打开行为；设置/删除需进入项目。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup_project)；qml/HomePage.qml; qml/ProjectsPage.qml; qml/CreateProjectDialog.qml; src/projectapi.cpp |
| F026 | 删除项目 | 已实现 | 原生二次确认后删除；已有临时项目联调验证。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup_project)；qml/HomePage.qml; qml/ProjectsPage.qml; qml/CreateProjectDialog.qml; src/projectapi.cpp |
| F027 | 项目搜索与自定义排序 | 未实现 | 无项目级搜索/排序控件；网页当前是否提供取决于版本，非已确认本机入口。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup_project)；qml/HomePage.qml; qml/ProjectsPage.qml; qml/CreateProjectDialog.qml; src/projectapi.cpp |
| F028 | 最近项目排序 | 部分实现 | 首页标题是 Recent Projects，但直接使用项目返回列表，未维护按最近访问排序。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup_project)；qml/HomePage.qml; qml/ProjectsPage.qml; qml/CreateProjectDialog.qml; src/projectapi.cpp |
| F029 | 顺序任务采样设置 | 未实现 | 无 sampling 设置；桌面按自己的任务列表顺序打开，未接入服务端采样队列。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup_project)；qml/HomePage.qml; qml/ProjectsPage.qml; qml/CreateProjectDialog.qml; src/projectapi.cpp |
| F030 | 随机任务采样设置 | 未实现 | 网页 General 有 Random sampling，QML 未读取/修改并执行该策略。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup_project)；qml/HomePage.qml; qml/ProjectsPage.qml; qml/CreateProjectDialog.qml; src/projectapi.cpp |
| F031 | 重置项目缓存 | 未实现 | 本机 Danger Zone 明确有 Reset Cache；QML 无入口。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup_project)；qml/HomePage.qml; qml/ProjectsPage.qml; qml/CreateProjectDialog.qml; src/projectapi.cpp |
| F032 | 丢弃全部数据管理标签页 | 未实现 | 本机 Drop All Tabs 可见，QML 无操作。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup_project)；qml/HomePage.qml; qml/ProjectsPage.qml; qml/CreateProjectDialog.qml; src/projectapi.cpp |

### 标注配置与模板编辑

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| F033 | XML 配置编辑并保存到 Django | 已实现 | 可编辑 XML 字符串并提交项目；本地能标注的类型仍受解析器限制。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |
| F034 | 矩形与多边形基础模板 | 已实现 | 只有两类实际可用模板；完整模板目录见附录。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |
| F035 | 完整模板库及分类切换 | 部分实现 | 显示多类模板文字，但只实现两张计算机视觉模板卡片，分类文字无切换行为。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |
| F036 | 可视化批量输入标签名称 | 部分实现 | 按行输入标签并生成矩形/多边形 XML；不能完整反向编辑任意现有配置。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |
| F037 | 标签颜色选择器 | 未实现 | 自动分配固定颜色；可通过 XML 写 background，未提供颜色选择器。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |
| F038 | 自定义数据字段映射 | 部分实现 | XML 可手动修改 Image value/name/toName，能解析单一直接任务字段；无可视化映射向导。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |
| F039 | 配置 XML 语法高亮与自动补全 | 未实现 | Consolas 文本框，没有标签/属性补全。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |
| F040 | 真实配置预览 | 未实现 | Preview 显示固定示例图片，不能按当前 XML 和任务实时渲染。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |
| F041 | 通用 XML 对象/控制/视觉标签引擎 | 未实现 | 不是通用 XML 渲染器；仅提取 Image、一个形状控件和静态 Label。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |
| F042 | 一个任务混用矩形和多边形控件 | 未实现 | configure 要求 controls == 1。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |
| F043 | 多图片及图片列表 | 未实现 | configure 要求 images == 1，拒绝 valueList/selectionEnabled。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |
| F044 | XML 的布局、Header、Text、Style 渲染 | 未实现 | 这些标签部分被白名单接受，但对应布局/文字/CSS 内容被忽略。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |
| F045 | 动态标签与 Label alias | 未实现 | 只能读静态 Label value，alias 明确拒绝。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |
| F046 | 标签 hotkey 属性与数字快捷键 | 未实现 | 标签显示 1/2 等数字，但没有对应 Shortcut；hotkey 属性也未解析。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |
| F047 | 必填、条件显隐、范围限制等配置语义 | 未实现 | 未实现 required、visibleWhen、perRegion 等通用属性语义。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |
| F048 | 配置兼容性与不支持数据保护 | 部分实现 | 不支持的标签/旋转/结果会禁用编辑；部分被忽略的属性不会被完整检出，不能视为全兼容校验。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/setup)；qml/LabelingSetup.qml; qml/ProjectSettings.qml; src/annotationdocument.cpp |

### 数据管理器

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| F049 | 任务列表与网格 | 已实现 | 真实 API 任务、鉴权缩略图，两种显示方式。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F050 | 任务基础统计列 | 已实现 | ID、完成、标注数、跳过数、预测数和标注人；不代表能编辑预测。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F051 | 服务端分页 | 已实现 | 固定每页 50，支持翻页及末页失效恢复。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F052 | 自选每页条数 | 未实现 | 没有页大小选择器。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F053 | 任务文件名搜索 | 已实现 | 服务端对配置的 image 字段 contains 查询。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F054 | 数字任务 ID 精确搜索 | 已实现 | 数值输入转换为 task ID equal。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F055 | 已标注/未标注筛选 | 已实现 | 服务端 completed_at empty 条件。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F056 | 按 ID 升降序 | 已实现 | 使用服务端 ordering。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F057 | 任意字段排序 | 未实现 | 网页支持 Created at、预测分数等；QML 仅 ID。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F058 | 多字段与 AND/OR 复杂筛选 | 未实现 | QML 只组合固定的搜索和完成状态；不是网页通用 Filter Builder。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F059 | 按标注人、时间、数量、分数筛选 | 未实现 | 本机 Filters 字段菜单已观察到这些字段。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F060 | 按标注结果/预测结果筛选 | 未实现 | 本机字段菜单出现 Annotation results/Prediction results；可用语义与版本有关。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F061 | 复制/粘贴筛选 JSON、固定筛选侧栏 | 未实现 | 本机有 Copy/Paste filters 和 Pin filters，QML 无。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F062 | 新建、重命名、删除、保存标签页 | 未实现 | QML Default 只是固定外观，无标签页 API。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F063 | 完整列选择与动态任务字段 | 部分实现 | 只能额外开关文件名和创建时间；其他列固定，不能显示任意 data 字段。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F064 | 列宽调整、列顺序与密度 | 未实现 | 列宽基本固定；没有网页 Comfortable/Compact 密度切换。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F065 | 单任务勾选 | 已实现 | 选择具体 task ID。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F066 | 当前页全选 | 已实现 | 勾选本页任务。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F067 | 跨页保留所选任务 | 已实现 | 维护具体 ID 集合，筛选变化清空。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F068 | 一键全选全部筛选结果 | 未实现 | 批量接口始终 all=false + included IDs，不能服务端全选整个结果集。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F069 | 批量删除任务 | 已实现 | 确认后提交 delete_tasks。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F070 | 批量删除全部标注和草稿 | 已实现 | 确认后提交 delete_tasks_annotations。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F071 | 按指定标注人删除标注 | 未实现 | 网页删除对话框的标注人表单未接入，QML 总是清空所选任务的全部标注。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F072 | 批量删除预测 | 已实现 | 提交 delete_tasks_predictions；不代表实现预标注查看/使用。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F073 | 检索模型预测 | 未实现 | 本机 Actions 的 Retrieve Predictions 未接入。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F074 | 从预测批量创建标注 | 未实现 | 本机 Create Annotations From Predictions 未接入。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F075 | 移除重复任务 | 未实现 | 本机 Remove Duplicated Tasks 未接入。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F076 | 按所选 N 条任务进入标注队列 | 未实现 | QML 勾选用于删除；Label All Tasks 仍从当前列表首条开始，未用选中 ID 集合。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F077 | 按当前筛选/排序逐条标注 | 部分实现 | 在已支持的固定查询范围内可跨页顺序前进；非服务端完整采样/任务分配流程。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F078 | 任务快速查看侧栏 | 未实现 | QML 点击任务切换到全页 Workspace，未复刻网页数据管理器与编辑器并排视图。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F079 | 原始任务 JSON/任务源查看 | 未实现 | 本机 view-task-source 入口可见，QML 无详情查看器。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |
| F080 | 完整任务信息和概要面板 | 部分实现 | 有数量和 ID，缺任意元数据、创建/修改人、模型分数及完整任务概要。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/manage_data)；qml/ProjectPage.qml; src/taskworkflow.cpp; src/apiclient.cpp |

### 导入

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| F081 | 多图片本地上传 | 已实现 | multipart 每文件独立字段，有真实多文件联调。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/tasks)；qml/ImportPane.qml; qml/ImportDialog.qml; src/projectapi.cpp |
| F082 | 拖放、浏览文件、待导入清单 | 已实现 | 原生文件选择器/DropArea；支持清空选择。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/tasks)；qml/ImportPane.qml; qml/ImportDialog.qml; src/projectapi.cpp |
| F083 | JSON/CSV/TSV 任务文件 | 部分实现 | 可交给 Django 导入；原生标注仅单图片矩形/多边形，不能等同于任意结构化任务支持。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/tasks)；qml/ImportPane.qml; qml/ImportDialog.qml; src/projectapi.cpp |
| F084 | 数据集 URL 导入 | 已实现 | http(s) URL 表单编码交给 Django 导入。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/tasks)；qml/ImportPane.qml; qml/ImportDialog.qml; src/projectapi.cpp |
| F085 | 包含既有标注的 JSON 导入 | 部分实现 | 通用上传可由后端导入；原生只可读取受支持的矩形/多边形结果，未逐格式实测。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/tasks)；qml/ImportPane.qml; qml/ImportDialog.qml; src/projectapi.cpp |
| F086 | 包含 predictions 的 JSON 导入 | 部分实现 | 能传给后端；QML fetchTask 不读取 predictions，无法显示/接受预标注。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/tasks)；qml/ImportPane.qml; qml/ImportDialog.qml; src/projectapi.cpp |
| F087 | 音频/视频/文本/HTML/PDF 导入并标注 | 未实现 | All files 可能把文件传到 Django，但没有对应原生对象与标注控件，业务流程不成立。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/tasks)；qml/ImportPane.qml; qml/ImportDialog.qml; src/projectapi.cpp |
| F088 | 大文件分块、后台导入、可恢复上传 | 未实现 | 客户端一次拼装请求，批次约 64 MB 限制；没有分块/断点续传。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/tasks)；qml/ImportPane.qml; qml/ImportDialog.qml; src/projectapi.cpp |
| F089 | 多 URL 队列与导入结果明细 | 部分实现 | 一个 URL 输入，返回统一成功/失败消息；无网页逐文件明细、字段映射与完整导入向导。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/tasks)；qml/ImportPane.qml; qml/ImportDialog.qml; src/projectapi.cpp |
| F090 | 云端/本地目录同步导入 | 未实现 | 需要存储配置和同步接口，QML 都没有。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/tasks)；qml/ImportPane.qml; qml/ImportDialog.qml; src/projectapi.cpp |

### 导出

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| F091 | 完整 Label Studio JSON 导出 | 已实现 | 同步 GET exportType=JSON，包含全部任务，QSaveFile 原子写入；已验证重载导出。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；qml/ProjectPage.qml; src/projectapi.cpp: exportProject |
| F092 | 只导出选中任务/当前筛选标签页 | 未实现 | 固定 download_all_tasks=true，不使用选中集/筛选器限定导出。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；qml/ProjectPage.qml; src/projectapi.cpp: exportProject |
| F093 | 仅导出已标注任务 | 未实现 | 没有导出范围选择开关。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；qml/ProjectPage.qml; src/projectapi.cpp: exportProject |
| F094 | 导出格式选择器 | 未实现 | 文件对话框只提供 JSON。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；qml/ProjectPage.qml; src/projectapi.cpp: exportProject |
| F095 | 快照创建、状态和下载管理 | 未实现 | 没有 snapshot API 工作流；不能把调用 JSON 导出算成快照导出。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；qml/ProjectPage.qml; src/projectapi.cpp: exportProject |
| F096 | 大项目后台导出、进度和重试 | 未实现 | QML 为同步请求，受超时和内存约束。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；qml/ProjectPage.qml; src/projectapi.cpp: exportProject |
| F097 | 导出中包含媒体文件 | 未实现 | 没有下载媒体和打包功能。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；qml/ProjectPage.qml; src/projectapi.cpp: exportProject |

### 导出格式

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| F098 | JSON-MIN | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F099 | CSV | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F100 | TSV | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F101 | COCO | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F102 | COCO with Images | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F103 | Pascal VOC XML | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F104 | YOLO | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F105 | YOLO with Images | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F106 | YOLOv8 OBB | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F107 | YOLOv8 OBB with Images | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F108 | CoNLL2003 | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F109 | Brush labels to NumPy | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F110 | Brush labels to PNG | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F111 | Brush labels to COCO | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F112 | ASR Manifest | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F113 | DocLang (.dclx) | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |
| F114 | spaCy（文档列出，当前图片项目导出列表未显示） | 未实现 | QML 没有该格式入口、转换请求或下载处理；能否服务端导出还取决于项目标注类型。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/export)；src/projectapi.cpp: exportProject 固定 exportType=JSON |

### 标注工作流与工作区

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| F115 | 读取已有矩形/多边形标注 | 已实现 | 解析结果并保留区域 ID；不支持其他结果组合。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F116 | 新建多份标注、切换已有标注 | 已实现 | 下拉选择 Annotation 或 New annotation；切换有未保存保护。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F117 | 新建提交与已有标注更新 | 已实现 | POST 新标注，PATCH 已有标注；有真实保存重载验证。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F118 | 提交/更新后继续下一任务 | 已实现 | 可选 Next after submit，支持跨任务分页和列表结束反馈。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F119 | 上一任务/下一任务 | 已实现 | 在桌面当前查询顺序内导航。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F120 | 提交并退出标注流 | 未实现 | 没有一键 Submit and Exit，只能保存后另行导航。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F121 | 真正记录跳过状态 | 已实现 | 创建 was_cancelled=true 空标注；不覆盖原标注。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F122 | 跳过队列、多人预留与锁定语义 | 部分实现 | 记录跳过后按桌面列表前进；未使用服务端 next-task/锁超时队列完整工作流。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F123 | 空标注提交 | 已实现 | 受支持配置可提交空 result；没有项目级开关或必填语义校验。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F124 | 删除当前标注 | 已实现 | 确认后 DELETE annotation，刷新任务统计。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F125 | 设置/取消 Ground truth | 已实现 | PATCH ground_truth；已有唯一性及真实接口验证，文档版本表差异见备注。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F126 | 全部标注并排比较 | 未实现 | 本机 Compare All 按钮可见；QML 只能逐份切换。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F127 | 标注人姓名、时间、分数等详情 | 部分实现 | 下拉显示 User ID 和标注号；缺完整作者、时间、计时、分数、来源信息。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F128 | 未保存修改离开确认 | 已实现 | 任务/页面/关闭/标注切换时保留保护；包括未闭合多边形。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F129 | 失败后保留编辑、撤销/重做 | 已实现 | 内存历史及保存失败保留编辑；不是服务器版本历史。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F130 | 自动保存草稿到服务器 | 未实现 | 无 drafts API；离开或崩溃后不能恢复未提交修改。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F131 | 草稿列表、恢复和草稿转正式标注 | 未实现 | 没有原生草稿管理。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F132 | 重置当前编辑到已保存状态 | 部分实现 | 导航放弃确认可以还原；缺网页独立 Reset 按钮。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F133 | 标注耗时/暂停计时 | 未实现 | 保存只传 result，未维护 lead_time、暂停和恢复逻辑。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F134 | 展示标注说明与标注前弹窗 | 未实现 | 既没有说明设置表单，也没有读取并呈现说明。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F135 | 区域列表与选择 | 已实现 | 有平面区域列表、单选、标签和基本坐标显示。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F136 | 区域属性数值编辑 | 未实现 | 坐标是只读 Label，不能数值输入 X/Y/W/H/角度。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F137 | 隐藏/显示单个或全部区域 | 未实现 | 没有可见性状态或眼睛工具。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F138 | 锁定/解锁区域 | 未实现 | 没有区域锁和对应交互。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F139 | 多选区域、批量改标签/删除 | 未实现 | 仅 selectedIndex 单选。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F140 | 复制、粘贴、克隆区域 | 未实现 | 无 Ctrl+D/C/V 或等价命令。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F141 | 区域分组、排序、层级大纲 | 未实现 | 没有按标签/工具分组、树结构、时间/分数排序。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F142 | 区域关系/关系标签/方向 | 未实现 | 解析器拒绝 Relations 及 relation 结果。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F143 | 区域自定义元数据和每区域控件 | 未实现 | 无 perRegion 控件或 metadata 编辑器。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F144 | 预测可视化、置信分数和来源 | 未实现 | 不读取 task.predictions，不能把预测计数列视为可视化。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F145 | 接受/拒绝模型建议 | 未实现 | 没有预测建议状态或转人工标注流程。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F146 | 编辑器布局、面板停靠/折叠/全屏 | 未实现 | 工作区使用固定左右布局，没有网页 Outliner 布局管理。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F147 | 编辑器偏好设置 | 未实现 | 无标签显示、透明度、线宽、保持选中、界面按钮等偏好配置。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F148 | 标注历史记录/版本追踪 | 未实现 | QML 撤销仅本次会话；本机 History 入口存在但本轮未验证付费后端完整能力。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F149 | 评论、回复、解决和通知 | 未实现 | 本机 Comments 入口可见；没有 QML 页面/接口；未发送评论验证服务器写入。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F150 | 任务/标注/区域深链接 | 未实现 | 无原生 URL/deep-link 解析和定位。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F151 | 固定常用快捷键 | 已实现 | R/V/H、保存、撤销、重做、Delete、Enter/Esc；与网页完整键表不同。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |
| F152 | 用户/项目级快捷键同步 | 未实现 | 未读取后端用户 Hotkeys 或 EDITOR_KEYMAP。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；qml/Workspace.qml; qml/AnnotationCanvas.qml; src/apiclient.cpp; src/taskworkflow.cpp |

### 图片绘制与编辑

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| F153 | 轴对齐带标签矩形绘制 | 已实现 | 拖动绘制、百分比坐标、保留 ID。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；src/annotationdocument.cpp; qml/AnnotationCanvas.qml |
| F154 | 矩形移动与四角缩放 | 已实现 | 鼠标编辑，有原生 UI 自动测试。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；src/annotationdocument.cpp; qml/AnnotationCanvas.qml |
| F155 | 带标签多边形绘制 | 已实现 | 逐点、点击首点/Enter 闭合、Esc 取消。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；src/annotationdocument.cpp; qml/AnnotationCanvas.qml |
| F156 | 多边形顶点拖动与整体移动 | 已实现 | 有原生 UI 和真实后端多边形联调。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；src/annotationdocument.cpp; qml/AnnotationCanvas.qml |
| F157 | 多边形添加/删除已有顶点 | 未实现 | 现有顶点可拖动，未实现插入/移除顶点。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；src/annotationdocument.cpp; qml/AnnotationCanvas.qml |
| F158 | 修改区域标签与删除 | 已实现 | 单选区域修改标签/删除。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；src/annotationdocument.cpp; qml/AnnotationCanvas.qml |
| F159 | 图片缩放、平移、适应窗口 | 已实现 | 滚轮、按钮、中键；zoom 约 0.2 到 8 倍。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；src/annotationdocument.cpp; qml/AnnotationCanvas.qml |
| F160 | 图片原始尺寸与 EXIF 方向 | 已实现 | C++ 读取尺寸/EXIF，已有方向坐标测试。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；src/annotationdocument.cpp; qml/AnnotationCanvas.qml |
| F161 | 图片/矩形旋转、三点旋转框 | 未实现 | 非零 image_rotation/rotation 结果被拒绝。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；src/annotationdocument.cpp; qml/AnnotationCanvas.qml |
| F162 | 椭圆、关键点、笔刷、掩码、向量 | 未实现 | 均无原生绘制工具和结果处理；标签明细见附录。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；src/annotationdocument.cpp; qml/AnnotationCanvas.qml |
| F163 | 无标签区域与后补标签 | 未实现 | 必须有至少一个静态标签，且结果必须单标签。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；src/annotationdocument.cpp; qml/AnnotationCanvas.qml |
| F164 | 同一区域多标签/多控件结果 | 未实现 | 结果验证要求一个标签、单一形状控件。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；src/annotationdocument.cpp; qml/AnnotationCanvas.qml |
| F165 | 按实例像素分割及擦除 | 未实现 | 没有画笔、位掩码或橡皮擦。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；src/annotationdocument.cpp; qml/AnnotationCanvas.qml |
| F166 | 图像亮度/对比度/滤镜 | 未实现 | 没有图像调节工具。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；src/annotationdocument.cpp; qml/AnnotationCanvas.qml |
| F167 | 多图页导航、多模态联动 | 未实现 | 只展示一张图片。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/labeling)；src/annotationdocument.cpp; qml/AnnotationCanvas.qml |

### 模型与预测

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| F168 | 模型连接新增/列表/修改/删除 | 未实现 | 项目设置无 Model 分区，未调用 ML backend 管理接口。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/ml)；src/apiclient.h; src/apiclient.cpp; qml/ProjectSettings.qml |
| F169 | 模型认证、参数和交互模式 | 未实现 | 无模型连接表单。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/ml)；src/apiclient.h; src/apiclient.cpp; qml/ProjectSettings.qml |
| F170 | 模型测试请求与健康检查 | 未实现 | 未提供 Send Test Request。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/ml)；src/apiclient.h; src/apiclient.cpp; qml/ProjectSettings.qml |
| F171 | 手动启动训练 | 未实现 | 无 Start Training。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/ml)；src/apiclient.h; src/apiclient.cpp; qml/ProjectSettings.qml |
| F172 | 提交后触发训练设置 | 未实现 | 无法配置后端训练开关；服务端已经配置的自动行为不算 QML 功能。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/ml)；src/apiclient.h; src/apiclient.cpp; qml/ProjectSettings.qml |
| F173 | 预测版本/分数列表与管理 | 未实现 | 无 Predictions 分区，仅显示任务预测数量。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/ml)；src/apiclient.h; src/apiclient.cpp; qml/ProjectSettings.qml |
| F174 | 选择预标注来源与默认模型 | 未实现 | 没有 model_version/prelabel 配置。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/ml)；src/apiclient.h; src/apiclient.cpp; qml/ProjectSettings.qml |
| F175 | 启用/禁用用预测预标注 | 未实现 | 没有 Annotation Settings 对应表单和渲染。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/ml)；src/apiclient.h; src/apiclient.cpp; qml/ProjectSettings.qml |
| F176 | 智能工具与交互预测调用 | 未实现 | 没有 context/predict 请求、SAM/自动检测等工具。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/ml)；src/apiclient.h; src/apiclient.cpp; qml/ProjectSettings.qml |
| F177 | 预测数据上传后修正提交 | 部分实现 | JSON 可交给后端；QML 不读取 predictions，只有转为支持的正式标注后才可编辑。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/ml)；src/apiclient.h; src/apiclient.cpp; qml/ProjectSettings.qml |

### 存储

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| F178 | 源存储新增/修改/删除 | 未实现 | 无 Cloud Storage 分区或存储 API。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/storage)；qml/ProjectSettings.qml; src/apiclient.h |
| F179 | 目标存储新增/修改/删除 | 未实现 | 无原生目标存储管理。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/storage)；qml/ProjectSettings.qml; src/apiclient.h |
| F180 | 源同步与目标标注同步 | 未实现 | 无 Sync 操作。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/storage)；qml/ProjectSettings.qml; src/apiclient.h |
| F181 | 同步状态、历史、错误和计数 | 未实现 | 无对应状态页面。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/storage)；qml/ProjectSettings.qml; src/apiclient.h |
| F182 | 本地目录源/目标存储 | 未实现 | 不能配置本地目录、正则或同步策略。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/storage)；qml/ProjectSettings.qml; src/apiclient.h |
| F183 | Amazon S3 基础连接 | 未实现 | 未接入；后端提供基础连接能力。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/storage)；qml/ProjectSettings.qml; src/apiclient.h |
| F184 | Google Cloud Storage 基础连接 | 未实现 | 未接入；后端提供基础连接能力。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/storage)；qml/ProjectSettings.qml; src/apiclient.h |
| F185 | Azure Blob 基础连接 | 未实现 | 未接入；后端提供基础连接能力。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/storage)；qml/ProjectSettings.qml; src/apiclient.h |
| F186 | Redis 存储连接 | 未实现 | 未接入；后端提供基础连接能力。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/storage)；qml/ProjectSettings.qml; src/apiclient.h |
| F187 | 文件/任务导入模式与前缀/正则 | 未实现 | 没有存储选项表单。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/storage)；qml/ProjectSettings.qml; src/apiclient.h |
| F188 | 预签名 URL、URI 解析与代理媒体 | 部分实现 | 能读普通 http(s)/相对媒体并跟随有限重定向；不会调用任务 URI resolve 接口，原始 s3/gs URI 不能直接使用。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/storage)；qml/ProjectSettings.qml; src/apiclient.h |
| F189 | 存储凭据/TTL/删除同步配置 | 未实现 | 没有相关设置和 API。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/storage)；qml/ProjectSettings.qml; src/apiclient.h |

### Webhooks 与开发集成

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| F190 | 项目 Webhook 列表/新增/编辑/删除 | 未实现 | 网页有独立设置页，QML 无。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/webhooks)；qml/ProjectSettings.qml; src/apiclient.h; qml/HomePage.qml |
| F191 | Webhook 事件选择与启用开关 | 未实现 | 没有事件选择表单。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/webhooks)；qml/ProjectSettings.qml; src/apiclient.h; qml/HomePage.qml |
| F192 | Webhook headers、认证、payload 选项 | 未实现 | 没有相关管理接口。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/webhooks)；qml/ProjectSettings.qml; src/apiclient.h; qml/HomePage.qml |
| F193 | 组织级 Webhook | 未实现 | 既无原生管理，也未为本机启用/验证对应组织环境变量。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/webhooks)；qml/ProjectSettings.qml; src/apiclient.h; qml/HomePage.qml |
| F194 | 调用现有 Django REST API | 部分实现 | 桌面使用业务所需接口，不是覆盖整个 Label Studio API 的管理客户端。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/webhooks)；qml/ProjectSettings.qml; src/apiclient.h; qml/HomePage.qml |
| F195 | Python SDK 自动化 | 仅后端已有 | 外部 SDK 可继续使用同一 Django；不是 QML 内置脚本/SDK 控制台。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/webhooks)；qml/ProjectSettings.qml; src/apiclient.h; qml/HomePage.qml |
| F196 | API/文档等资源链接 | 仅网页跳转 | 能从首页打开文档，不能算实现文档所描述的业务功能。 | 社区版/本机网页 | [文档](https://labelstudio.com.cn/guide/webhooks)；qml/ProjectSettings.qml; src/apiclient.h; qml/HomePage.qml |

### 桌面基础与本地增强

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| F197 | 全原生 QML / Material | 已实现 | C++17 + Qt Quick Controls Material；不依赖网页嵌入。 | 桌面自身能力，非社区版对等分母 | [文档](https://labelstudio.com.cn/guide/hotkeys)；qml/Main.qml; src/languagemanager.cpp; build.ps1 |
| F198 | 中英文即时切换及记忆 | 已实现 | 303 条应用翻译；Qt 标准控件翻译；保留编辑和选择。 | 桌面自身能力，非社区版对等分母 | [文档](https://labelstudio.com.cn/guide/hotkeys)；qml/Main.qml; src/languagemanager.cpp; build.ps1 |
| F199 | 浅色/深色主题及记忆 | 已实现 | Material 配合 Theme 状态和 QSettings。 | 桌面自身能力，非社区版对等分母 | [文档](https://labelstudio.com.cn/guide/hotkeys)；qml/Main.qml; src/languagemanager.cpp; build.ps1 |
| F200 | 日期本地化 | 部分实现 | 项目卡片跟随语言；任务列表仍手工截取 ISO 字符串。 | 桌面自身能力，非社区版对等分母 | [文档](https://labelstudio.com.cn/guide/hotkeys)；qml/Main.qml; src/languagemanager.cpp; build.ps1 |
| F201 | MSVC 编译与运行库打包 | 已实现 | Qt 6.12 + VS 2026；有 windeployqt 独立运行验证。 | 桌面自身能力，非社区版对等分母 | [文档](https://labelstudio.com.cn/guide/hotkeys)；qml/Main.qml; src/languagemanager.cpp; build.ps1 |
| F202 | Qt 6.6.3 实机兼容验证 | 未验证 | CMake 最低版本为 6.6.3，现有实际测试仅 Qt 6.12。 | 桌面自身能力，非社区版对等分母 | [文档](https://labelstudio.com.cn/guide/hotkeys)；qml/Main.qml; src/languagemanager.cpp; build.ps1 |
| F203 | 离线演示 | 已实现 | 示例只保存在内存；不等于真实项目离线标注。 | 桌面自身能力，非社区版对等分母 | [文档](https://labelstudio.com.cn/guide/hotkeys)；qml/Main.qml; src/languagemanager.cpp; build.ps1 |
| F204 | 真实任务离线缓存、恢复和同步 | 未实现 | 没有本地项目数据库或离线提交队列。 | 桌面自身能力，非社区版对等分母 | [文档](https://labelstudio.com.cn/guide/hotkeys)；qml/Main.qml; src/languagemanager.cpp; build.ps1 |
| F205 | 自动启动/停止 Django 与服务状态 | 未实现 | 后端由用户单独启动。 | 桌面自身能力，非社区版对等分母 | [文档](https://labelstudio.com.cn/guide/hotkeys)；qml/Main.qml; src/languagemanager.cpp; build.ps1 |
| F206 | 跨重启会话/凭据记忆 | 未实现 | 凭据只保存在进程内存；只记服务器地址、主题、显式语言选择。 | 桌面自身能力，非社区版对等分母 | [文档](https://labelstudio.com.cn/guide/hotkeys)；qml/Main.qml; src/languagemanager.cpp; build.ps1 |
| F207 | 更新检查/安装器/签名 | 未实现 | 当前是可分发目录，没有完整安装升级流程。 | 桌面自身能力，非社区版对等分母 | [文档](https://labelstudio.com.cn/guide/hotkeys)；qml/Main.qml; src/languagemanager.cpp; build.ps1 |

## 标签目录逐项对照

以下列出指定文档当前标签导航中的全部 51 项。这里评价的是**标签语义及渲染能力**；基础工具主流程已实现并不等于该标签所有属性全部兼容。目录中存在商业版或新版本标签，本机可用性不能仅凭文档名称推定。

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| T001 | Audio | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/audio)；src/annotationdocument.cpp: configure/load 白名单 |
| T002 | Chat | 未实现 | QML 无对应引擎；文档含商业版/实验功能，本机是否可用须另核对。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/chat)；src/annotationdocument.cpp: configure/load 白名单 |
| T003 | HyperText | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/hypertext)；src/annotationdocument.cpp: configure/load 白名单 |
| T004 | Image | 部分实现 | 仅一个直接字段图片；无多图、列表、旋转及完整对象属性。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/image)；src/annotationdocument.cpp: configure/load 白名单 |
| T005 | List | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/list)；src/annotationdocument.cpp: configure/load 白名单 |
| T006 | Paragraphs | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/paragraphs)；src/annotationdocument.cpp: configure/load 白名单 |
| T007 | Pdf | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/pdf)；src/annotationdocument.cpp: configure/load 白名单 |
| T008 | ReactCode | 未实现 | QML 无对应引擎；文档含商业版/实验功能，本机是否可用须另核对。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/reactcode)；src/annotationdocument.cpp: configure/load 白名单 |
| T009 | Table | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/table)；src/annotationdocument.cpp: configure/load 白名单 |
| T010 | Text | 未实现 | 解析白名单允许该标签，但实际内容/效果未呈现；不能标为已支持。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/text)；src/annotationdocument.cpp: configure/load 白名单 |
| T011 | TimeSeries | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/timeseries)；src/annotationdocument.cpp: configure/load 白名单 |
| T012 | Video | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/video)；src/annotationdocument.cpp: configure/load 白名单 |
| T013 | Brush | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/brush)；src/annotationdocument.cpp: configure/load 白名单 |
| T014 | BrushLabels | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/brushlabels)；src/annotationdocument.cpp: configure/load 白名单 |
| T015 | BitmaskLabels | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/bitmasklabels)；src/annotationdocument.cpp: configure/load 白名单 |
| T016 | Choice | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/choice)；src/annotationdocument.cpp: configure/load 白名单 |
| T017 | Choices | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/choices)；src/annotationdocument.cpp: configure/load 白名单 |
| T018 | DateTime | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/datetime)；src/annotationdocument.cpp: configure/load 白名单 |
| T019 | Ellipse | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/ellipse)；src/annotationdocument.cpp: configure/load 白名单 |
| T020 | EllipseLabels | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/ellipselabels)；src/annotationdocument.cpp: configure/load 白名单 |
| T021 | HyperTextLabels | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/hypertextlabels)；src/annotationdocument.cpp: configure/load 白名单 |
| T022 | KeyPoint | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/keypoint)；src/annotationdocument.cpp: configure/load 白名单 |
| T023 | KeyPointLabels | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/keypointlabels)；src/annotationdocument.cpp: configure/load 白名单 |
| T024 | Label | 部分实现 | 静态名称和颜色；无 alias、动态标签、hotkey 等。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/label)；src/annotationdocument.cpp: configure/load 白名单 |
| T025 | Labels | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/labels)；src/annotationdocument.cpp: configure/load 白名单 |
| T026 | MagicWand | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/magicwand)；src/annotationdocument.cpp: configure/load 白名单 |
| T027 | Number | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/number)；src/annotationdocument.cpp: configure/load 白名单 |
| T028 | Pairwise | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/pairwise)；src/annotationdocument.cpp: configure/load 白名单 |
| T029 | ParagraphLabels | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/paragraphlabels)；src/annotationdocument.cpp: configure/load 白名单 |
| T030 | Polygon | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/polygon)；src/annotationdocument.cpp: configure/load 白名单 |
| T031 | PolygonLabels | 部分实现 | 基础绘制/编辑/保存已实现；不含全部标签属性与高级交互。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/polygonlabels)；src/annotationdocument.cpp: configure/load 白名单 |
| T032 | Ranker | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/ranker)；src/annotationdocument.cpp: configure/load 白名单 |
| T033 | Rating | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/rating)；src/annotationdocument.cpp: configure/load 白名单 |
| T034 | Rectangle | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/rectangle)；src/annotationdocument.cpp: configure/load 白名单 |
| T035 | RectangleLabels | 部分实现 | 基础绘制/编辑/保存已实现；不含全部标签属性与高级交互。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/rectanglelabels)；src/annotationdocument.cpp: configure/load 白名单 |
| T036 | Relation | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/relation)；src/annotationdocument.cpp: configure/load 白名单 |
| T037 | Relations | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/relations)；src/annotationdocument.cpp: configure/load 白名单 |
| T038 | Shortcut | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/shortcut)；src/annotationdocument.cpp: configure/load 白名单 |
| T039 | Taxonomy | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/taxonomy)；src/annotationdocument.cpp: configure/load 白名单 |
| T040 | TextArea | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/textarea)；src/annotationdocument.cpp: configure/load 白名单 |
| T041 | TimeSeriesLabels | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/timeserieslabels)；src/annotationdocument.cpp: configure/load 白名单 |
| T042 | TimelineLabels | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/timelinelabels)；src/annotationdocument.cpp: configure/load 白名单 |
| T043 | Vector | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/vector)；src/annotationdocument.cpp: configure/load 白名单 |
| T044 | VectorLabels | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/vectorlabels)；src/annotationdocument.cpp: configure/load 白名单 |
| T045 | VideoRectangle | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/videorectangle)；src/annotationdocument.cpp: configure/load 白名单 |
| T046 | Collapse | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/collapse)；src/annotationdocument.cpp: configure/load 白名单 |
| T047 | Filter | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/filter)；src/annotationdocument.cpp: configure/load 白名单 |
| T048 | Header | 未实现 | 解析白名单允许该标签，但实际内容/效果未呈现；不能标为已支持。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/header)；src/annotationdocument.cpp: configure/load 白名单 |
| T049 | Markdown | 未实现 | 无对应原生对象/控件/结果处理；不支持的配置会被拒绝。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/markdown)；src/annotationdocument.cpp: configure/load 白名单 |
| T050 | Style | 未实现 | 解析白名单允许该标签，但实际内容/效果未呈现；不能标为已支持。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/style)；src/annotationdocument.cpp: configure/load 白名单 |
| T051 | View | 部分实现 | 仅作为 XML 结构容器接受；不渲染嵌套布局/样式。 | 文档标签；不同版本可用性以本机为准 | [文档](https://labelstudio.com.cn/tags/view)；src/annotationdocument.cpp: configure/load 白名单 |

本机网页源码还包含 `PagedView`、`Bitmask`、`Dialog`、`VideoVector`、`VideoVectorLabels` 等文档当前导航未单列的组件，QML 同样无实现。应在后续原生 XML 引擎设计时逐一建支持矩阵，而不是忽略未知属性后宣称兼容。

## 模板目录逐项对照

此处列出模板导航及全部模板分类页收录的 86 个具体模板（排除分类页和入门页）。只有基础矩形/多边形流程有对应实现；两行标为“部分实现”是按官方模板整体语义评价，不否认相应基础绘制工作流已经可用。标为商业版的依据是目录中的锁标记，未标锁不保证当前本机版本支持。

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| M001 | 多边形语义分割 | 部分实现 | QML 提供这一任务类型的基础模板和绘制流程；不代表完整复制官方模板全部参数/交互。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/image_polygons)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M002 | 掩码语义分割 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/image_segmentation)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M003 | 边界框目标检测 | 部分实现 | QML 提供这一任务类型的基础模板和绘制流程；不代表完整复制官方模板全部参数/交互。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/image_bbox)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M004 | 椭圆目标检测 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/image_ellipse)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M005 | 关键点标注 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/image_keypoints)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M006 | 光学字符识别 (OCR) | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/optical_character_recognition)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M007 | PDF OCR 标注 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档标注为商业版 | [文档](https://labelstudio.com.cn/templates/pdf_ocr)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M008 | 多页文档标注 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/multi-page-document-annotation)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M009 | 图像描述生成 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/image_captioning)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M010 | 图像分类 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/image_classification)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M011 | 多图像分类 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/multi-image_classification)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M012 | 视觉问答 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/visual_question_answering)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M013 | 库存追踪 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/inventory_tracking)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M014 | 视觉基因组 (Visual Genome) | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/visual_genome)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M015 | 带边界框的医学图像分类 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/medical_imaging_classification)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M016 | 命名实体识别 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/named_entity)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M017 | 情感分析文本分类 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/sentiment_analysis)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M018 | 分类法 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/taxonomy)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M019 | 关系抽取 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/relation_extraction)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M020 | 文本摘要 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/text_summarization)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M021 | 机器翻译 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/machine_translation)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M022 | 问答 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/question_answering)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M023 | 内容审核 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/content_moderation)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M024 | 上下文滚动 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/contextual_scrolling)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M025 | 自动语音识别 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/transcribe_audio)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M026 | 基于片段的自动语音识别 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/automatic_speech_recognition_segments)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M027 | 信号质量检测 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/signal_quality_detection)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M028 | 声音事件检测 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/sound_event_detection)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M029 | 语音活动检测 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/voice_activity_detection)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M030 | 说话人日志 (Speaker Diarization) | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/speaker_segmentation)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M031 | 音频分类 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/audio_classification)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M032 | 基于片段的音频分类 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/audio_regions)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M033 | 意图分类 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/intent_classification)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M034 | 电子表格编辑器 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档标注为商业版 | [文档](https://labelstudio.com.cn/templates/react_spreadsheet)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M035 | 多通道音频转录 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档标注为商业版 | [文档](https://labelstudio.com.cn/templates/react_audio)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M036 | 理赔代理追踪 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档标注为商业版 | [文档](https://labelstudio.com.cn/templates/react_claims)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M037 | 视觉排序器 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/generative-visual-ranker)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M038 | 搜索页面排名 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/serp_ranking)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M039 | 网站评分 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/website_rating)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M040 | 配对回归 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/pairwise_regression)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M041 | 配对分类 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/pairwise_comparison)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M042 | 基于内容的图像检索 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/content_image_retrieval)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M043 | 文档检索 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/document_retrieval)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M044 | 文本生成图像 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/text_to_image)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M045 | ASR 假设选择 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/asr_hypotheses)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M046 | 自由格式元数据 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/freeform_metadata)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M047 | 表格数据 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/tabular_data)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M048 | PDF 分类 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/pdf_classification)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M049 | HTML 实体识别 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/html_document)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M050 | HTML 分类 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/html_classification)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M051 | 时间序列标注 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/time_series)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M052 | 时间序列分类 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/time_series_classification)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M053 | 时间序列预测 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/time_series_forecasting)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M054 | 带音频和视频同步的时间序列标注 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/timeseries_audio_video)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M055 | 变点检测 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/change_point_detection)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M056 | 活动识别 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/activity_recognition)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M057 | 信号质量 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/signal_quality)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M058 | 离群值与异常检测 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/outliers_anomaly_detection)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M059 | 视频分类 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/video_classification)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M060 | 视频帧分类 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/video_frame_classification)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M061 | 视频时间轴分割 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/video_timeline_segmentation)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M062 | 视频目标检测与追踪 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/video_object_detector)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M063 | 响应生成 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/response_generation)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M064 | 响应选择 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/response_selection)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M065 | 共指消解与实体链接 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/coreference_entity_linking)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M066 | 对话分析 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/dialogue_analysis)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M067 | 槽位填充与意图分类 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/slot_filling)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M068 | 聊天机器人评估 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档标注为商业版 | [文档](https://labelstudio.com.cn/templates/chatbot)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M069 | 利用 LLM 微调代理 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档标注为商业版 | [文档](https://labelstudio.com.cn/templates/chat_llm_eval)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M070 | 不利用 LLM 微调代理 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档标注为商业版 | [文档](https://labelstudio.com.cn/templates/chat_eval)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M071 | 聊天中的红队测试 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档标注为商业版 | [文档](https://labelstudio.com.cn/templates/chat_red_team)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M072 | 评估生产对话以进行 RLHF | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档标注为商业版 | [文档](https://labelstudio.com.cn/templates/chat_rlhf)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M073 | 监督式 LLM 微调 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/generative-supervised-llm)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M074 | 为 RLHF 收集人类偏好 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/generative-pairwise-human-preference)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M075 | RAG 检索 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/generative-llm-ranker)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M076 | LLM 响应审核 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/llm_response_moderation)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M077 | LLM 响应评分 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/llm_response_grading)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M078 | 并排 LLM 输出对比 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/llm_side_by_side)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M079 | 利用人类反馈评估 RAG | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/llm_rag_human_feedback)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M080 | 利用 Ragas 评估 RAG | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/llm_ragas)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M081 | 多轮对话评估 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/multi_turn_chat)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M082 | 乳腺癌乳房 X 光图像分类 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/breast_cancer_mammogram_classification)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M083 | HTML NER 标注 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/html_ner_tagging_person_and_organization)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M084 | 发票 NER 标注 (BIO 格式) | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/ner_tagging_invoices_bio_format)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M085 | 发票 OCR 预 NER BIO 格式 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/ocr_invoices_pre_ner_bio_format)；qml/LabelingSetup.qml; src/annotationdocument.cpp |
| M086 | X / Twitter 帖子的二级情感分析 | 未实现 | 没有对应原生模板/对象/控制标签组合；分类标题出现不等于模板可用。 | 文档未标锁；不保证每项均适用于本机版本 | [文档](https://labelstudio.com.cn/templates/two_level_sentiment_analysis_of_x_twitter_posts)；qml/LabelingSetup.qml; src/annotationdocument.cpp |

## 社区版、Starter Cloud 和企业版边界

以下保留[版本对照](https://labelstudio.com.cn/guide/label_studio_compare)中社区版标为不支持的每一行，并补上[企业功能说明](https://labelstudio.com.cn/guide/enterprise_features)的关键能力。此处的“未实现”不应直接转换成当前社区版的纯 QML 开发任务：工作区、RBAC、审核和商业分析等通常需要相应后端/API/授权或另行设计业务后端。

版本表中的认证、SLA 和人员支持属于商业服务，不计为 QML 功能缺口；可嵌入界面、白标和插件也须区分商业产品定义与开源集成方式。

**文档与本机存在差异，优先相信本机可核实证据：**

- 版本表把 Ground truth 标作付费能力，但本机 Django 接受该字段，QML 已有真实联调通过记录。
- 企业功能表把 Reset Cache 归为企业版，本机社区开发版 Danger Zone 实际有该按钮；QML 尚未实现。
- 企业说明中的采样表与社区项目设置说明不完全一致；本机顺序/随机可配置，不确定性采样是禁用的企业入口。QML 三者均未接入。
- 本机有 Comments、History 入口，不能据此断言评论写入、完整历史或企业审核 API 已可用。本轮没有发送评论或修改历史进行写入验证。
- 基本 ML 连接、手动训练、获取预测、Webhook 是社区能力；完整自动主动学习循环和不确定性任务策略要区分商业能力，不能把基本模型功能统统算成企业版。

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| E001 | 基于角色的工作流 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:受限；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E002 | 基于角色的访问控制 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:受限；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E003 | 可自定义的权限 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E004 | 高级云存储集成 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E005 | 聊天 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:受限；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E006 | 在工作区中组织项目 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E007 | 个人沙盒工作区 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:✅；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E008 | AI 助手 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:✅；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E009 | 项目成员资格 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:✅；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E010 | 项目级角色 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:受限；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E011 | 项目级用户设置 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:受限；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E012 | 分配任务 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:✅；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E013 | 自动分配任务 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:✅；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E014 | 简化的标注员界面 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:✅；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E015 | 批量标注 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E016 | 可编程与可嵌入界面 | 未实现（需区分版本） | 对照表将此项列为商业能力，但经典 LSF 嵌入、自行修改开源界面与商业版相应产品能力并非同一范围；QML 未提供对应兼容接口，不能据此断言所有同类集成都必须付费。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E017 | 白标 (White Labeling) | 未实现（需区分版本） | 对照表将此项列为商业能力，但经典 LSF 嵌入、自行修改开源界面与商业版相应产品能力并非同一范围；QML 未提供对应兼容接口，不能据此断言所有同类集成都必须付费。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E018 | 插件 | 未实现（需区分版本） | 对照表将此项列为商业能力，但经典 LSF 嵌入、自行修改开源界面与商业版相应产品能力并非同一范围；QML 未提供对应兼容接口，不能据此断言所有同类集成都必须付费。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E019 | 自动化预标注 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E020 | LLM 微调与评估 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E021 | 运行基准测试 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E022 | 主动学习循环 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E023 | 项目仪表板 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E024 | 标注员绩效仪表板 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E025 | 活动日志 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E026 | 标注历史记录 | 未实现（本机入口可见） | 本机有界面入口，但本轮未验证完整服务端读写/授权能力；QML 无对应功能。 | 社区:无；Starter:✅；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E027 | 分配审核员 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E028 | 任务自动重新分配 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E029 | 一致性指标 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:✅；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E030 | 自定义一致性指标 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E031 | 评论与通知 | 未实现（本机入口可见） | 本机有界面入口，但本轮未验证完整服务端读写/授权能力；QML 无对应功能。 | 社区:无；Starter:✅；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E032 | 识别基础事实 | 已实现（本机差异） | 对照表把此项列为付费；本机 Django 支持 ground_truth，QML 已实现并有真实联调记录。 | 社区:无；Starter:✅；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E033 | 重叠配置 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:✅；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E034 | 暂停标注员 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E035 | 标注限制 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E036 | 标注员共识矩阵 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E037 | 标签分布图表 | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E038 | SSO | 未实现（版本受限） | 版本对照标为社区版无此功能；QML 未实现，不能仅补前端就取得商业后端能力。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E039 | SOC2 | 不适用（认证/服务） | 这是供应商认证、合同或支持服务，不是可由 QML 实现的界面功能；保留该行仅为完整对照原表。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E040 | 支持门户 | 不适用（认证/服务） | 这是供应商认证、合同或支持服务，不是可由 QML 实现的界面功能；保留该行仅为完整对照原表。 | 社区:无；Starter:✅；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E041 | 正常运行时间 SLA | 不适用（认证/服务） | 这是供应商认证、合同或支持服务，不是可由 QML 实现的界面功能；保留该行仅为完整对照原表。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E042 | 客户成功经理 | 不适用（认证/服务） | 这是供应商认证、合同或支持服务，不是可由 QML 实现的界面功能；保留该行仅为完整对照原表。 | 社区:无；Starter:❌；企业:✅ | [文档](https://labelstudio.com.cn/guide/label_studio_compare)；版本对照表 + 本地网页/代码 |
| E043 | SCIM 用户/组自动配置 | 未实现（版本受限） | 身份提供商同步用户、角色和组，现有 QML 无实现。 | 企业版/商业能力，需对应后端 | [文档](https://labelstudio.com.cn/guide/enterprise_features)；企业功能文档；QML/C++ 无相关模块 |
| E044 | 多租户组织隔离 | 未实现（版本受限） | 独立组织的资源与用户隔离，需要企业级后端能力。 | 企业版/商业能力，需对应后端 | [文档](https://labelstudio.com.cn/guide/enterprise_features)；企业功能文档；QML/C++ 无相关模块 |
| E045 | Databricks Unity Catalog 存储 | 未实现（版本受限） | 商业连接器；QML 无任何存储连接器管理。 | 企业版/商业能力，需对应后端 | [文档](https://labelstudio.com.cn/guide/enterprise_features)；企业功能文档；QML/C++ 无相关模块 |
| E046 | S3 IAM / GCS WIF / Azure 服务主体 | 未实现（版本受限） | 高级云存储认证组合；QML 未实现。 | 企业版/商业能力，需对应后端 | [文档](https://labelstudio.com.cn/guide/enterprise_features)；企业功能文档；QML/C++ 无相关模块 |
| E047 | 提示词模板、内联运行和生成任务 | 未实现（版本受限） | Prompts 模块；QML 无。 | 企业版/商业能力，需对应后端 | [文档](https://labelstudio.com.cn/guide/enterprise_features)；企业功能文档；QML/C++ 无相关模块 |
| E048 | 任务预留、重试与跳过理由策略 | 未实现（版本受限） | 企业项目工作流选项，QML 只有基础 skip。 | 企业版/商业能力，需对应后端 | [文档](https://labelstudio.com.cn/guide/enterprise_features)；企业功能文档；QML/C++ 无相关模块 |
| E049 | 自定义标签权重和低一致性策略 | 未实现（版本受限） | 质量管理规则及服务端策略，QML 无。 | 企业版/商业能力，需对应后端 | [文档](https://labelstudio.com.cn/guide/enterprise_features)；企业功能文档；QML/C++ 无相关模块 |

## 保留后端与部署运维能力

以下功能由继续使用的 Django、外部模型、SDK 或部署环境提供，不应为了“重写前端”而重新实现整个服务器；若需要桌面管理入口，应新增对应 UI/API 操作。列为边界不代表本轮测试过全部部署模式。

| 编号 | 功能项 | QML 状态 | 具体范围或缺口 | 版本/范围 | 依据 |
|---|---|---|---|---|---|
| B001 | 命令行初始化管理员/用户及检索信息 | 保留后端能力／未做桌面管理 | 现有 Django/CLI 职责，QML 无用户创建管理界面。 | 后端/运维边界，不计普通 QML 功能缺口 | [文档](https://labelstudio.com.cn/guide/signup)；现有 Django 项目与所列文档；本轮不宣称已验证全部部署方式 |
| B002 | 仅允许邀请链接注册 | 保留后端能力／未做桌面管理 | LABEL_STUDIO_DISABLE_SIGNUP_WITHOUT_LINK 由服务端环境配置。 | 后端/运维边界，不计普通 QML 功能缺口 | [文档](https://labelstudio.com.cn/guide/signup)；现有 Django 项目与所列文档；本轮不宣称已验证全部部署方式 |
| B003 | SQLite/PostgreSQL 配置与迁移 | 保留后端能力／未做桌面管理 | 数据库由 Django 管理，不应在 QML 重写数据库。 | 后端/运维边界，不计普通 QML 功能缺口 | [文档](https://labelstudio.com.cn/guide/storedata)；现有 Django 项目与所列文档；本轮不宣称已验证全部部署方式 |
| B004 | 数据目录、持久化和备份恢复 | 保留后端能力／未做桌面管理 | 服务端部署运维能力；桌面无原生运维面板。 | 后端/运维边界，不计普通 QML 功能缺口 | [文档](https://labelstudio.com.cn/guide/persistent_storage)；现有 Django 项目与所列文档；本轮不宣称已验证全部部署方式 |
| B005 | pip/源码/Docker 安装启动 | 保留后端能力／未做桌面管理 | 现有 Django 部署流程；QML 未打包 Python 后端。 | 后端/运维边界，不计普通 QML 功能缺口 | [文档](https://labelstudio.com.cn/guide/install)；现有 Django 项目与所列文档；本轮不宣称已验证全部部署方式 |
| B006 | Kubernetes/Helm/Ingress/气隙部署 | 保留后端能力／未做桌面管理 | 部署环境能力；不计为普通桌面页面缺口。 | 后端/运维边界，不计普通 QML 功能缺口 | [文档](https://labelstudio.com.cn/guide/install_k8s)；现有 Django 项目与所列文档；本轮不宣称已验证全部部署方式 |
| B007 | 后端升级与数据库迁移 | 保留后端能力／未做桌面管理 | 保留原有服务端维护方式，桌面无升级管理。 | 后端/运维边界，不计普通 QML 功能缺口 | [文档](https://labelstudio.com.cn/guide/upgrade_community)；现有 Django 项目与所列文档；本轮不宣称已验证全部部署方式 |
| B008 | HTTPS、CORS、CSRF 与服务端安全策略 | 保留后端能力／未做桌面管理 | 服务器配置继续有效；桌面遵守会话/CSRF，非安全策略管理器。 | 后端/运维边界，不计普通 QML 功能缺口 | [文档](https://labelstudio.com.cn/guide/security)；现有 Django 项目与所列文档；本轮不宣称已验证全部部署方式 |
| B009 | REST API、Python SDK 与脚本导入导出 | 保留后端能力／未做桌面管理 | 现有后端/API 不因换用 QML 消失；桌面仅调用部分接口。 | 后端/运维边界，不计普通 QML 功能缺口 | [文档](https://labelstudio.com.cn/guide/api)；现有 Django 项目与所列文档；本轮不宣称已验证全部部署方式 |
| B010 | 自定义 ML 后端开发和部署 | 保留后端能力／未做桌面管理 | 模型服务代码独立于 UI；QML 尚缺模型连接管理。 | 后端/运维边界，不计普通 QML 功能缺口 | [文档](https://labelstudio.com.cn/guide/ml_create)；现有 Django 项目与所列文档；本轮不宣称已验证全部部署方式 |
| B011 | Webhook 投递、超时和日志 | 保留后端能力／未做桌面管理 | 已配置时 Django 负责执行，QML 缺管理页面。 | 后端/运维边界，不计普通 QML 功能缺口 | [文档](https://labelstudio.com.cn/guide/webhook_create)；现有 Django 项目与所列文档；本轮不宣称已验证全部部署方式 |
| B012 | 经典 LSF JavaScript 嵌入 API/回调 | 保留后端能力／未做桌面管理 | 属于浏览器前端集成接口；文档指出独立 LSF 包已弃用，不能视为原生 QML 已兼容。 | 后端/运维边界，不计普通 QML 功能缺口 | [文档](https://labelstudio.com.cn/guide/frontend_reference)；现有 Django 项目与所列文档；本轮不宣称已验证全部部署方式 |

## 建议补齐顺序

| 优先级 | 范围 | 可核验的完成条件 |
|---|---|---|
| P0 | 明确单图矩形/多边形支持边界；补未知属性保护、真实预览和保存草稿 | 显示与真实配置一致，未支持配置不伪装成功，关闭重启后草稿可恢复。 |
| P1 | 补齐本机社区网页的管理操作 | 8 个项目设置区、账户/成员/令牌、通用任务查询和标签页、选中任务队列、JSON/COCO/YOLO 等导出真实读写。 |
| P1 | 模型/预标注、存储、Webhook | 所有管理操作真实接入 Django；预测能够显示、修正并保存，源/目标可同步。 |
| P2 | 原生标注引擎与常用控件 | 先实现 Image + Choices、Text/Labels、TextArea、Relations、关键点/椭圆/笔刷、OCR；每种配置读写与网页结果一致。 |
| P3 | 音视频、时间序列、PDF、多图和多模态 | 原生播放/定位/区域/同步交互，导入、标注、保存、重载和导出闭环。 |
| 单独立项 | 商业版工作流、质量分析、SSO/RBAC/工作区等 | 先确定实际后端/API 能力和产品范围，再开发对应桌面交互。 |

## 代码与验证证据

本轮是静态/页面核查，**没有重跑对用户项目有写入的功能测试**。已实现的证据包括当前代码路径与前序留存的测试：`tests/tst_live.cpp`、`tests/tst_ui.cpp`、`tests/tst_api.cpp`、`tests/tst_document.cpp`、`tests/tst_language.cpp`。最近 `build-msvc/i18n-live-results.txt` 显示真实 Django 全流程通过；这证明测试覆盖的子集，不证明本清单中的未实现模块。

| 代码 | 关键证据 |
|---|---|
| `qml/Main.qml` | 页面路由、账户只读信息、邀请/账户网页跳转、离开确认与主题。 |
| `qml/ProjectSettings.qml` | 只有 General、Labeling Interface、Danger Zone 三项。 |
| `qml/LabelingSetup.qml` | 两类基础模板、标签文本生成器、XML 文本框、固定示意图。 |
| `qml/ProjectPage.qml` | 固定 Default、有限查询/列、任务勾选、三个批量操作和 JSON 文件选择器。 |
| `src/projectapi.cpp` | 会话登录、项目 CRUD、文件/URL 导入、固定 JSON 导出。 |
| `src/taskworkflow.cpp` | 固定 query、显式 ID 集合、前后切换、提交继续、跳过、ground_truth/删除。 |
| `src/annotationdocument.cpp` | 标签白名单、images/controls 限制、形状/标签结果验证。 |
| `src/apiclient.cpp` | 只读取 annotations，未把 predictions/drafts 加载成可编辑状态。 |
| `qml/Workspace.qml`、`qml/AnnotationCanvas.qml` | 固定快捷键、单选区域、矩形/多边形交互和固定面板。 |
| `src/languagemanager.cpp` | 标准翻译器、语言切换/记忆、重翻译现有对象。 |

后端与网页版本差异的源代码参考：`../label-studio/web/apps/labelstudio/src/pages/Settings/GeneralSettings.jsx`、`DangerZone.jsx`；`../label-studio/label_studio/data_manager/actions/basic.py`、`predictions_to_annotations.py`；`../label-studio/web/libs/editor/src/tags/{object,control,visual}/index.js`。

## 文档来源索引

原始公共文档和目录采集保存在 `build-msvc/feature-audit/`，其中 `sources.json` 记录来源，`tags-links.json` 与 `templates-links.json` 记录完整目录。敏感凭据未写入清单或采集目录。

- [关于 Label Studio](https://labelstudio.com.cn/guide/get_started)
- [术语表](https://labelstudio.com.cn/guide/glossary)
- [版本对比](https://labelstudio.com.cn/guide/label_studio_compare)
- [企业版功能](https://labelstudio.com.cn/guide/enterprise_features)
- [快速入门](https://labelstudio.com.cn/guide/quick_start)
- [安装](https://labelstudio.com.cn/guide/install)
- [使用 Kubernetes 安装](https://labelstudio.com.cn/guide/install_k8s)
- [设置 Ingress 控制器](https://labelstudio.com.cn/guide/ingress_config)
- [隔离网环境（Airgapped）部署](https://labelstudio.com.cn/guide/install_k8s_airgapped)
- [可用 Helm 参数](https://labelstudio.com.cn/guide/helm_values)
- [故障排除](https://labelstudio.com.cn/guide/install_troubleshoot)
- [设置数据库](https://labelstudio.com.cn/guide/storedata)
- [设置持久化存储](https://labelstudio.com.cn/guide/persistent_storage)
- [启动 Label Studio](https://labelstudio.com.cn/guide/start)
- [升级 Label Studio](https://labelstudio.com.cn/guide/upgrade_community)
- [保护 Label Studio 安全](https://labelstudio.com.cn/guide/security)
- [创建项目](https://labelstudio.com.cn/guide/setup_project)
- [配置标注界面](https://labelstudio.com.cn/guide/setup)
- [管理项目](https://labelstudio.com.cn/guide/manage_projects_lso)
- [项目设置](https://labelstudio.com.cn/guide/project_settings)
- [数据管理器](https://labelstudio.com.cn/guide/manage_data)
- [标注指南](https://labelstudio.com.cn/guide/labeling)
- [快捷键](https://labelstudio.com.cn/guide/hotkeys)
- [跳过任务](https://labelstudio.com.cn/guide/skip)
- [添加项目存储](https://labelstudio.com.cn/guide/storage)
- [AWS S3](https://labelstudio.com.cn/guide/storage_s3)
- [Google Cloud](https://labelstudio.com.cn/guide/storage_gcp)
- [Microsoft Azure](https://labelstudio.com.cn/guide/storage_azure)
- [Databricks](https://labelstudio.com.cn/guide/storage_databricks)
- [Redis](https://labelstudio.com.cn/guide/storage_redis)
- [本地存储](https://labelstudio.com.cn/guide/storage_local)
- [导入数据](https://labelstudio.com.cn/guide/tasks)
- [格式化导入数据](https://labelstudio.com.cn/guide/task_format)
- [导入预标注数据](https://labelstudio.com.cn/guide/predictions)
- [导出标注结果](https://labelstudio.com.cn/guide/export)
- [机器学习集成](https://labelstudio.com.cn/guide/ml)
- [编写自定义机器学习后端](https://labelstudio.com.cn/guide/ml_create)
- [机器学习示例与教程](https://labelstudio.com.cn/guide/ml_tutorials)
- [添加用户](https://labelstudio.com.cn/guide/signup)
- [管理用户账号](https://labelstudio.com.cn/guide/admin_manage)
- [账号与设置](https://labelstudio.com.cn/guide/user_account)
- [访问令牌](https://labelstudio.com.cn/guide/access_tokens)
- [API 参考](https://labelstudio.com.cn/guide/api)
- [SDK 参考](https://labelstudio.com.cn/guide/sdk)
- [Webhooks](https://labelstudio.com.cn/guide/webhooks)
- [Webhook 开发](https://labelstudio.com.cn/guide/webhook_create)
- [Webhook 事件参考](https://labelstudio.com.cn/guide/webhook_reference)
- [前端集成参考](https://labelstudio.com.cn/guide/frontend_reference)
- [故障排除](https://labelstudio.com.cn/guide/troubleshooting)
- [文档](https://labelstudio.com.cn/guide/)
- [模板](https://labelstudio.com.cn/templates/)
- [标签](https://labelstudio.com.cn/tags/)

可筛选表格：[CSV](feature-audit-2026-10-16.csv)；结构化清单：[JSON](feature-audit-2026-10-16.json)。
