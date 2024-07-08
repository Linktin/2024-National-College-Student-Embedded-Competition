#include "backstorg.h"
#include "ui_backstorg.h"
#include <vector>
#include <QList>
#include <QListView>
#include <QStringListModel>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QTcpSocket>
#include <QSqlQuery>
#include <QList>
#include <QSqlError>
#include <QHostAddress>
#include <QStandardItemModel>
#include <QHBoxLayout>
#include <QDebug>
#include <QDialog>
#include <QInputDialog>
#include <QFile>
#include <QTimer>
#include <QFileDialog>
#include "imagecenterdelegate.h"
#include "Pagination.h"

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

#define server_ip "192.168.0.30"
#define server_port 10086

backstorg::backstorg(QTcpSocket *s,Widget* widget,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::backstorg),
    server(new QTcpServer),
    socket(s),//通信套接字
    widget(widget),
    model(new QStandardItemModel),
    pagination(new Pagination),
    pageLineEdit(new QLineEdit),
    totalPagesDisplay(new QLabel),
    pageLayout(new QHBoxLayout),
    prevButton(new QPushButton("上一页")),
    nextButton(new QPushButton("下一页")),
    totalPagesLabel(new QLabel("页码总数")),
    timer(new QTimer)
{
    ui->setupUi(this);
    //设置标题
    setTile();
    //设置左侧列表
    setList();
    //设置中央表格样式
    setUpTableView();
    //设置按钮样式
    setButton();
    //开启服务器
    startServer();

    initUserManageMent();//登录后默认进入用户管理界面

    connect(ui->listView, &QListView::clicked, this, &backstorg::onListItemClicked);//点击左侧列表发送信号
    connect(this, &backstorg::itemClicked, this, &backstorg::UsersManagement);//根据对应点击的List信号做出响应
    connect(ui->addUserButton,&QPushButton::clicked,this,&backstorg::onAddUserClicked);//增加用户按钮
    connect(ui->deleteUserButton,&QPushButton::clicked,this,&backstorg::onDeleteUserClicked);//删除用户按钮
    connect(socket,&QTcpSocket::disconnected,this,&backstorg::backLogin);//断开连接
    connect(ui->refreshButton,&QPushButton::clicked,this,&backstorg::onUserRefreshButton);//用户刷新按钮
    connect(ui->exitButton,&QPushButton::clicked,this,&backstorg::onExitButton);
    connect(ui->modifyUserButton,&QPushButton::clicked,this,&backstorg::onModifyUserCilcked);//删除用户按钮
    connect(pagination, &Pagination::pageChanged, this, &backstorg::updateTableView);// 连接 Pagination 的信号
    connect(timer, &QTimer::timeout, this, &backstorg::updateTimeLabel);//定时器显示时间
    connect(ui->confirmButton,&QPushButton::clicked,this,&backstorg::pushAttendenceTime);//提交考勤时间

    // 设置定时器的超时时间为1秒
    timer->setInterval(1000);
    timer->start();

}

backstorg::~backstorg()
{
    delete ui;
}

//设置标题
void backstorg::setTile()
{
    //设置顶端标题样式
    ui->toplabel->setStyleSheet("QLabel { background-color: rgb(79,166,126); }");
    ui->toplabel->setFont(QFont("Lucida Console",24));
    ui->toplabel->setText("考勤系统后台");
    ui->toplabel->setAlignment(Qt::AlignCenter); // 设置文本居中对齐
}

//设置左侧List
void backstorg::setList()
{
    //设置左侧List样式
    QStringList list;
    list.append("用户管理");
    list.append("考勤管理");
    QStringListModel *listmodel = new QStringListModel(list);
    ui->listView->setModel(listmodel);
    ui->listView->setSpacing(4);
    ui->listView->setStyleSheet("QListView { background-color: lightgray;font-size:25px}");
    ui->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

}
//掩藏用户管理按钮
void backstorg::hideUserManageMentButton(bool flag)
{
    if(flag)
    {
        ui->addUserButton->hide();
        ui->refreshButton->hide();
        ui->modifyUserButton->hide();
        ui->deleteUserButton->hide();
        ui->exitButton->hide();
    }
    else
    {
        ui->addUserButton->show();
        ui->refreshButton->show();
        ui->modifyUserButton->show();
        ui->deleteUserButton->show();
        ui->exitButton->show();
    }
}

//隐藏考勤界组件
void backstorg::hideAttendenceWidget(bool flag)
{
    if(flag)
    {
        ui->startTime->hide();
        ui->endTime->hide();
        ui->acturalLabel->hide();
        ui->totalLabel->hide();
        ui->totalNum->hide();
        ui->acturalNum->hide();
        ui->confirmButton->hide();
        ui->timeLabel->hide();
        ui->text->hide();
    }
    else
    {
        ui->startTime->show();
        ui->endTime->show();
        ui->acturalLabel->show();
        ui->totalLabel->show();
        ui->totalNum->show();
        ui->acturalNum->show();
        ui->confirmButton->show();
        ui->timeLabel->show();
        ui->text->show();
    }
}

// 更新表格视图，根据当前页和总页数显示相应的行
void backstorg::updateTableView(int currentPage, int totalPages)
{
    // 计算当前页的起始行和结束行
    int startRow = (currentPage - 1) * pagination->itemsPerPage();
    int endRow = qMin(startRow + pagination->itemsPerPage(), model->rowCount());

    // 遍历模型中的所有行，根据当前页设置行的可见性
    for (int row = 0; row < model->rowCount(); ++row)
    {
        bool isVisible = (row >= startRow && row < endRow);
        ui->tableView->setRowHidden(row, !isVisible); // 设置行的可见性
    }

    // 更新页码显示
    pageLineEdit->setText(QString::number(currentPage)); // 显示当前页码
    totalPagesDisplay->setText(QString::number(totalPages)); // 显示总页数
}

//设置表格属性
void backstorg::setUpTableView()
{
    // 设置表格内容居中
       ui->tableView->setStyleSheet("QTableView::item { text-align: center; }");

       // 设置表头的字体和样式
       QFont font = ui->tableView->horizontalHeader()->font();
       font.setPointSize(14); // 设置字体大小
       font.setBold(true); // 设置字体加粗
       ui->tableView->horizontalHeader()->setFont(font);

       // 设置表头居中
       ui->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
       ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

       ImageCenterDelegate *delegate = new ImageCenterDelegate();
       ui->tableView->setItemDelegate(delegate);

       // 创建分页控件布局
       pageLayout->addWidget(prevButton);
       pageLayout->addWidget(nextButton);
       pageLayout->addWidget(pageLineEdit);
       pageLayout->addWidget(totalPagesLabel);
       pageLayout->addWidget(totalPagesDisplay);

       // 设置跳转页码的输入框长度
       pageLineEdit->setFixedWidth(50);
       pageLineEdit->setReadOnly(true);

       //设置总页数字体的宽度
       totalPagesLabel->setFixedWidth(80);

        // 设置按钮的固定宽度
        prevButton->setFixedSize(100,30);
        nextButton->setFixedSize(100,30);

       connect(prevButton, &QPushButton::clicked, pagination, &Pagination::showPreviousPage);
       connect(nextButton, &QPushButton::clicked, pagination, &Pagination::showNextPage);
       // 创建分页控件的容器
       QWidget *paginationContainer = new QWidget(this);
       paginationContainer->setLayout(pageLayout);
       paginationContainer->setGeometry(140, 1080 - 50, 1781, 50); // 设置分页控件的位置和大小，假设分页控件高度为50

       //默认页数为1
       pageLineEdit->setText("1");

       //设置每页的行数为20
       pagination->setItemsPerPage(20);

       pagination->setModel(model);

}

//隐藏表格
void backstorg::hideTable(bool flag)
{
    int rowCount = model->rowCount(); // 获取表格的行数
    if(flag)
    {
        for (int row = 1; row < rowCount; ++row)
        {
            ui->tableView->hideRow(row); // 隐藏每一行
        }
    }
    else
    {
        for (int row = 1; row < rowCount; ++row)
        {
            ui->tableView->showRow(row); // 隐藏每一行
        }
    }

}
//设置按钮样式
void backstorg::setButton()
{
    QFont buttonFont;
    buttonFont.setBold(true);

    // 应用字体到按钮
    ui->addUserButton->setFont(buttonFont);
    ui->deleteUserButton->setFont(buttonFont);
    ui->modifyUserButton->setFont(buttonFont);
    ui->refreshButton->setFont(buttonFont);
}

//初始化用户管理界面
void backstorg::initUserManageMent()
{
    hideTable(false);
    hideAttendenceWidget(true);

    //查询数据库到表格显示
    if(!showUsers())
    {
        backLogin();
    }
}

//点击左侧的list表
void backstorg::onListItemClicked(const QModelIndex &index)
{
    QString itemText = index.data().toString();
    emit itemClicked(itemText);
}

//点击用户管理和考勤管理的槽函数
void backstorg::UsersManagement(const  QString &itemString)
{
    if(itemString == "用户管理")
    {
      //  hideTable(true);
        hideUserManageMentButton(false);
        hideAttendenceWidget(true);
        if(!showUsers())
        {
            backLogin();
        }
    }
    else if(itemString == "考勤管理")
    {
        //hideUserManageMentButton(true);
        attendenceManagement();
        showAttendence();
    }
    else
    {
       QMessageBox::information(this,"提示","NULL");
    }
}
bool backstorg::showUsers()
{
    if(socket->state() == QAbstractSocket::ConnectingState)
    {
        QMessageBox::information(this,"warning","数据库连接中");
        return false;
    }

   //构造查询消息类型
   QByteArray message;
   QDataStream stream(&message,QIODevice::WriteOnly);
   stream << SHOW_USERS;

   //发送查询消息
   socket->write(message);
   socket->flush();

   QMessageBox *messageBox = new QMessageBox(this);
   messageBox->setWindowTitle("稍等");
   messageBox->resize(100, 200); // 设置消息框大小
   messageBox->show();


   QThread::sleep(1);
   delete  messageBox;

   if(socket->waitForReadyRead(3000))
   {
       QDataStream responseStream(socket);
       int responseType;
       std::vector<int>imgs_size;
       std::vector<QByteArray>images;
       int overFlag;

       responseStream >> responseType;
       qDebug()<<responseType;

       if(responseType == SHOW_USERS_RES)
       {
          QList<QStringList>usersList;
          responseStream >> usersList;
          //保存传输过来的每张照片大小
          for(int i = 0;i<usersList.size();i++)
          {
              int img_size;
              responseStream >> img_size;
              imgs_size.push_back(img_size);
          }
          //保存传输的每张图片信息
          for(int i = 0;i<usersList.size();i++)
          {
              QByteArray imgData;
              //imgData.resize(static_cast<int>(imgs_size[i]));
              while(imgData.size() < imgs_size[i])
              {
                  if(!socket->bytesAvailable())
                  {
                      socket->waitForReadyRead(3000);
                  }
                 QByteArray temp = socket->read(imgs_size[i] - imgData.size());
                 imgData.append(temp);
              }
              images.push_back(imgData);
          }

          while(overFlag != OVER)
          {
              responseStream >> overFlag;
          }


          qDebug()<<usersList.size();
//          qDebug()<<images.size();
//          qDebug()<<overFlag;

        if(1)
        {
          //清空模型现有数据
          model->clear();
          //设置表头
          model->setHorizontalHeaderLabels({"序号","姓名","院系","照片"});
          for (int row = 0; row < usersList.size(); ++row)
          {
             for (int col = 0; col < usersList[row].size(); ++col)
             {
                 QStandardItem *item = new QStandardItem(usersList[row][col]);
                 item->setTextAlignment(Qt::AlignCenter);
                 model->setItem(row, col, item);
             }
             if (!images[row].isEmpty() && imgs_size[row])
             {
                 QPixmap pixmap;
                 pixmap.loadFromData(images[row]);

                 // 将图像显示在表格中
                 QStandardItem *item = new QStandardItem();
                 item->setData(QVariant(pixmap.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation)), Qt::DecorationRole);
                 item->setTextAlignment(Qt::AlignCenter);
                 model->setItem(row, 3, item);

             }
          }
          //将模型设置为tableView
          ui->tableView->setModel(model);

          // 设置列宽比数据稍长一点
          for (int col = 0; col < model->columnCount(); ++col)
          {
              ui->tableView->setColumnWidth(col, ui->tableView->columnWidth(col) + 40);
          }
          hideUserManageMentButton(false);
          hideTable(false);

          // 计算总页数
         int totalPages = (usersList.size() + pagination->itemsPerPage() - 1) / pagination->itemsPerPage();

         // 更新表格视图，只显示第一页的数据
         updateTableView(1, totalPages);
        }
           return true;
       }
       else//没有接受到服务器的响应信号
       {
           QMessageBox::information(this,"error","读取服务器数据响应失败");
           return false;
       }

   }
   else //没有接受到可读信号
   {
          QMessageBox::information(this,"error","连接服务器超时");
          return false;
   }
}
//增加用户信息
void backstorg::addUser(const QString &name, const QString &dept, const QString &fileName,const qint64&img_size,const QByteArray& img_data)
{

    QByteArray message;
    QDataStream stream(&message,QIODevice::WriteOnly);
    stream << static_cast<int>(ADD_USER);
    stream <<name<<dept<<fileName<<img_size;
    stream.writeRawData(img_data.data(),img_size);
    socket->write(message);
    socket->flush();

    if(socket->waitForReadyRead(30000))
    {
        QDataStream response(socket);
        int responseType;
        response >> responseType;

        if(responseType == ADD_USER_RES)
        {
            QMessageBox::information(this,"提示","用户添加成功");
        }
        else
        {
            QMessageBox::information(this,"提示","用户添加失败");
        }

    }

}

//点击添加用户
void backstorg::onAddUserClicked()
{
    bool ok;
    QString name = QInputDialog::getText(this, "输入姓名", "姓名:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;

    QString dept = QInputDialog::getText(this, "输入院系", "院系:", QLineEdit::Normal, "", &ok);
    if (!ok || dept.isEmpty()) return;

    QString img_path = QFileDialog::getOpenFileName(this, "选择照片", "", "Images (*.png *.xpm *.jpg)");
    if (img_path.isEmpty()) return;

    QFileInfo fileInfo(img_path);
    QString fileName = fileInfo.fileName();

    //读取照片文件
    QFile img_file(img_path);
    if(!img_file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this,"提示","无法打开文件");
        return;
    }

    QByteArray img_data = img_file.readAll();//文件数据
    qint64 img_size = img_file.size();//文件大小
    qDebug()<<img_size;

    //关闭文件
    img_file.close();
    addUser(name, dept, fileName,img_size,img_data);
}
//点击删除用户
void backstorg::onDeleteUserClicked()
{
    bool ok;
    QString id = QInputDialog::getText(this,"请输入用户ID","ID",QLineEdit::Normal,"",&ok);
    if(!ok || id.isEmpty())
    {
        return ;
    }
   deleteUser(id);
}
//删除用户
bool backstorg::deleteUser(QString &id)
{
    int ID = id.toInt();
    QByteArray message;
    QDataStream stream(&message,QIODevice::WriteOnly);


    stream << DELETE_USER;
    stream << ID;

    socket->write(message);
    socket->flush();

    QThread::msleep(10);

    if(socket->waitForReadyRead(300))
    {
        QDataStream response(socket);
        int responseType;
        response >> responseType;
        if(responseType == DELETE_USER_RES)
        {
            QMessageBox::information(this,"提示","删除用户成功");
            return true;
        }
        else
        {
            QMessageBox::information(this,"提示","删除用户失败");
            return false;
        }
    }
    QMessageBox::information(this,"提示","连接服务器超时");
    return false;
}

//修改用户按钮
void backstorg::onModifyUserCilcked()
{
    bool ok;
    QString ID = QInputDialog::getText(this,"请输入用户ID","ID",QLineEdit::Normal,"",&ok);
    if(!ok || ID.isEmpty())
    {
        QMessageBox::information(this,"警告","输入的ID不能为空!");
        return;
    }
    QString name = QInputDialog::getText(this,"请输入要修改的姓名","姓名",QLineEdit::Normal,"",&ok);
    if(!ok)
    {
        QMessageBox::information(this,"错误","获取姓名失败");
        return;
    }
   QString dept = QInputDialog::getText(this,"请输入要修改的院系","院系",QLineEdit::Normal,"",&ok);
   if(!ok)
   {
       QMessageBox::information(this,"错误","获取院系信息失败");
       return;
   }
   QString imgPath = QFileDialog::getOpenFileName(this,"选择照片","","Images (*.png *.xpm *.jpg)");
   QByteArray imgData;
   QString fileName;
   qint64 imgSize = 0;
   //如果图像不为空
   if(!imgPath.isEmpty())
   {
       QFileInfo fileInfo(imgPath);
       //图像名称
       fileName = fileInfo.fileName();

       QFile file(imgPath);
       if(!file.open(QIODevice::ReadOnly))
       {
           QMessageBox::information(this,"错误","无法打开文件");
           return;
       }
       //图像大小
       imgData = file.readAll();
       imgSize = imgData.size();

       file.close();
   }
   //只修改姓名和部门
   if(!name.isEmpty() && !dept.isEmpty() && imgPath.isEmpty())
   {
       int id = ID.toInt();
       qDebug()<<"修改姓名和部门";
       modifyUser(id,name,dept);
   }
   //只修改照片
   else if(name.isEmpty() && dept.isEmpty() && imgData.size())
   {
       int id = ID.toInt();
       qDebug()<<"修改照片";
       modifyUser(id,imgSize,fileName,imgData);
   }

}
//只修改姓名和部门
bool backstorg::modifyUser(int id, const QString& newName, const QString& newDept)
{
    QByteArray message;
    QDataStream instream(&message,QIODevice::WriteOnly);
    //构建信息流
    instream << MODIFY_USER_NAEM_DEPT;
    instream << id;
    instream << newName;
    instream << newDept;
    //发送出去
    socket->write(message);
    socket->flush();

    if(socket->waitForReadyRead(30000))
    {
        //收取信息
        QDataStream responseStream(socket);
        int responseType;
        responseStream >> responseType;

        if(responseType == MODIFY_USER_RES)
        {
            QMessageBox::information(this,"提示","用户修改成功");
            return true;
        }
        else if(responseType == FALIURE)
        {
             QMessageBox::information(this,"提示","用户修改失败");
             return false;
        }

    }
    QMessageBox::information(this,"提示","获取服务器响应失败");
    return false;
}
//只修改用户图像
bool backstorg::modifyUser(int id, qint64 newImgSize,const QString &fileName, const QByteArray& newImgData)
{

    QByteArray message;
    QDataStream instream(&message,QIODevice::WriteOnly);
    //构建信息流
    instream << MODIFY_USER_IMGDATA;
    instream << id;
    instream << newImgSize;
    instream << fileName;

    instream.writeRawData(newImgData.data(),newImgSize);


    //发送出去
    socket->write(message);
    socket->flush();

    if(socket->waitForReadyRead(30000))
    {
        //收取信息
        QDataStream responseStream(socket);
        int responseType;
        responseStream >> responseType;

        if(responseType == MODIFY_USER_RES)
        {
            QMessageBox::information(this,"提示","用户修改成功");
            return true;
        }
        else if(responseType == FALIURE)
        {
             QMessageBox::information(this,"提示","用户修改失败");
             return false;
        }

    }
    QMessageBox::information(this,"提示","获取服务器响应失败");
    return false;
}
//用户界面刷新
void backstorg::onUserRefreshButton()
{
    showUsers();
}

//退出按钮
void backstorg::onExitButton()
{
    //断开连接，返回登录界面
    backLogin();
}

//考勤管理部分-------------------------------------------------------------------------------------

//获取当前时间
void backstorg::updateTimeLabel()
{
    // 获取当前时间并将其格式化为“年-月-日 时:分:秒”形式
   QDateTime currentDateTime = QDateTime::currentDateTime();
   QString currentDateTimeStr = currentDateTime.toString("yyyy-MM-dd hh:mm:ss");

   // 设置标签文本为当前时间
   ui->timeLabel->setText(currentDateTimeStr);
  // ui->attendenceLabel->show();
}

//考勤管理界面
void backstorg::attendenceManagement()
{
    //隐藏用户管理界面的按钮
    hideUserManageMentButton(true);
    //显示考勤界面组件
    hideAttendenceWidget(false);
    //清空模型现有数据
    model->clear();
    //重新设置表头
    model->setHorizontalHeaderLabels({"序号","姓名","院系","考勤情况"});
    //分页情况清空
    updateTableView(1,1);

}
//验证时间是否有效
bool backstorg::isValidTime(const QString& timeStr)
{
    QRegExp timeRegex("^(?:[01]\\d|2[0-3]):[0-5]\\d$"); // 正则表达式，匹配24小时制的时间格式
    return timeRegex.exactMatch(timeStr);
}
//提交考勤时间
void backstorg::pushAttendenceTime()
{
    //获取用户设置的考勤时间
    QString startTime = ui->startTime->text();
    QString endTime = ui->endTime->text();
    //验证考勤时间是否合理
    if (!isValidTime(startTime) || !isValidTime(endTime))
    {
        QMessageBox::warning(this, "Error", "时间格式必须是24小时格式");
        return;
    }

    // 验证结束时间是否在起始时间之后
    QTime start = QTime::fromString(startTime, "HH:mm");
    QTime end = QTime::fromString(endTime, "HH:mm");

    if (end <= start)
    {
        QMessageBox::warning(this, "Error", "结束时间必须在开始时间之后.");
        return;
    }
    //构建请求，发送到开发板
    QByteArray message;
    QDataStream request(&message,QIODevice::WriteOnly);

    request << SET_TIME;
    request << startTime; // 将起始时间转换为字符串并序列化
    request << endTime;   // 将结束时间转换为字符串并序列化

    socket->write(message);
    socket->flush();

    if(socket->waitForReadyRead(30000))
    {
        QDataStream responseStream(socket);
        int over_flag;
        responseStream >> over_flag;
        if(over_flag == OVER)
        {
            ui->acturalNum->display(0);
            showAttendence();
            QMessageBox::information(this,"提示","设置成功");
        }
        else
        {
            QMessageBox::information(this,"提示","设置失败");
        }
    }
    else
    {
        QMessageBox::information(this,"提示","获取服务器响应失败");
    }
}
//展示考勤数据
void backstorg::showAttendence()
{
    QByteArray message;
    QDataStream stream(&message,QIODevice::WriteOnly);

    stream << SHOW_ATTEND;
    socket->write(message);
    socket->flush();
    //如果有响应
    while(1)
    {
        if(socket->waitForReadyRead(30000))
        {
            int responseType;
            int total;
            int attendence_num;
            QList<QStringList>usersList;
            QDataStream responseStream(socket);
            responseStream >> responseType;

            if(responseType == SHOW_ATTEND_RES)
            {
                responseStream >>total;
                responseStream >>attendence_num;
                responseStream >> usersList;

                ui->acturalNum->display(attendence_num);
                ui->totalNum->display(total);
                //清空模型现有数据
                model->clear();
              //设置表头
              model->setHorizontalHeaderLabels({"序号","姓名","院系","考勤情况"});
              for (int row = 0; row < usersList.size(); ++row)
              {
                 for (int col = 0; col < usersList[row].size(); ++col)
                 {
                     QStandardItem *item = new QStandardItem(usersList[row][col]);
                     item->setTextAlignment(Qt::AlignCenter);
                     model->setItem(row, col, item);
                 }
              }

              //将模型设置为tableView
              ui->tableView->setModel(model);

              // 设置列宽比数据稍长一点
              for (int col = 0; col < model->columnCount(); ++col)
              {
                  ui->tableView->setColumnWidth(col, ui->tableView->columnWidth(col) + 40);
              }
              // 计算总页数
             int totalPages = (usersList.size() + pagination->itemsPerPage() - 1) / pagination->itemsPerPage();

             // 更新表格视图，只显示第一页的数据
             updateTableView(1, totalPages);

           }
            else
            {
                 QMessageBox::information(this,"提示","获取数据失败");
            }
        }
        else
        {
            QMessageBox::information(this,"提示","获取服务器响应超时");
        }
        break;
    }

}
//与龙芯派的对话
void backstorg::readMessage()
{
    QTcpSocket *s = static_cast<QTcpSocket *>(sender());
    QDataStream stream(s);

    int total;
    int attendence_num;
    int id;
    QString attendenceType;


    stream >>total>>attendence_num>>id>>attendenceType;
    ui->totalNum->display(total);
    ui->acturalNum->display(attendence_num);
//        // 查找表格中匹配的 ID 行并更新考勤情况
//       for (int row = 0; row < model->rowCount(); ++row)
//       {
//           QStandardItem *item = model->item(row, 0); // 假设 ID 在第 0 列
//           if (item && item->text() == id)
//           {
//               QStandardItem *attendanceItem = new QStandardItem(attendenceType);
//               attendanceItem->setTextAlignment(Qt::AlignCenter);
//               model->setItem(row, 3, attendanceItem); // 假设考勤情况在第 3 列
//               break;
//           }
//       }
       qDebug()<<"打卡成功";
       showAttendence();

//       QByteArray message;
//       QDataStream responseStream(&message,QIODevice::WriteOnly);

//       responseStream << OVER;
//       s->write(message);
//       s->flush();

}
//龙芯派的新连接
void backstorg::newConnection()
{
    while(server->hasPendingConnections())
    {
        QTcpSocket *socket = server->nextPendingConnection();
        if(socket)
        {
            qDebug()<<"连接成功";
            connect(socket,&QTcpSocket::disconnected,socket,&QTcpSocket::deleteLater);
            connect(socket,&QTcpSocket::readyRead,this,&backstorg::readMessage);
        }
    }
}
//开启服务器
void backstorg::startServer()
{
    if(!server->listen(QHostAddress("192.168.0.102"),10086))
      {
          qDebug()<<"开启服务器失败";
          return;
      }
      qDebug()<<"服务器开启成功";
      connect(server,&QTcpServer::newConnection,this,&backstorg::newConnection);
}

void backstorg::backLogin()
{
   socket->disconnectFromHost();
   this->hide();
   widget->show();
}
