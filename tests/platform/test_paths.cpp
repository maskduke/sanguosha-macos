#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include "platform/paths/paths.h"
class PathsTest: public QObject { Q_OBJECT
private slots:
 void isolatedWritableFiles(){QTemporaryDir dir;QVERIFY(dir.isValid());qputenv("QS_DATA_DIR",dir.path().toUtf8());auto p=qs::userPath("etc/customScenes/test.txt");QVERIFY(p.startsWith(dir.path()+"/"));QFile f(p);QVERIFY(f.open(QIODevice::WriteOnly));f.write("scene");f.close();QCOMPARE(qs::readOverride("etc/customScenes/test.txt"),p);QVERIFY(qs::configFile().startsWith(dir.path()+"/"));}
 void traversalRejected(){QVERIFY_EXCEPTION_THROWN(qs::userPath("../outside"),std::invalid_argument);}
};
QTEST_APPLESS_MAIN(PathsTest)
#include "test_paths.moc"
