#ifndef BACKSTORG_H
#define BACKSTORG_H

#include <QWidget>
#include <QModelIndexList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTableView>
#include <QStandardItemModel>
#include <QHBoxLayout>
#include <QPushButton>
#include "widget.h"
#include "Pagination.h"

namespace Ui {
class backstorg;
}

class backstorg : public QWidget
{
    Q_OBJECT

public:
    explicit backstorg(QTcpSocket* socket,Widget *widget,QWidget *parent = nullptr);
    void UsersManagement(const QString &itemString);//用户管理
    void initUserManageMent();//初始化用户管理界面
    void setTile();//标题样式
    void setList();//左侧List样式
    void hideUserManageMentButton(bool flag);//隐藏用户管理界面按钮
    void hideTable(bool flag);//隐藏表格
    void setButton();
    void setUpTableView();
    bool showUsers();//显示用户
    bool deleteUser(QString &id);//删除用户
    bool modifyUser(int id, const QString& newName, const QString& newDept);// 修改用户信息，只包含姓名和院系
    bool modifyUser(int id, qint64 newImgSize,const QString &fileName, const QByteArray& newImgData);
    void addUser(const QString&name,const QString &dept,const QString &fileName,const qint64&img_size,const QByteArray& img_data);
    void attendenceManagement();//考勤管理界面
    void showAttendence();//展示考勤数据
    void hideAttendenceWidget(bool flag);//隐藏考勤界面组件
    bool isValidTime(const QString& timeStr);//验证设置的时间是否符合24小时制
    void startServer();//开启服务器
    ~backstorg();

signals:
    void itemClicked(const QString &itemString);
private slots:
    void onListItemClicked(const QModelIndex &index);//点击左侧list的槽函数
    void onAddUserClicked();//增加用户按钮
    void onDeleteUserClicked();//删除用户按钮
    void onModifyUserCilcked();//修改用户按钮
    void backLogin();//连接断开，返回登录界面
    void onUserRefreshButton();//用户刷新界面
    void onExitButton();//退出按钮
    void updateTableView(int currentPage, int totalPages);//更新表格
    void updateTimeLabel();//显示实时时间
    void pushAttendenceTime();//提交考勤时间
    void newConnection();//龙芯派的连接
    void readMessage();//创建一个对话

private:
    Ui::backstorg *ui;
    QTcpServer *server;
    QTcpSocket *socket;
    Widget* widget;
    QStandardItemModel *model;
    QLabel *label[100];
    Pagination *pagination;
    QLineEdit *pageLineEdit;
    QLabel *totalPagesDisplay;
    QHBoxLayout *pageLayout;
    QPushButton *prevButton;
    QPushButton *nextButton;
    QLabel *totalPagesLabel;
    QTimer *timer;//一秒钟更新一次时间
};

#endif // BACKSTORG_H
