# 重来一局布局回归（0.1.7）

用户反馈：完成游戏后重来，十人房间顶部角色框重叠；全新打开正常。

## 原因与修复

MainWindow::enterRoom 将 UI/ExpandDashboard 同步到复用的 QAction，原实现依赖 toggled 信号调用新 RoomScene 的 adjustDashboard。首次进入时 false→true 会发信号；之后 setChecked(true) 不再发信号，新场景保留默认手牌区宽度。CircularView 的角色横向间距又由此宽度计算，因而随之压缩。

每次进入房间显式调用 adjustDashboard，菜单仍支持后续手动切换。新场景安装到视图后，按当前视图 transform 调整位置，避免默认单位变换与实际缩放不一致。未修改游戏规则。

## TDD 与验证

新增 repeatedRoomKeepsExpandedLayout：1600×1000 窗口、十人环形布局，连续三次调用实际 enterRoom，比较手牌区矩形与全部九个角色矩形，并检查角色框两两不相交。测试通过空回放客户端隔离网络，覆盖重连成功后的场景创建路径，不冒充完整人工结束对局。

修复前第二次进入：手牌区宽度由 1590 退回 1037，断言失败；修复后通过。原生 Cocoa 平台同一测试通过。

Debug、Release 各 11 组代码回归通过（既有 assets.original_audio 原始素材元数据测试仍排除）。日志位于 test-results/20261016-restart-layout/。

交付为 dist/0.1.7，未替换用户已安装的应用；请在新版本中完成一局后点击“重来一局”作最终人工验收。

打包后启动、四项关闭及五种模式 AI 完整对局共 10 项检查通过；47 个 Mach-O 均为 arm64，深度签名验证通过。包与 dSYM UUID 一致：68B27AB5-BC85-3E46-BDED-7F8AB0994DFF。
