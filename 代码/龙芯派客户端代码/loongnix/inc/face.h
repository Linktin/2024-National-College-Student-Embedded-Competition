#ifndef __FACE_H__
#define __FACE_H__
#include <iostream>
#include<string>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include<opencv2/opencv.hpp>
#include<QtNetwork/QTcpSocket>
#include <opencv2/dnn/dnn.hpp>
#include<vector>
#include "sface.h"
#include "widget.h"
#include "user.h"
//using namespace cv;
#define zoom 4

class DetectedFace
{
public:
    cv::Mat faces;//人脸区域
    float conf;//置信度
    int x2,y2, w,h;//人脸矩形位置
    int landmark[5][2];//人脸五个关键点坐标
    int nums;//人脸个数
    std::string name;//姓名
    std::string dept;//部门
    std::string img_path;//头像位置
public:
    DetectedFace(){};
    ~DetectedFace(){faces.release();};
    cv::Mat visualize(const cv::Mat& image, float fps = -1.f);

      // 拷贝构造函数
    DetectedFace(const DetectedFace &other)
    {
        faces = other.faces.clone(); // 深拷贝
        conf = other.conf;
        x2 = other.x2;
        y2 = other.y2;
        w = other.w;
        h = other.h;
        nums = other.nums;
        name = other.name;
        dept = other.dept;
        img_path = other.img_path;
        for (int i = 0; i < 5; ++i)
        {
            landmark[i][0] = other.landmark[i][0];
            landmark[i][1] = other.landmark[i][1];
        }
    }

     // 赋值运算符重载
    DetectedFace& operator=(const DetectedFace &other)
    {
        if (this != &other)
        {
            faces = other.faces.clone(); // 深拷贝
            conf = other.conf;
            x2 = other.x2;
            y2 = other.y2;
            w = other.w;
            h = other.h;
            nums = other.nums;
            name = other.name;
            dept = other.dept;
            img_path = other.img_path;
            for (int i = 0; i < 5; ++i)
            {
                landmark[i][0] = other.landmark[i][0];
                landmark[i][1] = other.landmark[i][1];
            }
        }
        return *this;
    }
};

cv::VideoCapture GetCapture();
void FeatureCompare(Widget &widget);
void ProcessFrame(const cv::Mat &frame,const cv::Size &size);
void DisplayFrame(QLabel * label, const cv::Mat& frame,float fps);
void WakeFeatureCompare(int &flag);
void FaceDetect(QLabel *label);
bool RefreashItem();
void FaceDetect(QLabel *label,Widget &widget);
void LoadUsers();//加载用户信息
bool AddFaceDB(User &user);
bool DeleteFaceDB(User &d_user);
bool ModifyFaceDB(User &user);
void UpdateFaceDB();
std::pair<int,int>GetFaceDBNum();
void UpdateUserAttendce();
void ShowAttendence(QTcpSocket *socket);
QString ConvertAttendenceType(AtendenceType type);
cv::Mat GetFaceFeature(std::string face_path);//根据一张图片推理人脸特征
void SendUser(User &user,Widget &widget);
std::pair<User,bool> GetUser(SFace &sface,cv::Mat &feature,Widget &widget);
cv::Mat applyWhiteBalance(cv::Mat& image);
cv::Mat adjustBrightness(cv::Mat& image, double alpha, int beta);
cv::Mat adjustLighting(cv::Mat& image);
cv::Mat prepro_img(cv::Mat& image);
cv::Mat convertToHSV(cv::Mat& image);
bool isOverexposedRegion(double meanV);
void face_proc(cv::Mat& image,DetectedFace& dface);
void dynamicRangeCompression(cv::Mat& image, double gamma);
std::vector<double> extractRppgSignal(cv:: Mat& frame, cv::Rect& faceRegion);
bool isLiveFace(std:: vector<double>& rppgSignal);
#endif