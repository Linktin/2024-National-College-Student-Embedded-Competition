#include <opencv2/opencv.hpp>
#include <QtWidgets/QApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDataStream>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlDatabase>
#include <QtCore/QDir>
#include<ctime>
#include <string>
#include "utils.h"
#include "widget.h"
#include "face.h"
//返回当前日期
QString GetCurrentDate()
{
    time_t now = time(0); // 获取当前时间
    tm* ltm = localtime(&now);

    char timeStr[20];
    strftime(timeStr,sizeof(timeStr),"%Y-%m-%d",ltm);

    return QString::fromStdString(std::string(timeStr));
}

//返回当前时间
QString GetCurrentTime()
{
    time_t now = time(0); 
    tm* ltm = localtime(&now);

    char str[9];
    strftime(str,sizeof(str),"%H:%M:%S",ltm);
    
    return QString::fromStdString(std::string(str));
}

//修改文件名
bool modifyFileName(const QString& filePath, const QString& newFileName)
{
    QFile file(filePath);

    // 检查文件是否存在
    if (!file.exists())
    {
        qDebug() << "文件不存在.";
        return false;
    }

    // 获取文件路径的目录和文件名
    QFileInfo fileInfo(file);
    QString directory = fileInfo.absoluteDir().path();
    QString oldFileName = fileInfo.fileName();

    // 构建新的文件路径
    QString newFilePath = directory + "/" + newFileName;

    // 修改文件名
    if (file.rename(newFilePath))
    {
        qDebug() << "File name modified successfully.";
        return true;
    } else
    {
        qDebug() << "Failed to modify file name.";
        return false;
    }
}

//打开数据库
QSqlDatabase OpenDataBase()
{
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
       return QSqlDatabase();
    }

    return facedb;
}
