#include "widget.h"
#include "ui_widget.h"
#include <iostream>
#include <QPainter>
#include <QHostAddress>
#include <QAbstractSocket>
#include <QMessageBox>
#include <QDebug>
#include<QSqlDatabase>
#include<QSqlQuery>
#include "backstorg.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    // 设置 QLineEdit 的样式
       ui->ip->setStyleSheet(
                   "QLineEdit {"
                         "    font: 18pt 'Agency FB';"
                         "    color: white;"
                         "    background-color: rgba(0, 0, 0, 0);"
                         "    border: 2px solid white;"
                         "    outline: none;"
                         "}"
                         "QLineEdit:focus {"
                         "    border: 2px solid white;"
                         "    background-color: rgba(0, 0, 0, 0);"
                         "}"
       );

        ui->port->setStyleSheet(
                    "QLineEdit {"
                          "    font: 18pt 'Agency FB';"
                          "    color: white;"
                          "    background-color: rgba(0, 0, 0, 0);"
                          "    border: 2px solid white;"
                          "    outline: none;"
                          "}"
                          "QLineEdit:focus {"
                          "    border: 2px solid white;"
                          "    background-color: rgba(0, 0, 0, 0);"
                          "}"
                    );


    socket = new QTcpSocket(this);

//    connect(socket,&QTcpSocket::connected,[this]()
//    {
//        QMessageBox::information(this,"连接提示","连接成功");
//    });

    ui->ip->setFont(QFont("Lucida Console"));
    ui->ip->setText("192.168.0.30");
    ui->port->setFont(QFont("Lucida Console"));
    ui->port->setEchoMode(QLineEdit::Password); // 密码模式
    ui->port->setText("10086");

    label = new QLabel(this);
    background.load("D:\\static\\login.jpg");//加载bgp
    background = background.scaled(1920, 1080, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QPalette palette;
    palette.setBrush(QPalette::Background, background);
    this->setPalette(palette);


    this->show();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event); // 调用基类的绘制事件
    QPainter painter(this);


    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 20));


    painter.drawText(750, 100, "人脸识别考勤系统后台");
    painter.drawText(700,400,"账号");
    painter.drawText(700,500,"密码");
}


void Widget::on_pushButton_clicked()
{
    QString ip = ui->ip->text(); // 获取 IP 地址
    QString portStr = ui->port->text(); // 获取端口号
    bool ok; // 用于标记转换是否成功

    // 检查 IP 地址和端口号是否为空
    if (ip.isEmpty() || portStr.isEmpty()) {
        QMessageBox::warning(this, "错误", "请输入 IP 地址和端口号");
        return; // 如果为空，退出函数
    }

    // 将端口号转换为 ushort 类型
    ushort port = portStr.toUShort(&ok);
    if (!ok || port == 0) {
        QMessageBox::warning(this, "错误", "端口号无效");
        return; // 如果端口号无效，退出函数
    }

    // 检查 IP 地址是否有效
    QHostAddress address(ip);
    if (address.protocol() != QAbstractSocket::IPv4Protocol) {
        QMessageBox::warning(this, "错误", "IP 地址无效");
        return; // 如果 IP 地址无效，退出函数
    }


    QString str = address.toString();

    // 检查当前连接状态，如果正在连接或已连接，则不执行连接操作
    if (socket->state() == QAbstractSocket::ConnectedState ||
        socket->state() == QAbstractSocket::ConnectingState) {
        QMessageBox::information(this, "连接状态", "正在连接或已连接到服务器");
        return;
    }

    // 尝试连接服务器
    socket->connectToHost(address, port);

    // 检查连接是否成功
    if (socket->waitForConnected()) {
        //QMessageBox::information(this, "连接成功", "请等待");
        backstorg *back = new backstorg(socket,this);
        this->hide();
        back->show();
    } else {
        QMessageBox::warning(this, "连接失败", "无法连接到服务器");
        return;
    }

}

void Widget::on_pushButton_2_clicked()
{

    if(socket->state() == QAbstractSocket::ConnectedState)
    {
        socket->disconnectFromHost();
       // QMessageBox::information(this,"提示","已断开连接");
        this->close();
    }
    else {
        this->close();
        //QMessageBox::information(this,"提示","当前未连接到服务器");
    }
}
