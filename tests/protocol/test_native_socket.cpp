#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include "nativesocket.h"
#include "settings.h"
class NativeSocketTest: public QObject {
 Q_OBJECT
private slots:
 void loopbackListener(){Config.ServerPort=0;NativeServerSocket s;QVERIFY(s.listen(true));auto tcp=s.findChild<QTcpServer *>();QVERIFY(tcp);QCOMPARE(tcp->serverAddress(),QHostAddress(QHostAddress::LocalHost));}
 void fragmentedAndQueuedUtf8(){
  QTcpServer listener;QVERIFY(listener.listen(QHostAddress::LocalHost,0));QTcpSocket peer;peer.connectToHost(QHostAddress::LocalHost,listener.serverPort());QVERIFY(peer.waitForConnected());QTRY_VERIFY(listener.hasPendingConnections());NativeClientSocket socket(listener.nextPendingConnection());
  QSignalSpy received(&socket,&ClientSocket::message_got);QList<QByteArray> queued;
  connect(&socket,&ClientSocket::message_got,this,[&](QByteArray b){queued<<b;},Qt::QueuedConnection);
  QTRY_VERIFY(peer.canReadLine());QByteArray greeting=peer.readLine();QVERIFY(greeting.startsWith("qs-modern/1 "));
  peer.write(greeting.left(10));peer.flush();QTest::qWait(10);QCOMPARE(received.size(),0);peer.write(greeting.mid(10));
  QByteArray payload=QStringLiteral("中文 空格").toUtf8()+QByteArray(8192,'x');peer.write(payload.left(2));peer.flush();QTest::qWait(10);QCOMPARE(received.size(),0);peer.write(payload.mid(2)+"\nsecond\n");
  QTRY_COMPARE(received.size(),2);QTRY_COMPARE(queued.size(),2);QCOMPARE(queued[0],payload+"\n");QCOMPARE(queued[1],QByteArray("second\n"));
  socket.send(QStringLiteral("返回 中文"));QTRY_VERIFY(peer.canReadLine());QCOMPARE(peer.readLine(),QStringLiteral("返回 中文\n").toUtf8());
 }
 void rejectsDifferentRules(){QTcpServer l;QVERIFY(l.listen(QHostAddress::LocalHost,0));QTcpSocket p;p.connectToHost(QHostAddress::LocalHost,l.serverPort());QVERIFY(p.waitForConnected());QTRY_VERIFY(l.hasPendingConnections());NativeClientSocket s(l.nextPendingConnection());QSignalSpy errors(&s,&ClientSocket::error_message);QSignalSpy messages(&s,&ClientSocket::message_got);p.write("qs-modern/1 wrong-rules\nsignup ignored\n");QTRY_COMPARE(errors.size(),1);QCOMPARE(messages.size(),0);QTRY_VERIFY(!s.isConnected());}
 void rejectsOversize(){QTcpServer l;QVERIFY(l.listen(QHostAddress::LocalHost,0));QTcpSocket p;p.connectToHost(QHostAddress::LocalHost,l.serverPort());QVERIFY(p.waitForConnected());QTRY_VERIFY(l.hasPendingConnections());NativeClientSocket s(l.nextPendingConnection());QSignalSpy errors(&s,&ClientSocket::error_message);QTRY_VERIFY(p.canReadLine());p.write(p.readLine());p.write(QByteArray(qs::LineFramer::MaxPayload+1,'x'));QTRY_VERIFY(!s.isConnected());QCOMPARE(errors.size(),1);}
};
QTEST_GUILESS_MAIN(NativeSocketTest)
#include "test_native_socket.moc"
