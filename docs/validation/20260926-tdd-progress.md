# TDD 执行报告 — 2026-09-26

状态：M0/M1 已开始，完整游戏尚未构建，无可试玩 APP；未通过最终验收。

项目已位于 `/Users/haoranliu/Projects/sanguosha`，分支 `codex/macos-arm64-port`。原始导入提交 `a456ddd` 保存未经平台修改的历史源码。新增组件不会改变旧玩法源码。

## 已完成及证据

| 工作 | 实际结果 | 证据 |
|---|---|---|
| 基线工具 TDD | 先记录缺失模块失败，再实现；13 项测试通过 | [M0 日志](../../test-results/20260926-m0-tdd/)、[当前回归](../../test-results/20260926-m1-tdd/13-python-regression.log) |
| 原应用保护与资源匹配 | 独立快照一致；2,372 个同路径资源零内容差异；原应用本轮结束前复核无变化 | [来源报告](../baseline/source-provenance.md)、[未修改复核](../../test-results/20260926-m1-tdd/original-unchanged.json) |
| arm64 构建基础 | CMake/Ninja 编译原版 Lua 与新协议组件；三项测试程序均为原生 arm64 | [环境及二进制哈希](../../test-results/20260926-m1-tdd/environment.json) |
| 网络分帧 TDD | 无操作实现先触发 10 项失败；实现后 10 项通过 | [RED](../../test-results/20260926-m1-tdd/03-red.log)、[GREEN](../../test-results/20260926-m1-tdd/05-green.log) |
| 原 Lua 5.1 运行探针 | 原装配置执行、版本中文、26 包顺序、39 场景顺序、错误回传及 GC 通过，2 项测试 | [GREEN](../../test-results/20260926-m1-tdd/05-green.log) |
| 原装音频全量探针 | 477 个文件均匹配原哈希；476 项完整解码／长度断言通过，1 项长度不一致；非 Vorbis 拒绝测试通过 | [原始记录](../../test-results/20260926-m1-tdd/09-debug-audio-diagnosed.log)、[已知问题](../known-issues/LEGACY-AUDIO-001.md) |
| ASan | 协议和 Lua 测试通过；同一音频问题复现；本次日志无 AddressSanitizer 错误 | [ASan 全量记录](../../test-results/20260926-m1-tdd/12-asan-tests.log) |

计数不包含 QtTest 自动生成的 initTestCase／cleanupTestCase。原生 CTest 当前为 2 个 suite PASS、1 个 suite FAIL，不能表述为全绿。ASan 仅覆盖已构建组件及历史 Lua 源码；Homebrew 的 Qt／Vorbis 动态库本身未用 ASan 重编，也未测试游戏房间生命周期。

## 本轮实现契约

`LineFramer` 的 payload 上限为 1,048,576 字节，不含 LF；保留包括 CR、UTF-8 在内的原字节，分帧层不解释正文。覆盖所有字节切分点、逐字节输入、粘包、尾部残留、空帧、L−1／L／L+1、跨片超限、断线残帧、错误状态保持及 reset。

超限前检查长度，最多缓存一个上限内的未完成帧，错误后丢弃残留且拒绝后续输入，直至显式 reset。单次调用返回的已完成帧总量仍与输入大小成正比；后续 socket 接入须使用受限读取块，不能拿这个组件声称整个连接已实现资源限额。

该组件**尚未接入旧 NativeClientSocket**。旧协议编码、`char*` 信号生命周期、正文解析、握手及真实 TCP 集成仍待测试；1 MiB 仍是设计初值，尚未用全部合法游戏消息验证并冻结。

Lua 通过的是历史虚拟机和配置探针，未通过实际 SWIG QVariant／QObject 所有权、AI 加载及完整对局。音频通过的是解码探针，尚未实现 miniaudio／Core Audio 播放、混音和设备恢复。

## Test Spec 对应状态

- BASE-01：快照和哈希部分 PASS；M0 总门槛仍 IN_PROGRESS，内容注册／功能表待完成。
- BUILD：组件 Debug／ASan 可构建；完整游戏、依赖锁和独立打包 NOT_RUN。
- NET-01／02：分帧组件子项 PASS；真实连接、正文验证、合法最大消息测量 NOT_RUN。
- LUA-01～04：配置探针只是前置验证，完整绑定／线程用例 NOT_RUN。
- AUDIO-01：476 文件通过，1 文件 FAIL，不能关闭此项。
- 游戏、UI、录像、真实音频、完整模式、20 局 AI、3 局用户试玩和 2 小时稳定性：NOT_RUN。
- 两台设备 LAN：未有第二台测试设备证据，验收 BLOCKED；不妨碍继续实现。

没有改变原用户范围，没有接受测试例外，也没有新的产品拍板项。后续顺序：补齐 M0 注册／功能依据 → SWIG 与 Qt6 游戏编译 → 路径及 socket 组件接入 → 标准模式固定规则 fixture 与完整对局。音频异常保留失败并随实际后端接入处理。
