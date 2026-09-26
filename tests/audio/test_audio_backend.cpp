#include <QtTest>
#include "platform/audio/audio_backend.h"
class AudioTest: public QObject {
 Q_OBJECT
private slots:
 void decodeMixAndLifetime() {
  qs::AudioBackend audio(true); QVERIFY2(audio.ready(), qPrintable(audio.error()));
  const QString sample=QStringLiteral(QS_ASSET_ROOT "/audio/skill/tianyi.ogg");
  audio.play(sample,1.0f); audio.play(sample,1.0f); audio.waitForDecoders();
  QCOMPARE(audio.voiceCount(),1); QVERIFY(audio.render(4800));
  audio.stop(); QCOMPARE(audio.voiceCount(),0);
  audio.play(sample,0.5f); audio.waitForDecoders(); QCOMPARE(audio.voiceCount(),1);
  audio.shutdown(); QVERIFY(!audio.ready());
 }
 void missingAndCancelled() {
  qs::AudioBackend audio(true); QVERIFY(audio.ready());
  audio.play("/nonexistent/qs-audio.ogg",1); audio.waitForDecoders(); QCOMPARE(audio.voiceCount(),0);
  audio.play(QStringLiteral(QS_ASSET_ROOT "/audio/skill/tianyi.ogg"),1); audio.stop(); audio.waitForDecoders(); QCOMPARE(audio.voiceCount(),0);
 }
 void bgmLoopAndStop() {
  qs::AudioBackend audio(true); QVERIFY(audio.ready());
  audio.playMusic(QStringLiteral(QS_ASSET_ROOT "/audio/skill/tianyi.ogg"),0.2f); audio.waitForDecoders();
  QVERIFY(audio.musicPlaying()); for(int i=0;i<60;i++) QVERIFY(audio.render(4800));
  QVERIFY(audio.musicPlaying()); audio.setMusicVolume(0); audio.stopMusic(); QVERIFY(!audio.musicPlaying());
 }
};
QTEST_APPLESS_MAIN(AudioTest)
#include "test_audio_backend.moc"
