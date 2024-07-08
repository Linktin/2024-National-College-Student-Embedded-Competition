#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>
#include "db.h"
#include "face.h"
#include "user.h"

#define database "/home/hy/code/databases/face.db"//数据库路径


void createTable(sqlite3 *ppdb)
{
    const char *sql = "CREATE TABLE IF NOT EXISTS users ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "name TEXT NOT NULL,"
                      "dept TEXT NOT NULL,"
                      "face_path BLOB NOT NULL);";
    char *errmsg = nullptr;
    int r = sqlite3_exec(ppdb, sql, 0, 0, &errmsg);

    if (r != SQLITE_OK)
    {
        std::cerr << errmsg << std::endl;
        sqlite3_free(errmsg); // 释放错误信息内存
    }
    else
    {
        std::cout << "创建表成功" << std::endl;
    }
}

void insertTable(sqlite3 *ppdb)
{
    const char *sql = "INSERT INTO users (name, dept, face_path) "
                      "VALUES ('何烨', '电子系', '/home/hy/code/face/hy.jpg');";
    char *errmsg = nullptr;
    int r = sqlite3_exec(ppdb, sql, 0, 0, &errmsg);

    if (r != SQLITE_OK)
    {
        std::cerr << errmsg << std::endl;
        sqlite3_free(errmsg); // 释放错误信息内存
    }
    else
    {
        std::cout << "插入数据成功" << std::endl;
    }
}

int callback(void *arg, int num, char **values, char **name)
{
    std::vector<User>*users = static_cast<std::vector<User>*>(arg);
    if(num >= 3 && values)
    {
        std::string id_str = values[0];
        int id = std::stoi(id_str);
        std::string name = values[1];
        std::string dept = values[2];
        std::string face_path = values[3];
        User user(id,name,dept,face_path);
        users->push_back(user);
     // std::cout<<user.GetName()<<" "<<user.GetDept()<<" "<<user.GetFacePath()<<std::endl;
    }
    return 0;
}

void SelectData(sqlite3 *ppdb,std::vector<User>& facedb)
{
    const char *sql = "SELECT * FROM users;";
    char *errmsg = nullptr;
    int r = sqlite3_exec(ppdb, sql, callback, &facedb, &errmsg);

    if (r != SQLITE_OK)
    {
        std::cerr << errmsg << std::endl;
        sqlite3_free(errmsg); // 释放错误信息内存
    }
}

void loadFaceDb(std::vector<User>& facedb)
{
    sqlite3 *ppdb;
    //打开数据库连接
    int r = sqlite3_open(database,&ppdb);
    if(r != SQLITE_OK)
    {
        std::cerr<<"数据库打开失败"<<std::endl;
        return;
    }
    createTable(ppdb);
    //查找数据并放入容器
    SelectData(ppdb,facedb);
    //关闭数据库
    sqlite3_close(ppdb);
}
