#ifndef LONGLIVEDTCPSOCKET_H
#define LONGLIVEDTCPSOCKET_H

#include <QTcpSocket>
#include <QTimer>

#include "headerframe_helper.h"

namespace Socket
{
    class LongLivedTcpSocket : public QTcpSocket
    {
        Q_OBJECT
    public:
        static LongLivedTcpSocket *Instance();
        
        enum {  PULSE_INTERVAL = 10000, PULSE_ACK = 5000};
        static QString getSocketErrorType(QAbstractSocket::SocketError error);
        ~LongLivedTcpSocket() = default;
    signals:
        void dataReceived(const QByteArray &data, qint32 sourceFeatureCode);
        void error(const QString &error);
        void readyDisconnected();
    public slots:
        void on_startConnection(const QString &host, quint16 port, const QString &userName);
        void on_requestDisconnect();
        
        qint64 on_sendPlainText(const QByteArray &bytes);
        qint64 on_sendCoordinate(const QByteArray &bytes);
        qint64 on_sendHelp(const QByteArray &bytes);
        
        QString getUserName()const { return _userName; }
    protected:
        virtual qint64 on_sendData(HeaderFrameHelper::MessageType messageType, const QByteArray &bytes);
        virtual qint64 on_sendData(HeaderFrameHelper::MessageType messageType, const QString &message);
        virtual qint64 on_readyRead();
        qint64 on_sendACK(const QByteArray &bytes);
    private slots:
        void on_pulseTimerTimeout();
        void on_connected();
        void on_disconnected();
        
        void on_pulseTimeOut();
    private:
        QTimer *_pulseTimer, *_waitPulseACKTimer;
        bool pulseACK = false; 
        
        qint32 _featureCode;
        QString _userName;

        bool _isConnected = false;
        bool _waitingForWholeData = false;
        bool _isReading = false;
        qint64 _currentRead = 0; 
        qint32 _targetLength = 0;
        HeaderFrameHelper::TcpHeaderFrame _headerFrame;
        
        //隐藏构造函数，单例模式
        explicit LongLivedTcpSocket(QObject *parent = nullptr);
        LongLivedTcpSocket(const LongLivedTcpSocket &s) = delete;
        LongLivedTcpSocket operator=(const LongLivedTcpSocket &s) = delete;
    };
}

#endif // LONGLIVEDTCPSOCKET_H
