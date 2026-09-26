# Qt6／SWIG 第二轮 TDD 执行报告

日期：2026-09-26。M0/M1 继续进行；完整游戏编译仍为 FAIL，尚无可试玩 APP。

## 本轮完成

1. 建立完整历史 C++／SWIG 的 `qs_legacy_objects` 编译探针，自动生成 SWIG、moc、uic 文件。该目标只生成对象文件；没有链接游戏，也没有以关闭音频的方式交付游戏。
2. 用游戏实际使用的 `swig/naturalvar.i` 编译 Lua 测试模块，发现并修复 SWIG 4 生成代码中的局部变量重名、goto 跨初始化问题。修复仅增加局部作用域。
3. 将原 QVariant 基础声明提取到 `swig/qvariant_core.i`，游戏与测试共用。18 项测试覆盖 QString／QStringList 中文、空字符串／列表、拒绝 nil／bool／非法元素、数字转换、原 C 字符串 NUL 边界、QVariant 整数／布尔／中文及反复 GC。
4. 独立编译原 JSON／协议模块。先记录 Qt6 的 `toAscii`、类型别名重复重载失败，再修复；链接测试进一步发现旧 JsonCpp 缺少 `<istream>`，已补齐。9 项测试覆盖固定报文、错误报文、历史上限、数组顺序／失败时不部分写入、编码、倒计时及流读取。
5. 为 Qt6 元类型注册补齐完整 ServerPlayer 定义，为实际使用的 QRegExp 补齐头文件；场景 cpp 中的 Q_OBJECT 增加对应 moc 入口。

以上属于平台／绑定适配，未改卡牌、武将、场景的结算逻辑。

## 测试结果

| 测试组 | 有效测试实例 | Debug | ASan |
|---|---:|---|---|
| 增量分帧 | 10 | PASS | PASS |
| 原 Lua 运行／配置 | 2 | PASS | PASS |
| 实际 SWIG 值转换 | 18（新增） | PASS | PASS |
| 原 JSON／协议 | 9（新增） | PASS | PASS |
| 合计 | 39，其中本轮新增 27 | 4 suites PASS | 4 suites PASS |

不计 QtTest 自动生成的初始化／清理行。ASan 日志未报告本次受测代码的内存错误；不能据此关闭 QObject 生命周期、房间退出或线程验收。Qt 动态库本身未以 ASan 重编。

证据目录：`test-results/20260926-m1-bindings/`。

- SWIG RED：[05-binding-build.log](../../test-results/20260926-m1-bindings/05-binding-build.log)；第一轮 GREEN：[08-binding-green.log](../../test-results/20260926-m1-bindings/08-binding-green.log)。
- QVariant 为原代码特征验证：首次构建和执行即通过，没有伪造额外 RED。
- JSON RED：[13-json-red.log](../../test-results/20260926-m1-bindings/13-json-red.log)；链接失败：[14-json-build.log](../../test-results/20260926-m1-bindings/14-json-build.log)。
- 全组 Debug：[17-debug-green.log](../../test-results/20260926-m1-bindings/17-debug-green.log)；ASan：[20-asan-tests.log](../../test-results/20260926-m1-bindings/20-asan-tests.log)。
- [逐例记录](../../test-results/20260926-m1-bindings/cases.jsonl)、[受测源码集合哈希](../../test-results/20260926-m1-bindings/tested-source.json)。

音频未受本轮修改影响，沿用上一轮证据：476／477 通过，`tianyi.ogg` 保留 FAIL；本轮没有重跑音频，也没有豁免这个失败。直接执行不带过滤器的完整 CTest 仍会包含它。

## 编码和协议边界

本轮 JSON 特征测试基于冻结的 `jsonutils.cpp`／`protocol.cpp` 及 [Qt 历史 QString 文档](https://doc.qt.io/Archives/qt-4.7/qstring.html)：未设置全局 C-string codec 时，旧 `toAscii`／隐式 C 字符串转换使用 Latin-1。已检查当前匹配源码没有设置该全局 codec，因此以显式 `toLatin1`／`fromLatin1` 保留这个旧模块的边界；保真结论仍为源码等级 B，没有旧可执行程序运行对照。

这不表示新版中文通信已经验收：Lua 值转换按 Qt6 实际 UTF-8 行为验证，旧 JSON 模块的直接中文会有损失。新协议仍需统一两端编码、增加握手并接入真实 socket；不能把两个层次的测试混为中文端到端 PASS。

旧协议解析上限为 1,000 字节，新增分帧层上限初值为 1 MiB。二者尚未接通，不能宣称新版已支持 1 MiB 游戏报文。旧 `toString()` 还会删除报文中的空白并截断长消息，这些行为已识别但本轮没有更改；后续替换时需有独立失败样本和明确的迁移记录。

QStringList 在 Qt6 与 QList<QString> 为同一类型，因此保留一份同内容重载；调用两种类型的代码仍可使用该接口。

## 完整游戏尚未通过的部分

完整编译当前仍失败，详见 [编译日志](../../test-results/20260926-m1-bindings/21-legacy-current.log) 和 [结构化诊断](../../test-results/20260926-m1-bindings/legacy-compile-diagnostics.json)。当前观察到 191 个不同位置／信息的诊断，涉及 49 个路径；同一根因会引发多处诊断，且编译器有错误数量上限，因此这些数字既不是独立 bug 数，也不是完整迁移工作量。

下一批主要工作：

- 随机数适配及测试注入，替换已移除的 qrand／qsrand。
- Qt 容器、排序、空指针判断和旧字符串 API，逐处保持原语义。
- Graphics View／QMatrix、静态 QML 致谢页、计时器及旧 QHttp。
- 完整 SWIG 的游戏对象包装、所有权和房间 Lua 测试；当前只覆盖共享的基础值接口。
- 新 socket 分帧／握手／编码接入、路径隔离、音频后端及完整游戏链接。

M0 的完整内容注册表仍需在引擎可运行后冻结。规则、UI、AI 完整对局、LAN、录像和独立打包验收均未通过；无新产品范围决定或用户例外。

## 复跑入口

```sh
cmake --preset macos-arm64-debug
cmake --build --preset macos-arm64-debug
ctest --preset macos-arm64-debug -R '^(protocol|bindings)\.' --verbose

cmake --preset macos-arm64-asan
cmake --build --preset macos-arm64-asan
ctest --preset macos-arm64-asan -R '^(protocol|bindings)\.' --verbose

# 完整历史源码探针：当前预期编译失败，用于继续逐项修复。
cmake --build --preset macos-arm64-debug --target qs_legacy_objects -- -k 0 -j 6
```

默认构建仅要求当前可构建的组件／测试成功，完整源码探针独立执行，其失败在本报告显式保留；不能把默认构建成功写成“游戏已编译”。
