# macOS 26 / Apple Silicon 原生试玩版验证

本轮目标是可让用户手动验收的独立应用，不代表 Design Spec / Test Spec 全部正式验收完成。

## 交付

- 应用：`dist/QSanguosha-Nirvana.app`；ZIP 与 SHA-256 文件在同目录。
- 构建：0.1.0 / preview.1，arm64，macOS 26.0 起；Qt 6.11.1，Lua 5.1.4，SWIG 4.4.1，miniaudio 0.11.21，Vorbis 1.3.7。
- 使用匹配基线 `323f112658b5ea0c081b02e240afb92af7a379c1` 的原画面、字体、音乐、卡牌、武将和 AI；未引入新版武将或重做 UI。
- 打包为本地 ad-hoc 签名；没有 Developer ID 公证，没有公开发布。
- `dist/QSanguosha-Nirvana.app.dSYM` 保留崩溃符号，后续反馈请一并提供模式、武将、操作顺序及系统报告。

## 本轮实现与差异

| 范围 | 实现与边界 |
|---|---|
| 完整应用 | 全部历史 C++ 模块及 SWIG 游戏绑定已编译、链接；Qt Widgets / Graphics View 取代旧 Qt 4 API。致谢页按原 QML 的同一图片、坐标和返回操作改为 Graphics View。 |
| 内容启动检查 | 178 张卡牌、135 个非隐藏武将、26 个扩展注册项；33 个小型场景均可加载且人数不少于 2。原配置仍声明 39 个场景项，含命名场景和自定义场景。 |
| 音频 | 原 OGG 直接解码，后台线程加载，Core Audio 混音；同文件重复播放限制、循环、停止和资源释放有测试。原 tianyi 文件未修改。 |
| 数据目录 | 新配置写入 `~/Library/Application Support/QSanguoshaNirvana/config.ini`；自定义场景和默认数据库写入新数据目录。应用资源从自身 bundle 定位。 |
| 联机 | 实际 socket 已接入增量分帧；消息用拥有数据的 QByteArray 跨信号传递；JSON/TCP 统一 UTF-8，保留字符串中的空格，1 MiB 上限，拒绝无效/过长消息；新增 qs-modern/1、基线和规则哈希检查。单机监听 loopback，显式服务器保留 LAN 监听。 |
| 录像 | 修复 PNG 图像引用临时内存、容量单位错误及空/损坏图片崩溃；新增中文内容回读、伪造长度与空录像测试。旧个人录像兼容仍不作为验收条件。 |
| 原脚本兼容 | CardUseStruct 的 `broken` 临时标志提供 SWIG 可写字段；旧 AI 两处调用未传最小弃牌数，现默认等于请求弃牌数。后者为独立 `patches/runtime-lua.json`，只应用于运行资源副本。 |
| 随机数 | 保留 qrand/qsrand 调用边界及范围，底层使用线程局部 QRandomGenerator；不承诺与旧 Qt 4 的随机序列一致。 |

其他编译阻断修复包括 Qt 6 容器/API 调整，以及自定义场景编辑器中将布尔值误赋给国籍字符串的旧笔误。原 `assets/game` 内容与 `/Applications/QSanguosha.app` 均未被上述运行补丁改写。

## 崩溃报告与修复

用户提供的 ARM64 报告显示：主线程执行 `Engine::~Engine()`，3v3 的 `RoomThread::trigger()` 同时访问技能对象，造成 use-after-free。此前测试的 3v3 结算后循环没有停止，300 ms 后退出又触发提前释放。

已补上 3v3/虎牢关结束条件、停止后的阶段/事件保护，以及退出时先唤醒等待、等待 Room/选将/对局线程结束，再释放引擎资源的顺序。停止状态使用原子变量，补齐未初始化的 KOF 线程指针。测试模式等待线程真正结束；批测首次失败立即停止。极端情况下若退出等待超时，进程直接退出并跳过不安全析构，输出明确诊断；没有使用 QThread::terminate。

对应证据：`35-3v3-green.log`、`shutdown-checks/`、`49-ui-final.log`，以及后续独立包对局结果。调试窗口在等待玩家操作时执行菜单退出，进程返回 0。完整报告没有作为公开资料上传。

## 测试证据

全部原始结果在 `test-results/20260926-preview/`，RED/失败尝试保留，未覆盖。

- Python 基线工具：13 项通过，`69-python-tests.log`。
- Debug / RelWithDebInfo / ASan：9 组已实现的组件/实际 socket 回归通过，见 `53-debug-tests.log`、`55-release-tests.log`、`58-asan-tests.log`。ASan 配置 `detect_leaks=0`，不能据此声称无内存泄漏。
- `51-ai-discard-red.log` 确认原 AI 缺省参数异常；补丁后的同一函数回归通过。
- 独立包包含 47 个 Mach-O，全部仅 arm64；依赖路径扫描、递归代码签名校验通过，见 `66-package.log`。包内 `Resources/dependency-audit.json` 记录各库。
- 中文/空格目录搬迁启动通过；dyld 加载日志没有 Homebrew 路径，见 `68-relocation-smoke.log`。
- 交付包 **20 局全部通过**（2 人、4 人、8 人、3v3、KOF 各 4 局），另有启动及 4 类退出检查通过，见 `packaged-game-checks/summary.json`。完整游戏 ASan 的 2 人、4 人局及退出检查通过，但八人局在 **180 秒超时**，批测停止，见 `asan-game-checks-final/summary.json`；没有输出 ASan 内存错误，仍不能判为通过。这些是完整胜负流程/稳定性检查，不是逐技能旧版等价性证明。
- 补充的 ASan 3v3、KOF 完整对局均通过，分别耗时 22.54 秒和 12.96 秒，见 `asan-remaining/summary.json`；这不改变八人局超时的未通过状态。
- 初始八人局有两次触及测试的 60 秒上限，记录仍保留。后续独立八人复测正常结束（23.38 秒）；采样显示正在执行 Lua AI / 规则逻辑。测试上限调整为 180 秒，不修改实际玩法速度。不能将初始超时改写成 PASS。
- `70-original-unchanged.json` 再次确认原应用路径、文件内容、模式和符号链接无变化。

## 尚未完成的正式验收

- `assets.original_audio` 仍有 **1 个原资源异常**：tianyi.ogg 元数据长度与实际 EOF 解码长度不一致，见 LEGACY-AUDIO-001。完整 CTest 因而仍返回失败；没有豁免或隐瞒该项。新音频后端已对这个实际文件验证解码、PCM 输出、循环及释放。
- **仍需排查 ASan 八人局超时**：提高上限后仍出现，不能只归因于早先 60 秒门槛。交付包同类 4 局均完成，但不等于已解释该差异。
- 尚无第二台 Mac，未完成双设备 LAN 三局；已有真实 TCP 层测试及本机单机客户端/服务端贯通，不能代替跨机验证。
- 用户三局手动验收、两小时稳定性、音频实听/设备切换/睡眠唤醒、1× 显示、原装工具完整矩阵和逐技能黄金样本仍待执行。
- 缺少可运行的旧系统参考环境，不能宣称所有细节已证明 1:1；目前依据匹配源码/资源及原规则运行。
- 依赖锁记录本机精确版本、上游 formula 声明的源码哈希和安装收据；打包强制校验收据。它还不是自动下载完整工具链的 hermetic 构建系统。

本交付仅供用户按原计划先试玩和反馈，不将上述未完成项目标为通过，也不请求将其永久豁免。

## 最终交付标识

ZIP SHA-256：`7f4190f1ac15aa3e4e8c0647bb21636321f5712cdb25ac802e58300bcd378003`。构建/规则哈希/检查明细见 `test-results/20260926-preview/delivery.json`。实际窗口已验证双击甘宁选将、取消闪响应受伤、使用桃回血，以及无操作时限时从菜单退出（返回 0）。
