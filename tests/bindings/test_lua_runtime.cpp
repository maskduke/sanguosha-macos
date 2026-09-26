#include <QtTest>
#include <memory>
#include "lua.hpp"
using State = std::unique_ptr<lua_State, decltype(&lua_close)>;
class LuaRuntimeTest : public QObject {
    Q_OBJECT
private slots:
    void historicalRuntimeAndConfiguration() {
        QCOMPARE(LUA_VERSION_NUM, 501);
        State state(luaL_newstate(), lua_close);
        QVERIFY(state);
        luaL_openlibs(state.get());
        const QByteArray path = QByteArray(QS_ASSET_ROOT) + "/lua/config.lua";
        const int result = luaL_dofile(state.get(), path.constData());
        QVERIFY2(result == 0, result ? lua_tostring(state.get(), -1) : "");
        lua_getglobal(state.get(), "config");
        QVERIFY(lua_istable(state.get(), -1));
        lua_getfield(state.get(), -1, "version");
        QCOMPARE(QByteArray(lua_tostring(state.get(), -1)), QByteArray("20120715"));
        lua_pop(state.get(), 1);
        lua_getfield(state.get(), -1, "version_name");
        QCOMPARE(QString::fromUtf8(lua_tostring(state.get(), -1)), QStringLiteral("涅槃版"));
        lua_pop(state.get(), 1);
        // Oracle: original app config.lua (SHA-256 in original-manifest.json), in declared order.
        const QList<QByteArray> packages = {"StandardCard", "StandardExCard", "Maneuvering", "SPCard", "Nostalgia", "New3v3Card", "YitianCard", "Joy", "Disaster", "JoyEquip", "Standard", "Wind", "Fire", "Thicket", "Mountain", "God", "SP", "YJCM", "YJCM2012", "Special3v3", "Ling", "BGM", "NostalGeneral", "Yitian", "Wisdom", "Test"};
        lua_getfield(state.get(), -1, "package_names");
        QCOMPARE(lua_objlen(state.get(), -1), size_t(packages.size()));
        for (int i = 0; i < packages.size(); ++i) {
            lua_rawgeti(state.get(), -1, i + 1);
            QCOMPARE(QByteArray(lua_tostring(state.get(), -1)), packages[i]);
            lua_pop(state.get(), 1);
        }
        lua_pop(state.get(), 1);
        lua_getfield(state.get(), -1, "scene_names");
        QCOMPARE(lua_objlen(state.get(), -1), size_t(39));
        const QList<QByteArray> named = {"Guandu", "Fancheng", "Couple", "Zombie", "Impasse", "Custom"};
        for (int i = 1; i <= 39; ++i) {
            const QByteArray expected = i <= 6 ? named[i - 1] : QByteArray("MiniScene_") + QByteArray::number(i - 6).rightJustified(2, '0');
            lua_rawgeti(state.get(), -1, i);
            QCOMPARE(QByteArray(lua_tostring(state.get(), -1)), expected);
            lua_pop(state.get(), 1);
        }
    }
    void errorAndGarbageCollection() {
        State state(luaL_newstate(), lua_close);
        QVERIFY(state);
        luaL_openlibs(state.get());
        QVERIFY(luaL_dostring(state.get(), "error('probe failure')") != 0);
        QVERIFY(QByteArray(lua_tostring(state.get(), -1)).contains("probe failure"));
        lua_pop(state.get(), 1);
        QCOMPARE(luaL_dostring(state.get(), "local t = {}; for i=1,10000 do t[i]={i} end; t=nil; collectgarbage('collect'); return 7"), 0);
        QCOMPARE(lua_tointeger(state.get(), -1), lua_Integer(7));
    }
};
QTEST_APPLESS_MAIN(LuaRuntimeTest)
#include "test_lua_runtime.moc"
