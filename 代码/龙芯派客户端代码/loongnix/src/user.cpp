#include "user.h"
#include "widget.h"
#include "utils.h"
#include <string>
#include <opencv2/opencv.hpp>
#include <QtCore/QTime>

User::User(int id,const std::string& name,const std::string& dept,const  std::string face_path)
{
    this->id = id;
    this->name = name;
    this->dept = dept;
    this->face_path = face_path;
    this->absence_state = NON;
}
std::string User::GetName()
{
    return name;
}
std::string User::GetDept()
{
    return dept;
}
std::string User::GetFacePath()
{
    return face_path;
}
int User::GetID()
{
    return id;
}
User::User()
{

}
User::~User()
{
    feature.release();
};
// 拷贝构造函数
User::User(const User &other)
{
    id = other.id;
    name = other.name;
    dept = other.dept;
    face_path = other.face_path;
    absence_state = other.absence_state;
    feature = other.feature.clone(); // 深拷贝
}
// 赋值运算符重载
User& User::operator=(const User &other)
{
    if (this != &other)
    {
        id = other.id;
        name = other.name;
        dept = other.dept;
        face_path = other.face_path;
        absence_state = other.absence_state;
        feature = other.feature.clone(); // 深拷贝
    }
    return *this;
}
void User::SetName(std::string name)
{
    this->name = name;
}
void User::SetDept(std::string dept)
{
    this->dept = dept;
}
void User::SetFacePath(std::string face_path)
{
    this->face_path = face_path;
}
void User::SetID(int id)
{
    this->id = id;
}

AtendenceType User::isAbsence(Widget *widget)
{
    //获取当前考勤时间
    std::pair<QTime,QTime>res = widget->GetCurrentAttendenceTime();
    QTime start = res.first;
    QTime end = res.second;

    //当前时间
    QTime current = QTime::fromString(GetCurrentTime());
    //未开始考勤
    if(current < start  ||   (end <= start))
    {
        absence_state = NOTSTARTED;
        return NOTSTARTED;
    }
        
    //正常出勤    
    if(current >= start && current <=end)
    {
        absence_state = PRESENT;
        return PRESENT;
    }
       
    //迟到
    if(end.secsTo(current) <= 3 )
    {
        absence_state = LATE;
        return LATE;
    }
    //缺席
    absence_state = ABSENT;
    return ABSENT;            
}