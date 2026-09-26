#include <QtTest>
#include <sstream>
#include "jsonutils.h"
#include "protocol.h"
using namespace QSanProtocol;
class LegacyProtocolTest : public QObject {
    Q_OBJECT
private slots:
    void streamReader() {
        std::istringstream input("[1,2]");
        Json::Reader reader;
        Json::Value value;
        QVERIFY(reader.parse(input, value));
        QCOMPARE(value.size(), 2u);
        QCOMPARE(value[1].asInt(), 2);
    }
    void packetGoldenSample() {
        const std::string wire = "[41,7,5,4,[\"slash:42\",true]]";
        QSanGeneralPacket packet;
        QVERIFY(packet.parse(wire));
        QCOMPARE(packet.m_globalSerial, 41u);
        QCOMPARE(packet.m_localSerial, 7u);
        QCOMPARE(packet.getPacketType(), S_CLIENT_REPLY);
        QCOMPARE(packet.getCommandType(), S_COMMAND_RESPONSE_CARD);
        QCOMPARE(packet.getMessageBody()[0].asString(), std::string("slash:42"));
        QCOMPARE(packet.toString(), wire);
    }
    void rejectsMalformedPacket() {
        for (const auto &wire : {"null", "{}", "[]", "[1,2,3]", "[1,2,\"bad\",4]", "[1,2,3,4,5,6]", "not-json"}) {
            QSanGeneralPacket packet;
            QVERIFY2(!packet.parse(wire), wire);
        }
    }
    void historicalPacketLimit() {
        // Historical source protocol.cpp: cap is 1000 bytes, independent of new framing.
        for (int size : {999, 1000, 1001}) {
            const std::string prefix = "[1,2,3,4,\"";
            const std::string suffix = "\"]";
            const auto wire = prefix + std::string(size - prefix.size() - suffix.size(), 'x') + suffix;
            QSanGeneralPacket packet;
            QCOMPARE(packet.parse(wire), size <= 1000);
        }
    }
    void integerArrayRejectsWithoutPartialAppend() {
        QList<int> result{9};
        Json::Value mixed(Json::arrayValue); mixed.append(1); mixed.append("bad");
        QVERIFY(!Utils::tryParse(mixed, result));
        QCOMPARE(result, QList<int>{9});
        QVERIFY(Utils::tryParse(Utils::toJsonArray(QList<int>{2,3}), result));
        const QList<int> expected{9,2,3};
        QCOMPARE(result, expected);
    }
    void stringArrayOrderAndRejection() {
        const QStringList expected{"slash", "", "jink"};
        QStringList result;
        QVERIFY(Utils::tryParse(Utils::toJsonArray(expected), result));
        QCOMPARE(result, expected);
        Json::Value bad(Json::arrayValue); bad.append("peach"); bad.append(false);
        QVERIFY(!Utils::tryParse(bad, result));
        QCOMPARE(result, expected);
    }
    void historicalLatin1Boundary() {
        // Qt4 default codec + unchanged jsonutils source: preserve the old byte boundary.
        const QString text = QString::fromLatin1("caf\xe9");
        QCOMPARE(Utils::toJsonString(text).asString(), std::string("caf\xe9"));
        QCOMPARE(Utils::toQString(Json::Value("caf\xe9")), text);
        QStringList decoded;
        Json::Value array(Json::arrayValue); array.append("caf\xe9");
        QVERIFY(Utils::tryParse(array, decoded));
        QCOMPARE(decoded, QStringList{text});
        QCOMPARE(Utils::toJsonString(QStringLiteral("刘备")).asString(), std::string("??"));
    }
    void smallArrayOverloads() {
        const auto two = Utils::toJsonArray(QStringLiteral("slash"), QStringLiteral("jink"));
        QCOMPARE(two.size(), 2u);
        QCOMPARE(two[1].asString(), std::string("jink"));
        const auto three = Utils::toJsonArray("slash", "jink", "peach");
        QCOMPARE(three.size(), 3u);
        QCOMPARE(three[2].asString(), std::string("peach"));
        const auto nested = Utils::toJsonArray("cards", Json::Value(7));
        QCOMPARE(nested[1].asInt(), 7);
    }
    void countdownGoldenSamples() {
        Countdown countdown;
        Json::Value val(Json::arrayValue); val.append("MG_COUNTDOWN"); val.append(2); val.append(9);
        QVERIFY(countdown.tryParse(val));
        QCOMPARE(countdown.m_type, Countdown::S_COUNTDOWN_USE_SPECIFIED);
        QCOMPARE(countdown.m_current, time_t(2));
        QCOMPARE(countdown.m_max, time_t(9));
        QVERIFY(!countdown.hasTimedOut());
        countdown.m_current = 9;
        QVERIFY(countdown.hasTimedOut());
        Countdown unlimited;
        QVERIFY(!unlimited.hasTimedOut());
        QCOMPARE(unlimited.toJsonValue()[1].asInt(), int(Countdown::S_COUNTDOWN_NO_LIMIT));
        Json::Value malformed(Json::arrayValue); malformed.append("other"); malformed.append(2);
        QVERIFY(!countdown.tryParse(malformed));
    }
};
QTEST_APPLESS_MAIN(LegacyProtocolTest)
#include "test_legacy_protocol.moc"
