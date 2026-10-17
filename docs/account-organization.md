# 账户与组织：原生功能交付记录

日期：2026-10-16。以[账户与用户文档](https://labelstudio.com.cn/guide/signup)、本机 Label Studio 1.24.0.dev0 及[原始核查清单](feature-audit-2026-10-16.md)的 F001–F018 为范围。

使用原生 C++ / QML、Qt Quick Controls Material；数据写入现有 Django。登录后点击右上角账户菜单 →「账户与设置」或「成员与邀请」。登录页可切换注册；顶部支持简体中文 / English。

## 功能对照

| 编号 | 功能 | 当前实现与边界 |
|---|---|---|
| F001 | 邮箱密码登录 | 已实现。Django 会话、Cookie 和 CSRF。 |
| F002 | API Token / JWT 登录 | 已实现。Legacy、JWT Refresh / Access；账户写请求遇到 Access 过期可刷新后重试。 |
| F003 | 退出登录 | 已实现。会话登录调用 Django logout，成功后清除本地状态；Token 登录清除本地凭据，保留可复用令牌。 |
| F004 | 注册 | 已实现。邮箱、密码确认、可选邀请链接/令牌、邮件偏好，复用 Django 注册表单及服务器校验。 |
| F005 | 成员列表 | 已实现。分页显示邮箱、姓名、拥有者和最近活动；拥有者可确认移除其他成员。 |
| F006 | 成员活动与贡献 | 已实现。详情读取加入时间、标注数、参与项目数、创建及贡献项目；没有额外构造活动时间线。 |
| F007 | 邀请与复制链接 | 已实现。原生获取、显示、复制邀请链接。 |
| F008 | 重置邀请链接 | 已实现。二次确认后重置，显示新链接。 |
| F009 | 组织信息 | 已实现。名称、联系信息编辑及创建时间。 |
| F010 | 个人资料 | 已实现。名字、姓氏、电话修改；邮箱只读。 |
| F011 | 头像 | 已实现。上传、更换、移除；文件最大 1 MB，尺寸由 Django 校验不超过 1200 × 1200。 |
| F012 | 修改密码 | 已实现。旧密码验证、密码确认、Django 密码策略，修改成功保留当前会话。需配套本次新增的后端 API。 |
| F013 | 邮件偏好 | 已实现。读取、保存 allow_newsletters。 |
| F014 | 成员资格与贡献概要 | 已实现。组织、联系邮箱、加入日期、标注及项目贡献；统计取自后端。 |
| F015 | 个人访问令牌 | 已实现。列表、创建、撤销；可粘贴完整 Refresh Token 轮换。完整密钥仅创建/轮换时获得，既有列表返回的令牌缺少签名。 |
| F016 | 传统令牌 | 已实现。按组织策略加载、复制、重置。Legacy 不提供到期续期语义，重置后旧令牌失效。 |
| F017 | 组织令牌策略 | 已实现。PAT / Legacy 开关，确认后写入 Django。 |
| F018 | 快捷键 | 部分实现。账户默认值及个人项目覆盖、常用动作编辑、全部配置 JSON 编辑、网页格式导入/导出、恢复继承/默认值已实现；原生绑定限保存、提交、撤销、重做、删除区域、选择、绘制和图片平移。网页其他动作配置会保留，其对应原生功能仍待开发。 |

资料、组织、邮件偏好、令牌策略及快捷键编辑包含未保存保护；语言切换保留输入。保存失败不更新已保存基线。令牌默认遮盖，提供复制和清除；退出会清空账户状态。

社区版注册遵守现有 Django 的单组织规则：加入服务器第一个组织，邀请链接和禁用注册策略由服务器裁定。本次没有增加多组织切换、企业 RBAC 或 SSO。

## 后端改动

- 新增 `POST /api/current-user/password/`：仅当前已鉴权用户，校验旧密码及 Django 密码策略，更新会话认证哈希。
- 修正成员列表基础分支的 `active=true`：移除后的软删除成员不再出现在活动成员列表中；不改变默认完整列表。
- 无数据库迁移。使用其他 Django 安装时，需同步 `users/password_api.py`、`users/urls.py` 和 `organizations/api.py`。

## 验证与复现

实际使用 Qt 6.12.0 MSVC x64、Visual Studio 2026 构建；Qt 6.6.3 保持工程最低版本声明，未在该套件实测。

```powershell
# 桌面构建和自动测试
.\build.ps1 -Test

# 运行隔离账户真实往返测试并自动清理自己的测试账户/组织
.\tests\run-account-live.ps1

# 部署完整 Qt 运行目录
.\build.ps1 -Deploy
```

后端账户契约测试在 Django 临时测试数据库运行：

```powershell
# 工作目录：E:\projects\QML\label-studio
.venv\Scripts\python.exe label_studio\manage.py test users.tests.test_password_api --noinput
```

验证记录：

- `build-msvc/account-test.log`：language、document、api、native_ui、live_django 五组；真实联调用例未提供凭据时跳过。
- `build-msvc/account-backend-tests.log`：6 项通过，覆盖密码鉴权/策略/会话、活动成员列表、邀请注册及 CSRF。
- `build-msvc/account-live-results.txt`：隔离组织真实测试，覆盖资料、头像、成员详情/移除、邀请重置、组织、邮件偏好、快捷键、PAT 创建/轮换/撤销、Legacy 加载/重置、修改密码及退出后重新登录。
- 原生注册的邀请与表单编码由 API 测试覆盖；实际注册成功、非法邀请及缺失 CSRF 由 Django 临时测试数据库覆盖，未向用户现有组织注册测试成员。
- `build-msvc/account-screenshots/`：账户页中英文、1060 × 700 最小窗口、各分区截图；真实界面测试检查 QML 警告及语言切换保留输入。
- `build-msvc/account-deploy.log`、`build-msvc/account-startup.log`：Qt 运行目录部署完成；部署后的程序在 Windows 平台启动并保存登录截图，退出码 0。

测试仅操作带独立 QA 前缀的测试用户和组织，不修改用户原有项目、密码或令牌。
