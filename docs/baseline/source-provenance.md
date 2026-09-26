# 20120715 源码定位与导入

日期：2026-09-26。证据等级 B，M0 功能注册／可玩性检查仍进行中。

在 [yedezhanghao/QSanguosha 历史 fork](https://github.com/yedezhanghao/QSanguosha/tree/323f112658b5ea0c081b02e240afb92af7a379c1) 找到保留的涅槃版分支。选取发布当晚提交 `323f112658b5ea0c081b02e240afb92af7a379c1`（2012-07-15 22:51:45 +0800）。它替代先前仅供调研的 `fe5baeb...` 候选。

| 核对项 | 结果 |
|---|---|
| 本地同路径资源对历史 Git blob | 2,372 个完全一致，0 个内容不同 |
| 本地仅有文件 | 主程序、编译翻译、字体、9 个 Finder 元数据文件 |
| 字体 | `font/FONT.TTF` 与历史 `font/font.7z` 中原文件完全一致 |
| 源码 `src` tree | `7507d501f70dcef8ac17d8a21b531ba05473d54f` |
| 相邻提交 | `369893848663a5e6dbf998fd1887b9edee8e054e`、`184e196ca01a444997d5e3acbe0ae9f68a41f08d` 的 src tree 也相同 |
| 本地原样导入提交 | `a456ddd`；其 `src` tree 与历史 tree 完全相同 |
| 完整原应用快照 | 2,571 条目录项，2,422 个普通文件；复制后校验一致 |

源码 tar.gz SHA-256：`ce8cba0f55815682d74658943ed528cd12539a5b74cb5341b0613a7c20b1b6e0`。

字体 SHA-256：`373c0b1e267077567d0a0ff659969ff871d8b0231e8dfbb2a785bbc8918e029f`。

详细证据：[baseline.json](baseline.json)、[资源匹配清单](source-match-323f1126.json)、[原应用文件清单](original-manifest.json)。文件级匹配工具按 Git blob 算法比较实际内容，而不是只看文件名、版本号或发布日期。

这些证据足以提供可靠的历史移植起点，不能证明旧可执行文件每一条编译指令对应这次源码，也不能替代旧版运行对照。后续 M0 仍须冻结全部菜单、角色／技能／卡牌注册表、默认启用状态和模式可用性。Lua 探针只核对配置声明：26 个包、6 个命名场景及 33 个 MiniScene，不将其写成已验证可玩数量。

导入时保留 `src/`、`swig/`、`include/`、`resource/` 原文件；旧 qmake 工程和构建脚本存入 `docs/baseline/upstream/`，用于查证，不作为现代构建入口。新平台组件写入 `src/platform/`。
