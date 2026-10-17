# 界面翻译维护

英文为源语言，简体中文目录为 `desktop_zh_CN.ts`。QML 界面使用 `qsTr()`，C++ 客户端提示使用所属 QObject 类的 `tr()`，格式参数使用 `%1`、`%2` 等。用户数据、标签、路径、路由及 XML 不进入翻译目录。

在桌面工程目录执行（按实际 Qt 安装路径调整）：

```powershell
& 'E:/Qt/Qt6.12/6.12.0/msvc2022_64/bin/lupdate.exe' qml src -ts translations/desktop_zh_CN.ts -source-language en_US -target-language zh_CN -no-obsolete
```

使用 Qt Linguist 或文本编辑器补齐新增翻译，保留占位符，然后执行 `./build.ps1 -Test -Deploy`。构建使用 LinguistTools 的 lrelease，将应用译文及 Qt 自带的 `qtbase_zh_CN.qm`、`qtdeclarative_zh_CN.qm` 嵌入程序，分发无需单独复制 QM 文件。Qt 套件必须包含上述工具及翻译文件。

运行时 `LanguageManager` 安装/移除 QTranslator 并调用 `QQmlEngine::retranslate()`，不重建页面或后端会话。C++ 计算出的动态文本要在 QML 绑定中依赖 `languageManager.code`；下拉框的模型刷新需按语义 ID 保留选择。打开后的对话框若命令式修改标准按钮文本，使用 `Qt.binding` 保持翻译绑定。

新增 UI 文本后检查目录中无 `unfinished` 或空译文、占位符一致，并在两种语言下检查窄窗口及打开中的对话框。语言状态测试位于 `tests/tst_language.cpp` 和 `tests/tst_ui.cpp`。
