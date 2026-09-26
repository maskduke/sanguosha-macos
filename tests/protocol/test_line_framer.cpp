#include <QtTest>
#include "platform/network/line_framer.h"
using qs::LineFramer;

class FramingTest : public QObject {
    Q_OBJECT
private slots:
    void everySplitPreservesBytes() {
        const QByteArray payload = QStringLiteral("昵称：刘备/技能\\n响应").toUtf8();
        const auto wire = payload + '\n';
        for (qsizetype split = 0; split <= wire.size(); ++split) {
            LineFramer parser;
            auto frames = parser.feed(wire.left(split));
            frames += parser.feed(wire.mid(split));
            QCOMPARE(frames, QList<QByteArray>{payload});
            QVERIFY(parser.finish());
        }
    }
    void concatenationAndTail() {
        LineFramer parser;
        const QList<QByteArray> expected{"first", "", "third\r"};
        QCOMPARE(parser.feed("first\n\nthird\r\ntail"), expected);
        QCOMPARE(parser.bufferedBytes(), 4);
        QCOMPARE(parser.feed(" end\n"), QList<QByteArray>{"tail end"});
        QCOMPARE(parser.bufferedBytes(), 0);
    }
    void oneByteAtATime() {
        LineFramer parser;
        QList<QByteArray> frames;
        for (const char ch : QByteArray("one\ntwo\nthree\n"))
            frames += parser.feed(QByteArray(1, ch));
        const QList<QByteArray> expected{"one", "two", "three"};
        QCOMPARE(frames, expected);
    }
    void limits_data() {
        QTest::addColumn<int>("size");
        QTest::newRow("L-1") << int(LineFramer::MaxPayload - 1);
        QTest::newRow("L") << int(LineFramer::MaxPayload);
    }
    void limits() {
        QFETCH(int, size);
        LineFramer parser;
        const QByteArray payload(size, 'x');
        QVERIFY(parser.feed(payload).isEmpty());
        QCOMPARE(parser.bufferedBytes(), size);
        QCOMPARE(parser.feed("\n"), QList<QByteArray>{payload});
        QCOMPARE(parser.error(), LineFramer::Error::None);
    }
    void oversized_data() {
        QTest::addColumn<bool>("delimiter");
        QTest::newRow("no-LF") << false;
        QTest::newRow("with-LF") << true;
    }
    void oversized() {
        QFETCH(bool, delimiter);
        LineFramer parser;
        QByteArray input(LineFramer::MaxPayload + 1, 'x');
        if (delimiter) input += '\n';
        QVERIFY(parser.feed(input).isEmpty());
        QCOMPARE(parser.error(), LineFramer::Error::TooLarge);
        QCOMPARE(parser.bufferedBytes(), 0);
        QVERIFY(parser.feed("ignored\n").isEmpty());
        QVERIFY(!parser.finish());
        parser.reset();
        QCOMPARE(parser.feed("new connection\n"), QList<QByteArray>{"new connection"});
    }
    void oversizedAcrossChunks() {
        LineFramer parser;
        parser.feed(QByteArray(LineFramer::MaxPayload, 'x'));
        QVERIFY(parser.feed("x\n").isEmpty());
        QCOMPARE(parser.error(), LineFramer::Error::TooLarge);
        QCOMPARE(parser.bufferedBytes(), 0);
    }
    void earlierValidFramesSurviveBadTail() {
        LineFramer parser;
        QCOMPARE(parser.feed("valid\n" + QByteArray(LineFramer::MaxPayload + 1, 'x')),
                 QList<QByteArray>{"valid"});
        QCOMPARE(parser.error(), LineFramer::Error::TooLarge);
    }
    void disconnectDoesNotInventFrame() {
        LineFramer parser;
        QVERIFY(parser.finish());
        parser.feed("incomplete");
        QVERIFY(!parser.finish());
        QCOMPARE(parser.error(), LineFramer::Error::Truncated);
        QCOMPARE(parser.bufferedBytes(), 0);
        QVERIFY(parser.feed("\n").isEmpty());
    }
};
QTEST_APPLESS_MAIN(FramingTest)
#include "test_line_framer.moc"
