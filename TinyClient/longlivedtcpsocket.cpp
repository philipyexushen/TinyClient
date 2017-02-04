#include <QTextCodec>
#include "longlivedtcpsocket.h"

namespace Socket
{
    /// 获得socket的唯一实例
    LongLivedTcpSocket *LongLivedTcpSocket::Instance()
    {
        static LongLivedTcpSocket *socket = new LongLivedTcpSocket;
        return socket;
    }
    
    /// 创建一个新的socket
    LongLivedTcpSocket::LongLivedTcpSocket(QObject *parent)
        :QTcpSocket (parent)
    {
        qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
        
        _pulseTimer = new QTimer(this);
        _waitPulseACKTimer = new QTimer(this);
        connect(this, &LongLivedTcpSocket::connected, this, &LongLivedTcpSocket::on_connected);
        connect(this, &LongLivedTcpSocket::readyRead, this, &LongLivedTcpSocket::on_readyRead);
        connect(this, &LongLivedTcpSocket::disconnected, this, &LongLivedTcpSocket::on_disconnected);
        
        connect(this, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
              [=](QAbstractSocket::SocketError){ emit error(errorString()); });
    }
    
    /// 如果是多线程异步连接目标主机，必须在线程启动后用QMetaObject::InvokeMethod引发此槽函数
    void LongLivedTcpSocket::on_startConnection(const QString &host, quint16 port, const QString &userName)
    {
        _userName = userName;
        connectToHost(host,port);
    }
    
    /// 当连接成功时，并且返回一个登陆信息
    /// 此方法开启启动心跳包计数器
    void LongLivedTcpSocket::on_connected()
    {
        _isConnected = true;
        
        _pulseTimer->setInterval(PULSE_INTERVAL);
        connect(_pulseTimer,&QTimer::timeout, this, &LongLivedTcpSocket::on_pulseTimerTimeout);
        _pulseTimer->start();
        
        on_sendData(HeaderFrameHelper::MessageType::DeviceLogIn, _userName);
    }
    
    /// 响应客户端主动关闭连接信息
    void LongLivedTcpSocket::on_requestDisconnect()
    {
        _pulseTimer->stop();
        on_sendData(HeaderFrameHelper::MessageType::DeviceLogOut, QString());
        close();
    }
    
    /// 套接字关闭连接时，引发此槽，关闭心跳包定时器和设定_isConnected，发出readyDisconnected信号
    void LongLivedTcpSocket::on_disconnected()
    {
        _isConnected = false;
        _pulseTimer->stop();
        _waitPulseACKTimer->stop();
        
        emit readyDisconnected();
    }
    
    /// 当接受信息时，引发此槽方法
    /// 返回值为-1时，说明接收失败，大于零表示数据正常接收
    qint64 LongLivedTcpSocket::on_readyRead()
    {
        qint32 nRead = 0;
        qint64 readReturn;
        QByteArray bytes;

        _currentRead = bytesAvailable();
        //qDebug () << "nAvailable: " << _currentRead;
        
        if (_currentRead < HeaderFrameHelper::sizeofHeaderFrame())
            return 0;
        
        if (!_waitingForWholeData)
        {
            bytes.resize(HeaderFrameHelper::sizeofHeaderFrame());
            readReturn = read(bytes.data(), HeaderFrameHelper::sizeofHeaderFrame());
            
            if (readReturn == -1)
                return -1;
            nRead += readReturn;
            
            HeaderFrameHelper::praseHeader(bytes, _headerFrame);
            _targetLength = _headerFrame.messageLength + HeaderFrameHelper::sizeofHeaderFrame();
            //qDebug () << "headerFrame length: " << _headerFrame.messageLength;
        }
        
        if (_currentRead >= _targetLength)
        {
            qint32 length = _targetLength - HeaderFrameHelper::sizeofHeaderFrame();
            bytes.resize(length);
            readReturn = read(bytes.data(), length);
            
            if (readReturn == -1)
                return -1;
            nRead += readReturn;
            
            _featureCode = _headerFrame.featureCode;
            if(_headerFrame.messageType == static_cast<qint32>(HeaderFrameHelper::MessageType::PulseFacility))
            {
                
            }
            else if(_headerFrame.messageType == static_cast<qint32>(HeaderFrameHelper::MessageType::ACKPulse))
            {
                pulseACK = true;
            }
            else if(_headerFrame.messageType == static_cast<qint32>(HeaderFrameHelper::MessageType::PlainMessage)
                    ||_headerFrame.messageType == static_cast<qint32>(HeaderFrameHelper::MessageType::Coordinate))
            {
                emit dataReceived(bytes,_headerFrame.sourceFeatureCode);
            }
            else if (_headerFrame.messageType == static_cast<qint32>(HeaderFrameHelper::MessageType::ServerTest))
            {
                emit dataReceived(bytes,_headerFrame.sourceFeatureCode);
                on_sendData(HeaderFrameHelper::MessageType::ACK, QString("ACK!"));
            }
            qDebug () << "_currentRead: " << _currentRead;
            
            _waitingForWholeData = false;
            _currentRead -= _targetLength;
        }
        //如果不等于headerFrame.messageLength，说明还没读完，继续读取
        else
            _waitingForWholeData = true;
        return nRead;
    }
    
    /// 发送消息槽
    /// 返回值为-1时，说明发送失败，大于零表示数据正常发送
    qint64 LongLivedTcpSocket::on_sendData(HeaderFrameHelper::MessageType messageType, const QByteArray &bytes)
    {
        qint64 ret = -1;
        
        HeaderFrameHelper::TcpHeaderFrame header;
        header.messageType = static_cast<qint32>(messageType);
        
        header.featureCode = _featureCode;
        header.sourceFeatureCode = _featureCode;
        header.messageLength = bytes.size();      
        
        QByteArray packingBytes = HeaderFrameHelper::bindHeaderAndDatagram(header,bytes);
                                                 
        ret = write(packingBytes,packingBytes.size());
        flush();
        return ret;
    }
    
    /// （重载）发送消息槽，把message当成union的字符串发送
    /// 返回值为-1时，说明发送失败，大于零表示数据正常发送
    qint64 LongLivedTcpSocket::on_sendData(HeaderFrameHelper::MessageType messageType, const QString &message)
    {
        QTextCodec *codec = QTextCodec::codecForName("GB18030");
        auto bytes(codec->fromUnicode(message));
        
        return on_sendData(messageType, bytes);
    }
    
    /// 响应普通信息发送
    qint64 LongLivedTcpSocket::on_sendPlainText(const QByteArray &bytes)
    {
        return on_sendData(HeaderFrameHelper::MessageType::PlainMessage, bytes);
    }

    /// 响应坐标信息发送
    qint64 LongLivedTcpSocket::on_sendCoordinate(const QByteArray &bytes)
    {
        return on_sendData(HeaderFrameHelper::MessageType::Coordinate, bytes);
    }

    /// 响应测试回应发送
    qint64 LongLivedTcpSocket::on_sendACK(const QByteArray &bytes)
    {
        return on_sendData(HeaderFrameHelper::MessageType::ACK, bytes);
    }

    /// 响应求助信息发送
    qint64 LongLivedTcpSocket::on_sendHelp(const QByteArray &bytes)
    {
        return on_sendData(HeaderFrameHelper::MessageType::Help, bytes);
    }
    
    /// 响应心跳包发送
    void LongLivedTcpSocket::on_pulseTimerTimeout()
    {
        _waitPulseACKTimer->setSingleShot(true);
        _waitPulseACKTimer->start(PULSE_ACK);
        connect(_waitPulseACKTimer, SIGNAL(timeout()), this, SLOT(on_pulseTimeOut()));
        pulseACK = false;
        
        on_sendData(HeaderFrameHelper::MessageType::PulseFacility,QString());
    }
    
    /// 响应服务器心跳包回应
    void LongLivedTcpSocket::on_pulseTimeOut()
    {
        _waitPulseACKTimer->stop();
        disconnect(_waitPulseACKTimer, SIGNAL(timeout()), this, SLOT(on_pulseTimeOut()));
        if (!pulseACK && _isConnected)
        {
            error("socket lost connection");
            close();
        }
    }
}
