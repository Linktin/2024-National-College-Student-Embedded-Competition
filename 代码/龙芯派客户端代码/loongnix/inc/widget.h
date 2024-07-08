#ifndef WIDGET_H
#define WIDGET_H

#include "user.h"
#include "voice.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtGui/QPainter>
#include <QtWidgets/QVBoxLayout>
#include <QtGui/QImage>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpServer>
#include <opencv2/opencv.hpp>
#include <vector>
#include <QtCore/QPropertyAnimation>

namespace Ui {
class Widget;
};

class item_info;
class User;
enum AtendenceType;


enum MessageType {
    SHOW_USERS =0,
    ADD_USER ,
    MODIFY_USER_NAEM_DEPT ,
    MODIFY_USER_IMGDATA,
    DELETE_USER,

    SET_TIME, //设置时间

    SHOW_USERS_RES,
    ADD_USER_RES,
    MODIFY_USER_RES,
    DELETE_USER_RES,

    SHOW_ATTEND,
    SHOW_ATTEND_RES,

    OVER,
    FALIURE
};
class Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();
    QLabel* GetVideoLabel(); //获取视   频画布
    void ShowUsers(QTcpSocket *socket);
    void AddUser(QTcpSocket *socket);
    void DeleteUser(QTcpSocket *socket);
    void ModifyUserNameAndDept(QTcpSocket *socket);
    void ModifyUserImgData(QTcpSocket *socket);
    void ReceiveAttendenceTime(QTcpSocket *socket);
    bool isValidTime(const QString& timeStr);//检查时间格式是否有效
    std::pair<QTime ,QTime> GetCurrentAttendenceTime();//获取当前考勤时间
    void ClearItem();//清除item栏的显示
    void SendAttendence(User &user);//向后台发送考勤记录

private slots:
    void updateScrollingText(QString &text);//滚动文本的显示
    void createSideBarItem(item_info item);//创建侧边栏
    void newConnection();//客户端连接
    void readMessage();//读取客户端发的消息类型
    void updateTime();
    void onAnimationFinished();//循环三次后

signals:
    void detectedFace(item_info item);
private:
    int width = 1024; // 窗口宽度
    int height = 600; // 窗口高度
    Ui::Widget *ui; // UI界面
    QLabel *label; // 主画布标签
    QLabel *img_label; // 图像标签
    QLabel *timeLabel;
    QLabel *video_label; // 视频标签
    QLabel *sideBar_label; // 侧边栏窗口
    QLabel *item_label[7];
    QLabel *profile_label[7];

    QImage background; // 背景图片
    QImage img_bgr; // 图像背景图片
    QImage side_bar; // 侧边栏图片
    QImage item_img[7];//侧边栏item图片、
    QImage user_img[7];
    QImage border_img[7];
    std::vector<item_info>items;

    QTcpServer *server;
    QTcpSocket *client;
    QTime startTime;//考勤开始时间
    QTime endTime;//考勤结束时间

    QTimer *timer;//一秒钟更新一次时间

    QLabel *scrollingLabel;
    QPropertyAnimation *scrollingAnimation;//滚动文本的属性   

    void setupSideBar(); // 初始化侧边栏
    void startServer();
    void startClient();
    void startUi();

};

class item_info
{
public:
    int id;
    QString img_path;
    QString name;
    QString dept;
    QString date;
    QString time;
    AtendenceType type;

    item_info() {} // 默认构造函数
    item_info(int id,QString &img_path,QString &name, QString &dept,AtendenceType type, QString &date,QString &time):
    id(id),
    img_path(img_path),
    name(name),
    dept(dept),
    type(type),
    date(date),
    time(time){}
};
Q_DECLARE_METATYPE(item_info)
#endif // WIDGET_H
