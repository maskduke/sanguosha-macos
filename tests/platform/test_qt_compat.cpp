#include <QtTest>
#include "platform/qt_compat.h"
#include "core/util.h"
class CompatTest: public QObject {
 Q_OBJECT
private slots:
 void seedAndBounds() { qsrand(73); QList<int> a; for(int i=0;i<1000;i++){int n=qrand(); QVERIFY(n>=0); a<<n;} qsrand(73); for(int n:a) QCOMPARE(qrand(),n); }
 void shufflePreservesCards() { QList<int> a{0,1,2,3,4,5}; qsrand(17); qShuffle(a); auto b=a; std::sort(b.begin(),b.end()); QCOMPARE(b,(QList<int>{0,1,2,3,4,5})); QList<int> empty; qShuffle(empty); QVERIFY(empty.isEmpty()); }
 void setConversion() { QCOMPARE(qToSet(QStringList{"a","b","a"}), (QSet<QString>{"a","b"})); }
 void affineTransform() { QTransform t(1,0,0,1,7,9); QCOMPARE(t.map(QPointF(2,3)), QPointF(9,12)); }
};
QTEST_APPLESS_MAIN(CompatTest)
#include "test_qt_compat.moc"
