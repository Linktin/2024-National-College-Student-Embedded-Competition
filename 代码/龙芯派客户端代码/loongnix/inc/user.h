#ifndef __USER_HPP__
#define __USER_HPP__
enum AtendenceType
{
    NON,
    NOTSTARTED,
    PRESENT,
    LATE,
    ABSENT
};
#include "widget.h"
#include <string>
#include<opencv2/opencv.hpp>
class Widget;

class User
{
private:
    /* data */
    int id;
    std::string name;
    std::string dept;
    std::string face_path;
public:
    cv::Mat feature;
    AtendenceType  absence_state = NON; //考勤状态
    User();
    User(int id,const std::string& name,const std::string& dept,const  std::string face_path);
    User(const User &other);
    User& operator=(const User &other);
    std::string GetName();
    std::string GetDept();
    std::string GetFacePath();
    int GetID();
    void SetID(int id);
    void SetName(std::string name);
    void SetDept(std::string dept);
    void SetFacePath(std::string face_path);
    AtendenceType isAbsence(Widget *widget);
    ~User();
};

#endif