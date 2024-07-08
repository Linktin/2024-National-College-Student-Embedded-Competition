#ifndef __UTILS_H__
#define __UTILS_H_

#include <QtWidgets/QApplication>
#include <QtSql/QSqlDatabase>
QString GetCurrentDate();
QString GetCurrentTime();
bool modifyFileName(const QString& filePath, const QString& newFileName);//修改文件名
QSqlDatabase OpenDataBase();//打开数据库连接
#endif
