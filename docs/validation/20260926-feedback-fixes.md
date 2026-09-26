# 0.1.1 用户反馈修复（2026-09-26）

交付目录：`dist/0.1.1/`，原 0.1.0 包保留。应用版本 0.1.1 / build 2；匹配 dSYM 同目录。新旧客户端规则指纹不同，联机双方需使用新包。

## 修复

1. 手牌悬停提升至相邻手牌之上；移开恢复原层级，悬停期间重新排序会更新恢复层级；出牌移出手牌区时清理悬停状态。放大效果改为围绕原卡中心按逻辑坐标绘制，保留完整图像和 Retina 密度。
2. 未选目标的 SentbackEffect 原先新建 QImage 丢失 devicePixelRatio，Retina 下按两倍尺寸绘制暗影。现在保留图像密度，每次按当前源更新灰度图，且保存/恢复 painter 状态；避免旧图在内容或屏幕密度变化后继续使用。保留原来仅将不可选目标变暗的交互。
3. 用户本次报告为 RoomThread → Shien::trigger（wisdompackage.cpp:831）→ Player::isAlive，对 0xc4 地址访问。技能持有者查询可返回空指针，代码原先直接解引用。现在无有效持有者时不触发，执行入口也保护空持有者/空卡牌。这与首版退出时 Engine 析构竞态是不同原因。

## TDD 与验证

证据目录 `test-results/20260926-feedback/`。

- `02-red.log`：原实现的悬停层级、施恩触发条件失败。初始 Retina 测试同时存在离屏渲染目标尺寸误设，不能单独作为根因证据。
- 校正为明确的 200×200 逻辑目标后，临时恢复原 sprite 实现复测；`10-original-render-red.log`：普通屏通过、Retina 的目标外像素变灰，确证旧实现越界。随后恢复修复代码。
- Debug / Release 的 10 组已实现回归全部通过（`12-debug-tests.log`、`14-release-tests.log`）。保留已知的原天义音效资源失败，未将其标为通过；本次未改动音频资产，未重复该资产测试。
- 新增 6 个实际测试用例：悬停层级/恢复/重排；普通屏及 Retina 下暗影边界；普通屏及 Retina 下放大图像完整性；无持有者时真实施恩技能对 CardUsed、CardResponsed、空卡牌的安全返回。
- 原生 Cocoa 后端复验上述 6 例通过（`17-cocoa-tests.log`）；AddressSanitizer 下也通过（`16-asan-tests.log`）。`detect_leaks=0`，不作无内存泄漏承诺。

本次未声称复现用户那局完整操作序列或验证全部技能组合。原 ASan 八人长局超时、双机 LAN 和长时间稳定性验收仍沿用首版报告中的未完成状态。

## 最终交付验证

- 最终包启动、4 类退出及 2 人/4 人/8 人/3v3/KOF 各 1 局完整 AI 对战，共 10 项全部通过（`packaged-game-checks/summary.json`）。
- 47 个 Mach-O 均为 arm64，独立依赖审计和递归签名验证通过。GUI 已在新路径启动并显示主界面。
- dSYM 与可执行文件 UUID 相同：`8B39CD43-158E-3EB7-A77F-88C85AFE65E2`。
- ZIP SHA-256：`631836197815e1b3736f27ffe3df84c10d3abb07fd4c0286e9d26b7dbbb0c7fb`。规则指纹等见 `delivery.json`。
