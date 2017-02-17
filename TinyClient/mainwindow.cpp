#include <QAndroidJniObject>
#include <QThread>
#include <QTextCodec>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "CustomEvent.h"

namespace Window
{
    /// 创建一个新的主界面
    MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow)
    {
        ui->setupUi(this);
        connect(ui->_button_refetchGPS, SIGNAL(clicked()), this, SLOT(startGps()));
        connect(ui->_button_addressOneConnect, SIGNAL(clicked()), this, SLOT(on_startConnectionOne()));
        connect(ui->_button_addressTwoConnect, SIGNAL(clicked()), this, SLOT(on_startConnectionTwo()));
        connect(ui->_button_sendMessage, SIGNAL(clicked()), this, SLOT(on_sendMessageButton_clicked()));
        connect(ui->_button_clearBrower, SIGNAL(clicked()), ui->textBrowser,SLOT(clear()));
        
        inivilizeMessageTypeComboBox();
        inivilizeSocket();
    }
    
    /// 初始化单一实例的socket
    void MainWindow::inivilizeSocket()
    {
        QThread *workerThread = new QThread;
        
        _socket = Socket::LongLivedTcpSocket::Instance();
        connect(_socket, &QObject::destroyed, workerThread, &QThread::quit);
        connect(workerThread, &QThread::finished,workerThread,&QThread::deleteLater);
        
        connect(_socket, &Socket::LongLivedTcpSocket::connected, this, &MainWindow::on_successfulConnected);
        connect(_socket, &Socket::LongLivedTcpSocket::dataReceived, this, &MainWindow::on_dataRecieved);
        connect(_socket, SIGNAL(readyDisconnected()), this, SLOT(on_readyDisconnected()));
        connect(_socket, SIGNAL(error(const QString &)), this, SLOT(on_error(const QString &)));
        
        connect(this,SIGNAL(sendPlainText(const QByteArray &)), _socket, SLOT(on_sendPlainText(const QByteArray &)));
        connect(this,SIGNAL(sendCoordinate(const QByteArray &)), _socket, SLOT(on_sendCoordinate(const QByteArray &)));
        connect(this,SIGNAL(sendHelp(const QByteArray &)), _socket, SLOT(on_sendHelp(const QByteArray &)));
        
        connect(ui->_button_disconnect, SIGNAL(clicked()), this, SLOT(on_disconnectButton_clicked()));
        
        _socket->moveToThread(workerThread);
        workerThread->start();
    }
    
    /// 初始化发送类型选择框
    void MainWindow::inivilizeMessageTypeComboBox()
    {
        ui->_comboBox_messageType->addItem("PlainText",QVariant(static_cast<qint32>(HeaderFrameHelper::MessageType::PlainMessage)));
        ui->_comboBox_messageType->addItem("Coordinate",QVariant(static_cast<qint32>(HeaderFrameHelper::MessageType::Coordinate)));
        ui->_comboBox_messageType->addItem("Help",QVariant(static_cast<qint32>(HeaderFrameHelper::MessageType::Help)));
        
        ui->_comboBox_messageType->setCurrentIndex(0);
        _currentMessageType = HeaderFrameHelper::MessageType::PlainMessage;
        
        connect(ui->_comboBox_messageType, SIGNAL(currentIndexChanged(int)), this, SLOT(on_messageTypeComboBoxSelectionChanged(int)));
    }
    
    void MainWindow::on_messageTypeComboBoxSelectionChanged(int)
    {
        if(ui->_comboBox_messageType->currentIndex() == 0)
            _currentMessageType = HeaderFrameHelper::MessageType::PlainMessage;
        else if(ui->_comboBox_messageType->currentIndex() == 1)
            _currentMessageType = HeaderFrameHelper::MessageType::Coordinate;
        else if (ui->_comboBox_messageType->currentIndex() == 2)
            _currentMessageType = HeaderFrameHelper::MessageType::Help;
    }
    
    /// 发送消息
    void MainWindow::on_sendMessageButton_clicked()
    {
        QTextCodec *codec = QTextCodec::codecForName("GB18030");
        
        QString text = ui->_textEdit_messageEditor->toPlainText();
        QByteArray bytes = codec->fromUnicode(text);
        
        insertMessage(_userName + ": " + text);
        ui->_textEdit_messageEditor->clear();
        
        if (_currentMessageType == HeaderFrameHelper::MessageType::PlainMessage)
            emit sendPlainText(bytes);
        else if(_currentMessageType == HeaderFrameHelper::MessageType::Coordinate)
            emit sendCoordinate(bytes);
        else if(_currentMessageType == HeaderFrameHelper::MessageType::Help)
        {
            QByteArray coordinateMessage = codec->fromUnicode(_lastPostion + " ");
            coordinateMessage += bytes;
            emit sendHelp(coordinateMessage);
        }
            
    }
    
    /// 打开GPS定位功能
    void MainWindow::startGps()
    {
        // TODO:其实可以添加关闭GPS功能，但是等我先学了Andriod和Java再说
        ui->_button_refetchGPS->setEnabled(false);
        ui->_button_refetchGPS->setStyleSheet("background-color: rgb(156, 156, 167); color: rgb(255, 255, 255); ");
        //QAndroidJniObject javaAction = QAndroidJniObject::fromString(url);
        QAndroidJniObject::callStaticMethod<void>(
            "com/mtn/mes/GpsService",
            "calledByCpp",
            "()V");
    }
    
    /// 往textBrower中插入信息
    void MainWindow::insertMessage(const QString &messgae)
    {
        ui->textBrowser->moveCursor(QTextCursor::MoveOperation::End);
        ui->textBrowser->insertPlainText(messgae + "\n");
        ui->textBrowser->moveCursor(QTextCursor::MoveOperation::End);
    }
    
    /// 与链接地址1尝试连接
    void MainWindow::on_startConnectionOne()
    {
        _address = "157ww07468.imwork.net";
        _port = 18545;
        
        ui->_lineEdit_address ->setText(_address);
        ui->_lineEdit_port->setText(QString::number(_port));
        
        startConnectionHelper();
    }
    
    /// 与链接地址2尝试连接
    void MainWindow::on_startConnectionTwo()
    {
        _address = "philipyexushen.imwork.net";
        _port = 18271;
        
        ui->_lineEdit_address->setText(_address);
        ui->_lineEdit_port->setText(QString::number(_port));
        
        startConnectionHelper();
    }
    
    /// 尝试连接的私有方法
    void MainWindow::startConnectionHelper()
    {
        if (ui->_lineEdit_userName->text().isEmpty())
        {   
            ui->_lineEdit_userName->setText(tr(DEFAULT_USERNAME));
            _userName = tr(DEFAULT_USERNAME);
        }
        else
            _userName = ui->_lineEdit_userName->text();
       
        QMetaObject::invokeMethod(_socket,"on_startConnection",Qt::QueuedConnection, 
                                  Q_ARG(const QString &, _address), 
                                  Q_ARG(quint16,_port),
                                  Q_ARG(const QString &, _userName));
    }
    
    /// 成功连接响应方法
    void MainWindow::on_successfulConnected()
    {
        insertMessage(tr("###连接目标服务器成功！"));
        
        ui->_button_sendMessage->setEnabled(true);
        ui->_button_sendMessage->setStyleSheet(BLUE_STYLE);
        
        ui->_button_disconnect->setEnabled(true);
        ui->_button_disconnect->setStyleSheet(BLUE_STYLE);
        
        ui->_button_addressOneConnect->setEnabled(false);
        ui->_button_addressOneConnect->setStyleSheet(GRAY_STYLE);
                
        ui->_button_addressTwoConnect->setEnabled(false);
        ui->_button_addressTwoConnect->setStyleSheet(GRAY_STYLE);
        
        ui->_lineEdit_userName->setReadOnly(true);
    }

    /// 关闭连接
    void MainWindow::on_disconnectButton_clicked()
    {
        _isForceDisconnect = true;
        QMetaObject::invokeMethod(_socket, "on_requestDisconnect",Qt::QueuedConnection);
    }
    
    /// 响应套接字关闭
    void MainWindow::on_readyDisconnected()
    {
        insertMessage(tr("###与目标服务器断开连接"));
        ui->_button_sendMessage->setEnabled(false);
        ui->_button_sendMessage->setStyleSheet(GRAY_STYLE);
        
        ui->_button_disconnect->setEnabled(false);
        ui->_button_disconnect->setStyleSheet(GRAY_STYLE);
        ui->_lineEdit_userName->setReadOnly(false);
        
        if (!_isForceDisconnect && ui->_checkBox_IsAutoReconnect->isChecked())
        {
            insertMessage(tr("客户端将在%1秒后重新连接服务器").arg(RECONNECT_SECOND));
            QTimer::singleShot(RECONNECT_SECOND*1000, this, SLOT(startConnectionHelper()));
        }
        
        if (_isForceDisconnect)
        {
            ui->_button_addressOneConnect->setEnabled(true);
            ui->_button_addressOneConnect->setStyleSheet(BLUE_STYLE);
            
            ui->_button_addressTwoConnect->setEnabled(true);
            ui->_button_addressTwoConnect->setStyleSheet(BLUE_STYLE);
            
            _isForceDisconnect = false;
        }
    }
    
    /// 响应套接字出错，注意如果套接字出错，则其disconnected信号一定会发出
    void MainWindow::on_error(const QString &error)
    {
        insertMessage("###" + error);
    }
    
    /// 接受消息
    void MainWindow::on_dataRecieved(const QByteArray &data, qint32 sourceFeatureCode)
    {
        QTextCodec *codec = QTextCodec::codecForName("GB18030");
        QTextDecoder *decoder = codec->makeDecoder();
        
        QString message = decoder->toUnicode(data);
        insertMessage(QString::number(sourceFeatureCode) + ": " + message);
        
        delete decoder;
    }
    
    /// 重载主窗口的event函数，截获GPS获取事件
    bool MainWindow::event(QEvent *e)
    {
        if(e->type() == Events::CustomEvent::eventType())
        {
            e->accept();
            Events::CustomEvent *sce = (Events::CustomEvent*)e;
            if(sce->m_arg2 == "failed")    
                insertMessage("获取GPS定位坐标失败");
            else
            {
                _lastPostion = sce->m_arg2;
                insertMessage("P " + sce->m_arg2);
                QTextCodec *codec = QTextCodec::codecForName("GB18030");
                QByteArray bytes = codec->fromUnicode(sce->m_arg2);
                emit sendCoordinate(bytes);
            }
            return true;
        }
        return QWidget::event(e);
    }
    
    /// 析构主窗口，要记得socket的deleteLater才能关闭线程
    MainWindow::~MainWindow()
    {
        QMetaObject::invokeMethod(_socket,"deleteLater",Qt::QueuedConnection);
        delete ui;
    }
}


