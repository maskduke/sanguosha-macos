# 太阳神三国杀·涅槃版 — Apple Silicon 移植

目标为 macOS 26 / arm64，保留 20120715 原装内容、单机 AI 及新版之间 LAN／直接 IP 对战。

**当前代码版本：0.1.3（macOS 26 / Apple Silicon）。** 已在本机生成并验证原生试玩版；本仓库发布源代码，不包含 `dist/` 中的应用或 ZIP。已有本地构建可打开 `dist/0.1.3/QSanguosha-Nirvana.app`。详见 [手动验收说明](MANUAL_ACCEPTANCE.md)。原版资源和规则主体保留，完整正式验收仍在进行。

- [设计](DESIGN_SPEC.md) · [测试设计](TEST_SPEC.md) · [用户决策](DESIGN_DECISIONS.md)
- [0.1.3 恩怨与深色模式修复](docs/validation/20261011-enyuan-theme.md)
- [0.1.2 重复开局修复](docs/validation/20260926-restart-fix.md)
- [0.1.1 用户反馈修复](docs/validation/20260926-feedback-fixes.md)
- [首版实施报告](docs/validation/20260926-native-preview.md)
- [源码与资源来源](docs/baseline/source-provenance.md)
- [已发现原装音频问题](docs/known-issues/LEGACY-AUDIO-001.md)

## 本机运行测试

当前已验证工具：Apple Clang 21、CMake 4.3.3、Ninja 1.13.2、Qt 6.11.1、libvorbis 1.3.7／libogg 1.3.6；Lua 使用仓库中的历史 5.1.4 源码，SWIG 4.4.1。开发预设指向本机 Homebrew 路径；交付 APP 已携带依赖。`dependencies.lock.json` 记录本机依赖快照并由打包脚本校验。

```sh
git clone https://github.com/maskduke/sanguosha-macos.git
cd sanguosha-macos
# 先按下文“源码、资源和证据”准备 assets/game/
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

`macos-arm64-release` 使用 RelWithDebInfo 构建交付 APP。可用 `tools/package_macos.py --source build/macos-arm64-release/QSanguosha.app --destination dist/新构建/QSanguosha-Nirvana.app` 打包；已有交付目录不会被覆盖。

## 源码、资源和证据

历史代码及许可已纳入 Git；迁移兼容修复及原 AI 参数补丁在实施报告中逐项列出。原装素材在 `assets/game/`，原应用独立快照在 `.local/original/QSanguosha.app`，取证仓库／下载在 `.research/`，三者均不提交到 Git。新机器仅克隆代码不足以运行资源测试，需要按来源清单取得并验证资源；资源准备自动化仍待实施。

`test-results/20260926-m0-tdd/` 保留 Python 工具的 RED/GREEN 日志；`test-results/20260926-m1-tdd/` 保留原生分帧 RED/GREEN、Debug／ASan 全量结果、环境和原应用未修改的复核记录。测试通过只覆盖所列组件，不代替 Test Spec 的游戏验收。

原项目采用 GPL-3.0，见 [LICENSE](LICENSE) 及各历史文件中的署名／许可。本次公开发布的是源代码及其文档，游戏素材和打包应用未随仓库上传。

完整历史代码的编译探针可用 `cmake --build --preset macos-arm64-debug --target qs_legacy_objects -- -k 0 -j 6` 执行，当前已通过并被真实游戏目标使用。
