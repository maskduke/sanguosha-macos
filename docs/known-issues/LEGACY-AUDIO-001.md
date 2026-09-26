# LEGACY-AUDIO-001 — 天义原装音效缺少结束标记且长度不一致

状态：未解决；来源：旧资源已有；严重度暂记 S2，实际播放影响待音频后端实听确认。关联：AUDIO-01、后续 AUDIO-02。无用户例外，不豁免验收。

文件：`audio/skill/tianyi.ogg`。

SHA-256：`85c61ea08f8a311022babb834a2a3f0276fb5704d5daba71e3d53524523733e6`。

旧应用、历史源码压缩包、新资源目录均为上述同一文件，共 70,905 字节。没有修改或转码原文件。

## 复现与观测

```sh
build/macos-arm64-debug/test_original_audio 'originalOgg:audio/skill/tianyi.ogg'
```

macOS 26.6.2 / arm64 / libvorbisfile 1.3.7：双声道，44,100 Hz，单逻辑流；`ov_pcm_total(-1)` 报告 178,816 帧，而循环 `ov_read` 到 EOF 返回 179,264 帧，相差 448 帧（约 10.16 ms）。Debug 和 ASan 得到相同结果，未出现解码函数错误码。

文件包含序号 0–18 的 19 个完整页面，但没有任何页面设置 EOS 位。按 [Ogg framing 规范](https://xiph.org/ogg/doc/framing.html)，页头 `0x04` 位标识逻辑流结束。结合长度不一致，判断这是历史流结束信息异常；不能仅据此断言缺了哪段语音或推断原录音应有长度。

[ov_pcm_total 官方定义](https://xiph.org/vorbis/doc/vorbisfile/ov_pcm_total.html)与 [ov_read 返回值定义](https://xiph.org/vorbis/doc/vorbisfile/ov_read.html)是测试的长度和字节换算依据。测试坚持对比元数据和实际解码，不把新解码出的 179,264 帧改成“原版正确长度”。

## 证据与后续处理

- [首次失败](../../test-results/20260926-m1-tdd/07-debug-all.log)
- [完整解码诊断](../../test-results/20260926-m1-tdd/09-debug-audio-diagnosed.log)
- [三处同哈希及页面信息](../../test-results/20260926-m1-tdd/tianyi-ogg-pages.json)
- [ASan 复现](../../test-results/20260926-m1-tdd/12-asan-tests.log)

后续播放实现必须流式读取并处理实际 EOF，不能按不可信时长预分配后越界写入。先保持原字节并验证可播放、停止及释放；完成实听后再判断是否需要资源修复。替换音效、删音效或接受必需测试例外，均须另列具体方案供用户决定；目前未做这些变更。
