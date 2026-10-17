# Label Studio 原生桌面端设计

2026-10-16。采用用户确认的全原生 QML 前端、保留 Django 后端，Qt Quick Controls 使用用户选择的 Material style。

## 首阶段

独立的 C++17 / Qt Quick 工程，最低 Qt 6.6.3。首阶段支持一个 Image 和一个 RectangleLabels 的图片标注配置，完成服务连接、项目和分页任务列表、图片加载、矩形绘制/移动/缩放/删除、撤销/重做、标注读取和提交。所有按钮、输入框、列表选择器和对话框使用 Qt Quick Controls；矩形和图片画布使用 Qt Quick 场景图。支持 Material 浅色/深色主题。

## 模块

- ApiClient：REST 请求、传统 Token / JWT refresh token 认证、分页、错误提示和受保护图片读取。
- AnnotationDocument：解析 XML 配置、百分比矩形坐标、结果 JSON、编辑历史、脏状态以及原始标注 ID。
- QML：连接页、项目/任务导航、图片画布、标签和区域面板。提供内置示例以便未启动后端时检查交互。
- ImageProvider：把已经过鉴权下载并解码的图片交给原生 QML Image。

## 数据约束

标注矩形使用 Label Studio 的 0–100 百分比坐标，输出保留区域 ID、from_name、to_name、type、原图尺寸和 rotation=0。修改已有标注调用 PATCH /api/annotations/{id}/，新标注调用 POST /api/tasks/{id}/annotations/。保存成功才清除脏状态。禁止在切换任务、项目、标注或退出时静默丢失修改。

首阶段不支持旋转、多图片、多种控件、分割、音视频标注；遇到不支持的配置或结果整份拒绝编辑和保存，避免遗漏已有数据。多份标注允许显式选择。认证凭证只驻留内存，配置仅保存服务器地址和主题。图片凭证仅发送到后端同源地址，跨源图片不附带凭证。API 请求有超时且任务切换受忙状态限制。

## 验收

Qt Test 验证 XML 解析、反向拖拽/边界、百分比 JSON 往返、历史与脏状态、不支持数据保护，以及本地 HTTP 服务下的认证、分页、POST/PATCH 和图片鉴权。构建真实 Qt 可执行文件，离屏启动检查 QML 组件加载，再以桌面截图检查布局。真实 Django 连接取决于本机服务和可用凭证，若缺少则明确记录验证范围。
