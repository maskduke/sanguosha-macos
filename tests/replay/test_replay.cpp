#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include "recorder.h"
class ReplayTest: public QObject { Q_OBJECT
private slots:
 void pngRoundTrip(){QTemporaryDir d;QByteArray text="0 checkVersion 20120715\n10 speak ";text+=QStringLiteral("刘备 空 格").toUtf8();text+='\n';QImage image=Recorder::TXT2PNG(text);QVERIFY(!image.isNull());QVERIFY(image.save(d.filePath("record.png")));QCOMPARE(Replayer::PNG2TXT(d.filePath("record.png")),text);}
 void malformedImage(){QTemporaryDir d;QImage image(4,4,QImage::Format_ARGB32);image.fill(0xffffffff);QVERIFY(image.save(d.filePath("bad.png")));QVERIFY(Replayer::PNG2TXT(d.filePath("bad.png")).isEmpty());QVERIFY(Replayer::PNG2TXT(d.filePath("missing.png")).isEmpty());}
 void emptyReplay(){QTemporaryDir d;QFile f(d.filePath("empty.txt"));QVERIFY(f.open(QIODevice::WriteOnly));f.close();Replayer replay(nullptr,f.fileName());QCOMPARE(replay.getDuration(),0);}
};
QTEST_APPLESS_MAIN(ReplayTest)
#include "test_replay.moc"
