#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>

#include "longlivedtcpsocket.h"
#include "headerframe_helper.h"

namespace Ui {
class MainWindow;
}

namespace Window
{
#define DEFAULT_USERNAME "device"
#define BLUE_STYLE "background-color: rgb(39, 151, 255); color: rgb(255, 255, 255)"
#define RED_STYLE "background-color: rgb(255, 94, 97);color: rgb(255, 255, 255); "
#define GRAY_STYLE "background-color: rgb(156, 156, 167); color: rgb(255, 255, 255)"
    
    class MainWindow : public QMainWindow
    {
        Q_OBJECT
    public:
        enum { RECONNECT_SECOND = 15 };
        
        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();
    signals:
        void sendPlainText(const QByteArray &bytes);
        void sendCoordinate(const QByteArray &bytes);
        void sendHelp(const QByteArray &bytes);
    public slots:
        void startGps();
        void on_startConnectionOne();
        void on_startConnectionTwo();
        void on_readyDisconnected();
        void on_error(const QString &error);
        void on_dataRecieved(const QByteArray &data, qint32 sourceFeatureCode);
    protected:
        bool event(QEvent *e);
    private:
        Ui::MainWindow *ui;
        QString _address;
        quint16 _port;
        QString _userName;
        bool _isForceDisconnect = false;
        
        Socket::LongLivedTcpSocket *_socket = nullptr;
        HeaderFrameHelper::MessageType _currentMessageType;
        
        void inivilizeMessageTypeComboBox();
        void inivilizeSocket();
    private slots:
        void insertMessage(const QString &messgae);
        void startConnectionHelper();
        void on_successfulConnected();
        void on_sendMessageButton_clicked();
        void on_disconnectButton_clicked();
    };
}

#endif // MAINWINDOW_H
