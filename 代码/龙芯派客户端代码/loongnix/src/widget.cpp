#include "widget.h"
#include "ui_widget.h"
#include "utils.h"
#include "face.h"
#include "user.h"
#include <iostream>
#include<QtNetwork/QTcpSocket>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtCore/QDataStream>
#include <QtSql/QSqlQuery>
#include <QtCore/QStringList>
#include <QtCore/QtDebug>
#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QRegExp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <QtCore/QTime>
#include <QtCore/QThread>
#include <QtCore/QFileInfo>
#include <thread>

#define FACE_PATH "/home/hy/code/face/"      //存储用户照片的资源路径
#define SERVER_IP "192.168.0.102"
#define SERVER_PORT 10086

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    qRegisterMetaType<item_info>("item_info");
    ui->setupUi(this); // 设置UI界面
   
    //初始化UI
    startUi();
    // 初始化侧边栏
    setupSideBar();
    //开启服务器
    startServer(); 

    connect(this,&Widget::detectedFace,this,&Widget::createSideBarItem);
    connect(timer,&QTimer::timeout,this,&Widget::updateTime);
    connect(scrollingAnimation, &QPropertyAnimation::finished, this, &Widget::onAnimationFinished);//连接动画完成信号到槽函数
    this->show(); // 显示窗口
}

Widget::~Widget()
{
    delete ui; // 删除UI界面指针
}

QLabel* Widget::GetVideoLabel()
{
    return this->video_label; // 返回视频标签指针
}

void Widget::startUi()
{
    this->setFixedSize(width, height); 
    //初始化考勤时间，默认无效
    startTime.setHMS(0,0,0);
    endTime.setHMS(0,0,0);

    label = new QLabel(this); // 创建主画布标签
    img_label = new QLabel(label); // 创建图像标签，并设置为主画布标签的子控件
    video_label = new QLabel(label); // 创建视频标签，并设置为主画布标签的子控件
    sideBar_label = new QLabel(label);//侧边栏标签
    timer = new QTimer;//定时时钟
    timer->setInterval(1000);
    timer->start();//开启时钟

    // 创建标签用于显示时间和日期
    timeLabel = new QLabel(this);
    timeLabel->setStyleSheet("color: white"); // 设置字体颜色为白色
    timeLabel->setFont(QFont("Arial", 18));
    timeLabel->setGeometry(600, 5, 250, 30); // 设置标签的位置和大小

    // 创建滚动文本标签
    scrollingLabel = new QLabel(this);
    scrollingLabel->setGeometry(50, 490, 400, 30);
    scrollingLabel->setStyleSheet("color: white");
    scrollingLabel->setFont(QFont("Arial", 16));
    scrollingLabel->setText("请提醒管理员设置考勤时间");
    scrollingLabel->show();

    // 创建滚动动画
    scrollingAnimation = new QPropertyAnimation(scrollingLabel, "pos", this);
    scrollingAnimation->setDuration(7000); // 设置滚动时间为10秒
    scrollingAnimation->setStartValue(QPoint(50, 490));
    scrollingAnimation->setEndValue(QPoint(480, 490));
    scrollingAnimation->setLoopCount(3); // 设置循环次数为无限循环

   // 开始滚动动画
   scrollingAnimation->start();

    img_label->setGeometry(20, 20, 680, 560); // 设置图像标签的位置和大小
    video_label->setGeometry(40, 20, 680, 560); // 设置视频标签的位置和大小
    sideBar_label->setGeometry(710,55,300,480);//侧边栏标签大小

    background.load("/home/hy/code/static/background.png"); // 加载背景图片
    img_bgr.load("/home/hy/code/static/img_bgp.png"); // 加载图像背景图片
    side_bar.load("/home/hy/code/static/sidebar.png");//侧边栏图像

    background = background.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation); // 调整背景图片大小
    img_bgr = img_bgr.scaled(680, 480, Qt::IgnoreAspectRatio, Qt::SmoothTransformation); // 调整图像背景图片大小
    side_bar = side_bar.scaled(300,480,Qt::IgnoreAspectRatio, Qt::SmoothTransformation);//调整侧边栏大小

    img_label->setPixmap(QPixmap::fromImage(img_bgr)); // 设置图像标签的图片
    label->setPixmap(QPixmap::fromImage(background)); // 设置主画布标签的背景图片
    sideBar_label->setPixmap(QPixmap::fromImage(side_bar));//侧边栏图片

    video_label->lower(); // 确保视频标签显示在图像标签下方
    img_label->raise(); // 确保图像标签显示在视频标签上方
    sideBar_label->raise();
    scrollingLabel->raise();

}   
void Widget::setupSideBar()
{
    for(int i = 0;i<7;i++)
    {
        item_label[i] = new QLabel(label);
        item_label[i]->setGeometry(740,80+60*i+5,240,60);//item的大小、位置
    }
 
}

void Widget::createSideBarItem(item_info item)
{
    if(startTime == endTime || item.type == NOTSTARTED )
    {
        QString text = "考勤暂未开始";
        updateScrollingText(text);
        return;
    }
    if(items.size() == 7)
    {
        items.erase(items.begin());
    }
    if(item.type == PRESENT)
    {
        QString text = item.name+"打卡成功，希望您天天开心";
        updateScrollingText(text);
    }
    else if(item.type == LATE)
    {
        QString text = item.name+"您这次迟到了喔，下次注意呢";
        updateScrollingText(text);
    }
    else if(item.type == ABSENT)
    {
        QString text = item.name+"您这次缺勤了喔，下次注意呢";
        updateScrollingText(text);
    }
    items.push_back(item);
    User user(item.id,item.name.toStdString(),item.dept.toStdString(),item.img_path.toStdString());
    user.absence_state = item.type;
    SendAttendence(user);
    int i = 0;
    for(auto it= items.rbegin();it != items.rend();++it)
    {
        item_img[i].load("/home/hy/code/static/item.png");
        item_img[i] = item_img[i].scaled(240,60,Qt::IgnoreAspectRatio, Qt::SmoothTransformation);


        QPainter painter(&item_img[i]);

        if(it->type == LATE || it->type == ABSENT)
        {
            qDebug()<<item.type;
        }

        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial",15));
        painter.drawText(55, 30, it->name); // 调整绘制文字的坐标

        //院系
        painter.setFont(QFont("Arial",12));
        painter.setPen(QColor(173, 216, 230));
        painter.drawText(55,50,it->dept);

        //时间
        painter.setPen(Qt::white);
        painter.drawText(155,25,it->time);

        // 日期
        painter.setPen(QColor(173, 216, 230));
        painter.drawText(155,50,it->date);

        //头像
        border_img[i].load("/home/hy/code/static/biankuang.png");
        border_img[i] = border_img[i].scaled(45, 45, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        QImage img;
        img.load(it->img_path);
        user_img[i] = img;
        
        user_img[i] = user_img[i].scaled(40, 40, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        painter.drawImage(10,10,user_img[i]);
        painter.drawImage(6,6,border_img[i]);

        //更新侧边栏信息
        item_label[i]->setPixmap(QPixmap::fromImage(item_img[i]));
        item_label[i]->update();

        i++;
    }
}

//滚动文本显示一些提示信息
void Widget::updateScrollingText(QString &text)
{
    scrollingLabel->setText(text);
    scrollingAnimation->start();
}

//滚动文本显示三次后，将内容清空
void Widget::onAnimationFinished()
{
    scrollingLabel->clear();
}

void Widget::startServer()
{
    server = new QTcpServer;
    client = new QTcpSocket;
    if(!server->listen(QHostAddress("192.168.0.30"),10086))
    {
        std::cerr<<"开启服务器失败"<<std::endl;
        return;
    }
    std::cout<<"服务器开启成功"<<std::endl;
    connect(server,&QTcpServer::newConnection,this,&Widget::newConnection);
}

void Widget::newConnection()
{
    while(server->hasPendingConnections())
    {
        QTcpSocket *socket = server->nextPendingConnection();
        if(socket)
        {
            std::cout<<"连接成功"<<std::endl;
            connect(socket,&QTcpSocket::disconnected,socket,&QTcpSocket::deleteLater);
            connect(socket,&QTcpSocket::readyRead,this,&Widget::readMessage);
        }
    }
    if(client->state() == QTcpSocket::UnconnectedState)
    {
        startClient();
    }
}

void Widget::readMessage()
{
    QTcpSocket *s = (QTcpSocket *)sender();
    QDataStream stream(s);
    int message;
    stream >>message;

    std::cout<<message<<std::endl;

    switch(message)
    {
        case SHOW_USERS:
        ShowUsers(s);
        break;

        case ADD_USER:
        AddUser(s);
        break;

        case DELETE_USER:
        DeleteUser(s);
        break;

        case MODIFY_USER_NAEM_DEPT:
        ModifyUserNameAndDept(s);
        break;

        case MODIFY_USER_IMGDATA:
        ModifyUserImgData(s);
        break;

        case SET_TIME:
        ReceiveAttendenceTime(s);
        break;

        case SHOW_ATTEND:
        ShowAttendence(s);
        break;
    }
    
}

//查询数据库并返回后台
void Widget::ShowUsers(QTcpSocket *socket)
{
    printf("i am execute\n");
    if(!socket)
    {
        std::cout<<"连接断开"<<std::endl;
        return ;
    }
    // 移除已存在的默认连接
    if (QSqlDatabase::contains("qt_sql_default_connection")) 
    {
        QSqlDatabase::removeDatabase("qt_sql_default_connection");
    }

    QSqlDatabase facedb = QSqlDatabase::addDatabase("QSQLITE");
    facedb.setDatabaseName("/home/hy/code/databases/face.db");

    if(!facedb.open())
    {
        std::cout<<"打开数据库失败："<<facedb.lastError().text().toStdString();
        return;
    }

    //执行查询操作
    QSqlQuery query;
    if(!query.exec("select * from users"))
    {
        std::cout<<"查询失败"<<query.lastError().text().toStdString();
        return;
    }
    std::cout<<"查寻中"<<std::endl;

    //将查询结果存储在QList中
    QList<QStringList> userList;

    std::vector<int>imgs_size;//存放每张图片的大小
    std::vector<QByteArray>images;//存放照片的容器

    cv::Mat resized_face;
    while(query.next())
    {
        QStringList user;
        user<<query.value("id").toString();
        user<<query.value("name").toString();
        user<<query.value("dept").toString();
        userList.append(user);

        std::string facePath = query.value("face_path").toString().toStdString();
        QFileInfo fileInfo(QString::fromStdString(facePath));      
        if (facePath.empty() || !fileInfo.exists() || fileInfo.size() == 0)
        {   
            std::cerr << "不存在图像: " << facePath << std::endl;
            images.push_back(QByteArray());
            imgs_size.push_back(0);
            continue;
        }
        else
        {
            cv::Mat img = cv::imread(facePath);
            if(!img.empty())
            {
                cv::resize(img,resized_face,cv::Size(30,40));
            }
            QImage qImg(resized_face.data, resized_face.cols, resized_face.rows, resized_face.step, QImage::Format_BGR888);
            QByteArray imgData;
            QBuffer buffer(&imgData);
            buffer.open(QIODevice::WriteOnly);
            qImg.save(&buffer, "JPEG"); 

            images.push_back(imgData);//将图片放入容器

            int img_size = imgData.size();
            imgs_size.push_back(img_size); //将照片大小放入容器
        }
       
    }
    std::cout<<"查询完....."<<std::endl;

    //构建响应消息
    QByteArray responseMessage;
    QDataStream stream(&responseMessage,QIODevice::WriteOnly);
    stream << SHOW_USERS_RES;//响应标志位
    stream << userList;//用户信息
    for(const auto&img_size : imgs_size)//每张照片的大小
    {
        stream << img_size;
    }

    for(const auto&img: images)//每张图片都一次一次发过去
    {
        stream.writeRawData(img.constData(),img.size());
    }

    stream << OVER;
    //发送响应消息给客户端
    socket->write(responseMessage);
    socket->flush();
    std::cout<<"将查询结果发送到后台中......"<<std::endl;

    facedb.close();
}

void Widget::AddUser(QTcpSocket *socket)
{
    if(!socket)
    {
        std::cout<<"连接断开,增加用户失败"<<std::endl;
    }
    QDataStream stream(socket);
    QString name;
    QString dept;
    QString imgName;
    qint64 imgSize;
    QByteArray imgData;

    stream>>name>>dept>>imgName>>imgSize;
    std::cout << "接收到的用户信息: " << name.toStdString() << ", " << dept.toStdString() << ", " << imgName.toStdString() << ", " << imgSize << std::endl;
    //读取照片数据
  //  imgData.resize(imgSize);
  while (imgData.size() < imgSize)
  {
    if (!socket->bytesAvailable()) 
    {
        socket->waitForReadyRead(3000);
    }
    QByteArray tempData = socket->read(qMin(socket->bytesAvailable(), static_cast<qint64>(imgSize - imgData.size())));
    imgData.append(tempData);
}
  //  stream.readRawData(imgData.data(),imgSize);
    std::cout<<imgData.size()<<std::endl;
    //构建保存文件的完整路径
    QString imgPath = "/home/hy/code/face/" + imgName;
    //将照片数据保存在指定的路径
    QFile file(imgPath);
    if(!file.open(QIODevice::WriteOnly))
    {
        std::cout<<"保存照片失败"<<std::endl;
        return;
    }
    file.write(imgData);
    file.close();
    std::cout<<"保存图片成功"<<std::endl;
    // 移除已存在的默认连接
    if (QSqlDatabase::contains("qt_sql_default_connection")) 
    {
        QSqlDatabase::removeDatabase("qt_sql_default_connection");
    }
    //打开数据库连接
    QSqlDatabase facedb = QSqlDatabase::addDatabase("QSQLITE");
    facedb.setDatabaseName("/home/hy/code/databases/face.db");

    if(!facedb.open())
    {
        std::cout<<"增加用户时，打开数据库失败"<<std::endl;
        facedb.close();
        return;
    }

    //插入用户信息
    QSqlQuery query;
    query.prepare("insert into users (name,dept,face_path)values(?,?,?)");
    query.addBindValue(name);
    query.addBindValue(dept);
    query.addBindValue(imgPath);

    if(!query.exec())
    {
        std::cout<<"用户信息插入失败"<<query.lastError().text().toStdString();
        facedb.close();
        return;
    }
    std::cout << "插入用户成功"<<std::endl;

    //构建响应信息返回
    QByteArray responseMessage;
    QDataStream responseStream(&responseMessage, QIODevice::WriteOnly);
    responseStream << ADD_USER_RES;

    socket->write(responseMessage);
    socket->flush();

    //将该用户加入到内存中
    query.prepare("SELECT id FROM users ORDER BY id DESC LIMIT 0,1");
    if(!query.exec())
    {
        std::cout<<"查找最后一个用户失败"<<std::endl;
        return;
    }
    int id;
    if(query.next())
    {
         id = query.value(0).toInt();
        qDebug()<<id;
    }
   
    User user(id,name.toStdString(),dept.toStdString(),imgPath.toStdString());
    if(AddFaceDB(user))
    {
        qDebug()<<"添加到内存成功";
    }
    facedb.close();
}   
//修改名字和部门
void Widget::ModifyUserNameAndDept(QTcpSocket *socket)
{
    //打开数据库
    QSqlDatabase facedb = OpenDataBase();
    //接受套接字的发来的信息
    QDataStream stream(socket);

    int id;//id
    QString name;//name
    QString dept;//dept

    stream >> id;
    stream >> name;
    stream >> dept;

    QSqlQuery query;
    query.prepare("update users set name = :name,dept = :dept where id = :id");
    query.bindValue(":name",name);
    query.bindValue(":dept",dept);
    query.bindValue(":id",id);

    QByteArray responseMessage;
    QDataStream responseStrem(&responseMessage,QIODevice::WriteOnly);
    if(!query.exec())
    {
        qDebug()<<"更新用户信息失败"<<query.lastError().text();
        //发送错误信息给客户端
        responseStrem << FALIURE;
        socket->write(responseMessage);
        socket->flush();
        return;
    }
    qDebug()<<"更新用户信息成功";
    responseStrem << MODIFY_USER_RES;
    socket->write(responseMessage);
    socket->flush();

    User user;
    user.SetID(id);
    user.SetName(name.toStdString());
    user.SetDept(dept.toStdString());
    ModifyFaceDB(user);
    //关闭数据库
    facedb.close();
}

//修改用户图像信息
void Widget::ModifyUserImgData(QTcpSocket *socket)
{
    QDataStream stream(socket);
    int id;
    qint64 newImgSize;
    QString newFileName;
    QByteArray newImgData;
    QString oldFacePath;
    QString newFacePath;
    //从流里面获取信息
    stream >> id >> newImgSize >> newFileName;
    //接受图像数据
    while(newImgData.size() < newImgSize)
    {
        if(!socket->bytesAvailable())
        {
            socket->waitForReadyRead(30000);
        }
        QByteArray tempData = socket->read(qMin(socket->bytesAvailable(), static_cast<qint64>(newImgSize - newImgData.size())));
        newImgData.append(tempData);
    }
    qDebug() << newImgData.size();
    //打开数据库
    QSqlDatabase facedb = OpenDataBase();
    //构建sql语句
    QSqlQuery query;
    //先拿到旧照片的路径信息
    query.prepare("select face_path from users where id = ?");
    query.addBindValue(id);
    if(!query.exec())
    {
        QByteArray responseMessage;
        QDataStream responseStream(&responseMessage,QIODevice::WriteOnly);
        responseStream << FALIURE;
        socket->write(responseMessage);
        socket->flush();
        qDebug()<<"获取旧照片路径信息失败"<<query.lastError().text();
        return;
    }
    if(query.next())
    {
        oldFacePath = query.value(0).toString();
    }
    //构建新照片路径
    newFacePath = FACE_PATH + newFileName;
    //更新用户新照片信息
    query.prepare("update users set face_path = ? where id = ?" );
    query.addBindValue(newFacePath);
    query.addBindValue(id);
    if(!query.exec())
    {
        qDebug()<<"更新用户照片信息失败"<<query.lastError().text();
        QByteArray responseMessage;
        QDataStream responseStream(&responseMessage,QIODevice::WriteOnly);
        responseStream << FALIURE;
        socket->write(responseMessage);
        socket->flush();
        return;
    }
    qDebug()<<"更新用户照片信息成功";
    
    //删除旧照片
    QFile oldImg(oldFacePath);
    if(!oldImg.remove())
    {
        qDebug()<<"移除旧照片失败";
    }
    //构建新图像路径
    newFacePath = FACE_PATH + newFileName;
    //保存新图像
    QFile newImg(newFacePath);
    if(!newImg.open(QIODevice::WriteOnly))
    {
        qDebug()<<"保存新照片失败";
    }
    newImg.write(newImgData);
    newImg.close();//关闭文件
    qDebug()<<"保存新照片成功";

    QByteArray responseMessage;
    QDataStream responseStream(&responseMessage,QIODevice::WriteOnly);
    responseStream << MODIFY_USER_RES;
    socket->write(responseMessage);
    socket->flush();
    //关闭数据库连接
    facedb.close();

    User user;
    user.SetID(id);
    user.SetFacePath(newFacePath.toStdString());
    ModifyFaceDB(user);
}
void Widget::DeleteUser(QTcpSocket *socket)
{
    if(!socket)
    {
        std::cout<<"连接断开"<<std::endl;
        return;
    }

    QDataStream stream(socket);
    int id;

    stream >> id;
    std::cout<<"请求删除的用户ID："<<id<<std::endl;

    //连接数据库
    if(QSqlDatabase::contains("qt_sql_default_connection"))
    {
        QSqlDatabase::removeDatabase("qt_sql_default_connection");
    }

    QSqlDatabase facedb = QSqlDatabase::addDatabase("QSQLITE");
    facedb.setDatabaseName("/home/hy/code/databases/face.db");

    if(!facedb.open())
    {
        std::cout<<"数据库打开失败"<<std::endl;
        facedb.close();
        return;
    }
    //获取保存的图片路径
    QSqlQuery query;
    query.prepare("select face_path from users where id =?");
    query.addBindValue(id);
    if(!query.exec() || !query.next())
    {
        std::cout << "获取照片路径失败" << std::endl;
        facedb.close();
        return;
    }
    //删除保存的图片
    QString face_path = query.value("face_path").toString();
    std::cout << "请求删除的照片路径:"<<face_path.toStdString() <<std::endl;
    QFile file(face_path);
    if(!file.remove())
    {
        std::cout << "删除照片文件失败" << std::endl;
    }
    std::cout<<"删除用户成功"<<std::endl;
    //删除数据库的记录信息
    query.prepare("delete  from users where id = ?");
    query.addBindValue(id);
    //执行sql语句
    if(!query.exec())
    {
        std::cout<<"用户删除失败"<<std::endl;
        facedb.close();
        return;
    }
    //构建响应信息
    QByteArray responseMessage;
    QDataStream responseStream(&responseMessage,QIODevice::WriteOnly);
    responseStream << DELETE_USER_RES;
    //发送回客户端
    socket->write(responseMessage);
   
    socket->flush();
    //关闭数据库
    facedb.close();

    User user;
    user.SetID(id);

    DeleteFaceDB(user);
}

//验证时间是否有效
bool Widget::isValidTime(const QString& timeStr)
{
    QRegExp timeRegex("^(?:[01]\\d|2[0-3]):[0-5]\\d$"); // 正则表达式，匹配24小时制的时间格式
    return timeRegex.exactMatch(timeStr);
}
//设置定时器时钟实时显示
void Widget::updateTime()
{
    // 更新时间和日期
    timeLabel->setText(GetCurrentDate() + "     " + GetCurrentTime());
    timeLabel->update();

}

//设置考勤时间
void Widget::ReceiveAttendenceTime(QTcpSocket *socket)
{
    QString start;
    QString end;
    QDataStream stream(socket);

    stream >> start;
    stream >> end;

    qDebug()<<start;
    qDebug()<<end;

    if(!isValidTime(start) || !isValidTime(end))
    {
        QByteArray responseMessage;
        QDataStream responseStream(&responseMessage,QIODevice::WriteOnly);
        responseStream << FALIURE;
        socket->write(responseMessage);
        socket->flush();
        return;
    }
  
    startTime = QTime::fromString(start);
    endTime = QTime::fromString(end);

    QString text = "当前考勤时间从" + start + "到" +end;
    updateScrollingText(text);
    ClearItem();//清除右侧item
    UpdateFaceDB();//更新内存数据
    UpdateUserAttendce();//更改用户初始化考勤信息

    QByteArray responseMessage;
    QDataStream responseStream(&responseMessage,QIODevice::WriteOnly);

    responseStream << OVER;
    socket->write(responseMessage);
    socket->flush();
}
//获取当前考勤结束时间
std::pair<QTime ,QTime> Widget::GetCurrentAttendenceTime()
{
    return std::make_pair(startTime,endTime);
}

// //判断用户考勤情况
// AtendenceType item_info::isAbsence(Widget *widget)
// {
//     //获取当前考勤时间
//     std::pair<QTime,QTime>res = widget->GetCurrentAttendenceTime();
//     QTime start = res.first;
//     QTime end = res.second;

//     //当前考勤时间
//     QTime current = QTime::fromString(time);
//     //未开始考勤
//     if(current < start)
//         return NOTSTARTED;
//     //正常出勤    
//     if(current >= start && current <=end)
//         return PRESENT;
//     //迟到
//     if(current.secsTo(end) <= 3 )
//         return LATE;

//     return ABSENT;            
// }
//每次更新考勤时间，清除右侧的用户信息栏
void Widget::ClearItem()
{
    for(int i = 0;i<7;i++)
    {
        item_label[i]->setPixmap(QPixmap());
        items.clear();
    }
}
//初始化客户端套接字
void Widget::startClient()
{
    QString ip = SERVER_IP;
    ushort port = SERVER_PORT;

    client->connectToHost(ip,port);
    if(client->waitForConnected())
    {
        qDebug()<<"客户端启动成功";
    }
    else
    {
        qDebug()<<"客户端启动失败";
    }

}
//向后台发送考勤记录信息
void Widget::SendAttendence(User &user)
{
    if(client->state() == QTcpSocket::UnconnectedState)
    {
        qDebug()<<"与后台连接断开";
        return;
    }
    QByteArray message;
    QDataStream stream(&message,QIODevice::WriteOnly);

    std::pair<int,int>res = GetFaceDBNum();
    int total = res.first;
    int attendence_num = res.second;

    stream << total;
    stream << attendence_num;
    stream << user.GetID();
    QString attendenceType = ConvertAttendenceType(user.absence_state);
    stream << attendenceType;

    client->write(message);
    client->flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    // if(client->waitForReadyRead(300))
    // {
    //     int over_flag;
    //     QDataStream responseStream(client);
    //     responseStream >> over_flag;
    //     if(over_flag == OVER)
    //     {
    //         qDebug()<<"发送考勤记录成功";
    //     }
    // }
    // else
    // {
    //     qDebug()<<"后台响应超时";
    // }

}