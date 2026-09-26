#include <QtTest>
#include <memory>
#include "lua.hpp"
extern "C" int luaopen_qt_values(lua_State *);
class QtValuesTest : public QObject {
    Q_OBJECT
private slots:
    void boundary_data() {
        QTest::addColumn<QByteArray>("script");
        QTest::newRow("chinese-roundtrip") << QByteArray("assert(qt_values.echoString('刘备：仁德') == '刘备：仁德')");
        QTest::newRow("cpp-to-lua-chinese") << QByteArray("assert(qt_values.makeChinese() == '刘备：仁德')");
        QTest::newRow("empty-string") << QByteArray("assert(qt_values.echoString('') == '')");
        QTest::newRow("list-order-and-empty") << QByteArray("local t=qt_values.echoList({'杀','','诸葛亮'}); assert(#t==3 and t[1]=='杀' and t[2]=='' and t[3]=='诸葛亮')");
        QTest::newRow("empty-list") << QByteArray("assert(#qt_values.echoList({})==0)");
        QTest::newRow("nil-rejected") << QByteArray("assert(not pcall(qt_values.echoString,nil))");
        QTest::newRow("bool-rejected") << QByteArray("assert(not pcall(qt_values.echoString,true))");
        QTest::newRow("invalid-list-element") << QByteArray("assert(not pcall(qt_values.echoList,{false}))");
        QTest::newRow("variant-empty") << QByteArray("local v=qt_values.QVariant(); assert(v:toString()=='' and v:toInt()==0 and v:toBool()==false)");
        QTest::newRow("variant-integer") << QByteArray("local v=qt_values.QVariant(42); assert(v:toInt()==42 and v:toBool()==true)");
        QTest::newRow("variant-negative") << QByteArray("assert(qt_values.QVariant(-17):toInt()==-17)");
        QTest::newRow("variant-true") << QByteArray("assert(qt_values.QVariant(true):toBool()==true)");
        QTest::newRow("variant-false") << QByteArray("assert(qt_values.QVariant(false):toBool()==false)");
        QTest::newRow("variant-chinese") << QByteArray("assert(qt_values.QVariant('诸葛亮'):toString()=='诸葛亮')");
        QTest::newRow("variant-gc") << QByteArray("for i=1,1000 do local v=qt_values.QVariant('仁德'); assert(v:toString()=='仁德'); v=nil; collectgarbage('collect') end");
        // Lua 5.1 lua_isstring accepts numbers; preserve the historical conversion.
        QTest::newRow("numeric-coercion") << QByteArray("assert(qt_values.echoString(17)=='17')");
        // Historical C-string conversion truncates at NUL. Do not silently change it.
        QTest::newRow("historical-nul-boundary") << QByteArray("assert(qt_values.echoString('a'..string.char(0)..'b')=='a')");
        QTest::newRow("gc-and-repeated-roundtrip") << QByteArray("for i=1,1000 do local t=qt_values.echoList({'技能',tostring(i)}); assert(t[2]==tostring(i)); collectgarbage('collect') end");
    }
    void boundary() {
        QFETCH(QByteArray, script);
        std::unique_ptr<lua_State, decltype(&lua_close)> state(luaL_newstate(), lua_close);
        QVERIFY(state);
        luaL_openlibs(state.get());
        luaopen_qt_values(state.get());
        lua_settop(state.get(), 0);
        const int result = luaL_dostring(state.get(), script.constData());
        QVERIFY2(result == 0, result ? lua_tostring(state.get(), -1) : "");
    }
};
QTEST_APPLESS_MAIN(QtValuesTest)
#include "test_qt_values.moc"
