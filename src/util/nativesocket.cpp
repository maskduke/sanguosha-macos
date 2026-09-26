#include "nativesocket.h"
#include "settings.h"

#include <QTcpSocket>
#include <QTimer>
#include "qs_build_identity.h"
#include <QRegExp>
#include <QStringList>
#include <QUdpSocket>

NativeServerSocket::NativeServerSocket()
{
    server = new QTcpServer(this);
    daemon = NULL;

    connect(server, SIGNAL(newConnection()), this, SLOT(processNewConnection()));
}

bool NativeServerSocket::listen(bool localOnly){
    return server->listen(localOnly ? QHostAddress::LocalHost : QHostAddress::Any, Config.ServerPort);
}

void NativeServerSocket::daemonize(){
    daemon = new QUdpSocket(this);
    daemon->bind(Config.ServerPort, QUdpSocket::ShareAddress);

    connect(daemon, SIGNAL(readyRead()), this, SLOT(processNewDatagram()));
}

void NativeServerSocket::processNewDatagram(){
    while(daemon->hasPendingDatagrams()){
        QHostAddress from;
        char ask_str[256];

        daemon->readDatagram(ask_str, sizeof(ask_str), &from);

        QByteArray data = Config.ServerName.toUtf8();
        daemon->writeDatagram(data, from, Config.DetectorPort);
    }
}

void NativeServerSocket::processNewConnection(){
    QTcpSocket *socket = server->nextPendingConnection();
    NativeClientSocket *connection = new NativeClientSocket(socket);
    emit new_connection(connection);
}

// ---------------------------------

NativeClientSocket::NativeClientSocket()    
    :socket(new QTcpSocket(this))
{
    init();
}

NativeClientSocket::NativeClientSocket(QTcpSocket *socket)
    :socket(socket)
{
    socket->setParent(this);
    init();
}

void NativeClientSocket::init(){
    connect(socket, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(getMessage()));
    connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
            this, SLOT(raiseError(QAbstractSocket::SocketError)));
    connect(socket, &QTcpSocket::connected, this, [this]{sendHello();emit connected();});
    if(socket->state()==QAbstractSocket::ConnectedState) sendHello();
    QTimer::singleShot(5000,this,[this]{if(!compatible && socket->state()!=QAbstractSocket::UnconnectedState){emit error_message(tr("新版连接校验超时，请使用相同版本。"));socket->abort();}});
}

void NativeClientSocket::connectToHost(){
    QString address = "127.0.0.1";
    ushort port = 9527u;

    if(Config.HostAddress.contains(QChar(':'))){
        QStringList texts = Config.HostAddress.split(QChar(':'));
        address = texts.value(0);
        port = texts.value(1).toUShort();
    }else{
        address = Config.HostAddress;
        if(address == "127.0.0.1")
            port = Config.value("ServerPort","9527").toString().toUShort();
    }

    socket->connectToHost(address, port);
}

static QByteArray hello(){return QByteArray("qs-modern/1 323f112658b5ea0c081b02e240afb92af7a379c1 ")+QS_RULES_FINGERPRINT;}
void NativeClientSocket::sendHello(){socket->write(hello()+"\n");}
void NativeClientSocket::getMessage(){
    while(socket->bytesAvailable()){
        const auto frames=framer.feed(socket->read(64*1024));
        if(framer.error()!=qs::LineFramer::Error::None){emit error_message(tr("网络消息超过 1 MiB 上限。"));socket->abort();return;}
        for(const QByteArray &frame:frames){
            if(!compatible){
                if(frame!=hello()){emit error_message(tr("客户端协议或规则不一致，请使用同一个新版安装包。"));socket->abort();return;}
                compatible=true;
            }else{
                if(frame.contains('\0')){socket->abort();return;}
                emit message_got(frame+"\n");
            }
        }
    }
}

void NativeClientSocket::disconnectFromHost(){    
    socket->disconnectFromHost();
}

void NativeClientSocket::send(const QString &message){
    const QByteArray bytes=message.toUtf8();
    if(bytes.size()>qs::LineFramer::MaxPayload || bytes.contains('\n') || bytes.contains('\0')){
        emit error_message(tr("无法发送无效或过长的网络消息。"));socket->abort();return;
    }
    socket->write(bytes+"\n");
}

bool NativeClientSocket::isConnected() const{
    return socket->state() == QTcpSocket::ConnectedState;
}


QString NativeClientSocket::peerName() const{
    QString peer_name = socket->peerName();
    if(peer_name.isEmpty())
        peer_name = QString("%1:%2").arg(socket->peerAddress().toString()).arg(socket->peerPort());

    return peer_name;
}

QString NativeClientSocket::peerAddress() const{
    return socket->peerAddress().toString();
}

void NativeClientSocket::raiseError(QAbstractSocket::SocketError socket_error){
    // translate error message
    QString reason;
    switch(socket_error){
    case QAbstractSocket::ConnectionRefusedError:
        reason = tr("Connection was refused or timeout"); break;
    case QAbstractSocket::RemoteHostClosedError:
        reason = tr("Remote host close this connection"); break;
    case QAbstractSocket::HostNotFoundError:
        reason = tr("Host not found"); break;
    case QAbstractSocket::SocketAccessError:
        reason = tr("Socket access error"); break;
    case QAbstractSocket::NetworkError:
        return; // this error is ignore ...
    default: reason = tr("Unknow error"); break;
    }

    emit error_message(tr("Connection failed, error code = %1\n reason:\n %2").arg(socket_error).arg(reason));
}
