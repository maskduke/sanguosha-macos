# 太阳神三国杀·涅槃版 — Apple Silicon 移植

本地项目：`/Users/haoranliu/Projects/sanguosha`。目标为 macOS 26 / arm64，保留 20120715 原装内容、单机 AI 及新版之间 LAN／直接 IP 对战。

**目前是 M0/M1 工程阶段，尚无可试玩的新版 APP。** 已保存匹配历史源码，并建立基线工具、原生 Lua、协议分帧和原装音频解码测试。玩法、UI、SWIG 完整绑定、音频输出和打包尚未贯通。

- [设计](DESIGN_SPEC.md) · [测试设计](TEST_SPEC.md) · [用户决策](DESIGN_DECISIONS.md)
- [当前实施报告](docs/validation/20260926-tdd-progress.md)
- [源码与资源来源](docs/baseline/source-provenance.md)
- [已发现原装音频问题](docs/known-issues/LEGACY-AUDIO-001.md)

## 本机运行测试

当前已验证工具：Apple Clang 21、CMake 4.3.3、Ninja 1.13.2、Qt 6.11.1、libvorbis 1.3.7／libogg 1.3.6；Lua 使用仓库中的历史 5.1.4 源码。预设暂指向本机 Homebrew 路径，最终依赖锁及独立 APP 打包仍待完成。

```sh
cd /Users/haoranliu/Projects/sanguosha
python3 -m unittest discover -s tests -v
cmake --preset macos-arm64-debug
cmake --build --preset macos-arm64-debug
ctest --preset macos-arm64-debug --verbose
```

完整 CTest **当前会返回非零**：`assets.original_audio` 中 `tianyi.ogg` 的长度检查失败，详情见问题记录。没有将原资源异常改成 PASS 或默认跳过。只运行已实现的协议／Lua 组件：

```sh
ctest --preset macos-arm64-debug -R '^(protocol|bindings)\.' --verbose
```

ASan：

```sh
cmake --preset macos-arm64-asan
cmake --build --preset macos-arm64-asan
ctest --preset macos-arm64-asan --verbose
```

`macos-arm64-release` 预设已定义，但还未作为交付构建验证。当前所有目标都是测试组件，不能将生成的测试可执行文件当作游戏。

## 源码、资源和证据

历史代码及许可已纳入本地 Git；规则／AI 没有进行玩法修改。原装素材在 `assets/game/`，原应用独立快照在 `.local/original/QSanguosha.app`，取证仓库／下载在 `.research/`，三者均不提交到 Git。新机器仅克隆代码不足以运行资源测试，需要按来源清单取得并验证资源；资源准备自动化仍待实施。

`test-results/20260926-m0-tdd/` 保留 Python 工具的 RED/GREEN 日志；`test-results/20260926-m1-tdd/` 保留原生分帧 RED/GREEN、Debug／ASan 全量结果、环境和原应用未修改的复核记录。测试通过只覆盖所列组件，不代替 Test Spec 的游戏验收。

原项目采用 GPL-3.0，见 [LICENSE](LICENSE) 及各历史文件中的署名／许可。资源仅用于用户授权的本地移植，尚未公开发布。
