#include <iostream>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtGui/QImage>
#include <condition_variable>
#include <thread>
#include "face.h"
#include "widget.h"
#include "db.h"
#include "utils.h"


int main(int argc, char *argv[])
{
    //初始化qt界面
    QApplication a(argc, argv);
    Widget widget;
    // 设置窗口标志，隐藏窗口边框
    widget.setWindowFlags(Qt::FramelessWindowHint);
    widget.show();
    QLabel *videoLabel;
    videoLabel = widget.GetVideoLabel();
    //加载数据库的用户数据
    LoadUsers();
    FaceDetect(videoLabel,widget);
    return a.exec();
}
