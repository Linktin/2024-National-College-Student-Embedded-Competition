#include <opencv2/dnn/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/opencv.hpp>
#include <QtWidgets/QApplication>
#include<QtNetwork/QTcpSocket>
#include <QtCore/QStringList>
#include <QtWidgets/QLabel>
#include <QtGui/QImage>
#include <condition_variable>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <signal.h>
#include "face.h"
#include "sface.h"
#include "yunet.h"
#include "db.h"
#include "user.h"
#include "utils.h"
#include "widget.h"
#include "voice.h"
#define face_detect_path "/home/hy/code/face_detect.onnx"
#define face_recognition_path "/home/hy/code/face_recognition512_97.2.onnx"
using namespace cv;
using namespace std;

YuNet model(face_detect_path, cv::Size(160, 120), 0.9, 0.3, 5000, 0, 0);
SFace sface(face_recognition_path, 0, 0, 0);

DetectedFace reface;
std::vector<User>facedb;

std::mutex mtx;
std::mutex mtx2;


std::condition_variable cond;
bool ready = false;


cv::Mat hy = cv::imread("/home/hy/code/face/hy.jpg");
cv::Mat fuben;

cv::Size img_size(160,120);

const double OVEREXPOSURE_THRESHOLD = 0.4; // 过曝光阈值

// 保存识别到的人脸
void SaveFace(Mat &face)
{
    cv::Mat resized_face;
    cv::resize(face, resized_face, img_size);
    cv::imwrite("./heye.jpg", resized_face);
}

//获取摄像头
cv::VideoCapture GetCapture()
{
    cv::VideoCapture cap(0);
    if(!cap. isOpened())
    {
        std::cerr<<"请检查摄像头是否打开"<<std::endl;
        exit(-1);
    }
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    return  cap;
}

//人脸可视化
cv::Mat DetectedFace::visualize(const cv::Mat& image, float fps)
{
    static cv::Scalar box_color{0, 255, 0};
    static std::vector<cv::Scalar> landmark_color{
        cv::Scalar(255,   0,   0), // 右眼睛  蓝色
        cv::Scalar(  0,   0, 255), // 左眼睛  红色
        cv::Scalar(  0, 255,   0), // 鼻子  绿色
        cv::Scalar(255,   0, 255), // 左嘴角 紫色
        cv::Scalar(  0, 255, 255)  // 右嘴角 黄色
    };
    static cv::Scalar text_color{0, 255, 0};
    auto output_image = image.clone();
    if (fps >= 0)
    {
        cv::putText(output_image, cv::format("FPS: %.2f", fps), cv::Point(0, 15), cv::FONT_HERSHEY_SIMPLEX, 0.5, text_color, 2);
    }

    for (int i = 0; i < faces.rows; ++i)
    {
        // 给人脸画矩形
        x2 = static_cast<int>(faces.at<float>(i, 0))  *zoom;
        y2 = static_cast<int>(faces.at<float>(i, 1))  *zoom;
        w = static_cast<int>(faces.at<float>(i, 2))   *zoom;
        h = static_cast<int>(faces.at<float>(i, 3))   *zoom;
        cv::rectangle(output_image, cv::Rect(x2, y2, w, h), box_color, 2);

        // 置信度
        conf = faces.at<float>(i, 14);
        cv::putText(output_image, cv::format("%.4f", conf), cv::Point(x2, y2+12), cv::FONT_HERSHEY_DUPLEX, 0.5, text_color);

        //人脸的五个点
        for (int j = 0; j < landmark_color.size(); ++j)
        {
            landmark[j][0] = static_cast<int>(faces.at<float>(i, 2*j+4))*zoom, landmark[j][1] = static_cast<int>(faces.at<float>(i, 2*j+5))*zoom;
            cv::circle(output_image, cv::Point(landmark[j][0], landmark[j][1]), 2, landmark_color[j], 2);
        }
    }
    return output_image;
};

// 人脸比对
void FeatureCompare(Widget &widget)
{
    YuNet com_dectect(face_detect_path, cv::Size(160, 120), 0.9, 0.3, 5000, 0, 0);
    SFace com_recogn(face_recognition_path, 0, 0, 0);
    DetectedFace dface;
    std::pair<User,bool>res;
    auto tick_meter = cv::TickMeter();
    cv::Mat feature;
    cv::Mat temp;
    Voice voice_device;
    while (true)
    {
        {
            std::unique_lock<mutex> lock(mtx);
            cond.wait(lock, [] { return ready; });
            tick_meter.start();
            resize(fuben,temp,cv::Size(160,120));
            

            dface = com_dectect.infer(temp);
            // face_proc(temp,dface);
            temp = applyWhiteBalance(temp);
            if(!dface.faces.empty())
            {
                feature = com_recogn.infer(temp,dface.faces.row(0).colRange(0,dface.faces.cols-1)).clone();
            }
            temp.release();
            res = GetUser(com_recogn,feature,widget);//获取用户的考勤结果和用户信息
            tick_meter.stop();
            if(res.second)//进行打卡操作
            {
                std::cout << "识别时间："<<tick_meter.getAvgTimeMilli()<<std::endl;
                SendUser(res.first,widget);
                voice_device.success_voice();//语音播报
            }
            else
            {
                std::cout<<"未匹配到人脸信息"<<std::endl;
            }
            // 重置ready状态
            ready = false;
            feature.release();
            tick_meter.reset();
    }
}
}
vector<double> rppgSignal;
void ProcessFrame(const cv::Mat &frame,const cv::Size &size)
{
    //std::unique_lock<mutex> lock(mtx);
    if(frame.size() != size  && !frame.empty())
    {
        cv::resize(frame, fuben, img_size); // 缩放时间1ms
    }
    else
    {
        std::cout<<"抓取的视频帧为空"<<std::endl;
    }
    // cv::normalize(fuben,fuben, 0, 1, cv::NORM_MINMAX, CV_32F);
   // fuben = applyWhiteBalance(fuben);

    fuben = adjustLighting(fuben);
    reface = model.infer(fuben);
}
void DisplayFrame(QLabel * label, const Mat& frame,float fps) {

    cv::Mat resized_image = reface.visualize(frame, fps);
    // Convert the OpenCV frame to a QImage
    QImage img(resized_image.data, resized_image.cols, resized_image.rows, resized_image.step, QImage::Format_BGR888);

    // Set the QImage as the pixmap for the QLabel
    img = img.scaled(640,480,Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    label->setPixmap(QPixmap::fromImage(img));
    
    // Update the QLabel to display the new frame
    label->update();

    // Process any pending events in the Qt event loop
    QCoreApplication::processEvents();
}

//flag帧抽一帧
void WakeFeatureCompare(int &flag)
{
    if(reface.nums == 10)
    {
        if(flag >= 10)
            flag =0;
        else
            flag +=1;
    }
    if (reface.nums == 1 && flag>=10)
    {
        ready = true;
        cond.notify_one();
    }
}

//实时人脸识别代码核心逻辑块
void FaceDetect(QLabel *label,Widget &widget)
{
    cv::VideoCapture cap = GetCapture();
    cv::Mat frame;
    cv::Mat temp;
    auto tick_meter = cv::TickMeter();

    int flag = 30;
    // 开启人脸识别线程
    thread compare_thread(FeatureCompare,ref(widget));
    while (true)
    {
        bool has_frame = cap.read(frame);
        if (!has_frame)
        {
            std::cout << "No frames grabbed! Exiting ...\n"<<std::endl;
            break;
        }

        //处理视频帧
        tick_meter.start();
        ProcessFrame(frame,img_size);
        tick_meter.stop();
        //适当条件唤醒人脸识别线程
        WakeFeatureCompare(flag);
        //显示图片
        DisplayFrame(label, frame,tick_meter.getFPS());

        tick_meter.reset();
    }

    compare_thread.join();
}

//提取数据库中的用户信息的人脸特征并且预先缓存在内存
void LoadUsers()
{
    loadFaceDb(facedb);
    YuNet s_model(face_detect_path, cv::Size(120, 160), 0.9, 0.3, 5000, 0, 0);
    SFace s_sface(face_recognition_path, 0, 0, 0);
    int n = facedb.size();
    cv::Mat img_face[n];
    cv::Mat resized_face[n];
    int i = 0;

    for(auto &face :facedb)
    { 
        img_face[i] = cv::imread(face.GetFacePath());
        std::cout <<face.GetFacePath()<<std::endl;
        if (img_face[i].empty())
        {
            std::cerr << "加载图像失败: " << face.GetFacePath() << std::endl;
            continue; // 如果加载图像失败，跳过当前用户
        }
        // img_face[i] = applyWhiteBalance(img_face[i]);
        if(img_face[i].size() != cv::Size(120,160))
        {
           cv::resize(img_face[i],resized_face[i],cv::Size(120,160));
        }
        else
        {        
            resized_face[i] = img_face[i].clone();
        }
        reface = s_model.infer(resized_face[i]);

        if(reface.nums == 1)
        {
            face.feature = s_sface.infer(resized_face[i],reface.faces.row(0).colRange(0,reface.faces.cols-1)).clone();
            std::cout<<face.GetName()<<"加载成功"<<std::endl;  
        }
        else
        {
            std::cout<<face.GetName()<<"图像加载失败，请更新图片"<<std::endl;
        }
        img_face[i].release();
        resized_face[i].release();
        i++;
    }
}
//添加人脸库中的信息
bool AddFaceDB(User &user)
{
   user.feature = GetFaceFeature(user.GetFacePath()).clone();
   if(!user.feature.empty())
   {
        facedb.push_back(user);
        return true;
   }
    return false;
}
//删除人脸库中的信息
bool DeleteFaceDB(User &d_user)
{
    for (auto it = facedb.begin(); it != facedb.end(); ++it)
    {
        if(it->GetID() == d_user.GetID())
        {
            facedb.erase(it);
            std::cout<<"删除成功"<<std::endl;
            return true;
        }
    }
    return false;
}
//修改人脸库中的信息
bool ModifyFaceDB(User &user)
{
    for (auto it = facedb.begin(); it != facedb.end(); ++it)
    {
        if(user.GetID() == it->GetID())
        {
            if(!user.GetName().empty())
            {
                it->SetName(user.GetName());
            }
            if(!user.GetDept().empty())
            {
                it->SetDept(user.GetDept());
            }
            if(!user.GetFacePath().empty())
            {
                it->SetFacePath(user.GetFacePath());
                it->feature = GetFaceFeature(user.GetFacePath());
            }      
            return true;
        }
    }
    return false;
}
//更新人脸库中考勤信息
void UpdateFaceDB()
{   
    for(auto &user: facedb)
    {
        user.absence_state = NOTSTARTED;
    }
}
//根据照片推理人脸特征
cv::Mat GetFaceFeature(std::string face_path)
{
    cv::Mat img_face;
    cv::Mat resized_face;
    cv::Mat feature;

    img_face = cv::imread(face_path);
    if (img_face.empty())
    {
        std::cerr << "图像路径为空: " << face_path << std::endl;
       cv::Mat();
    }

    if(img_face.size() != cv::Size(120,160))
    {
        cv::resize(img_face,resized_face,cv::Size(120,160));
    }
    else
    {        
        resized_face = img_face.clone();
    }
    YuNet s_model(face_detect_path, cv::Size(120, 160), 0.9, 0.3, 5000, 0, 0);
    SFace s_sface(face_recognition_path, 0, 0, 0);
    DetectedFace s_face;

    s_face = s_model.infer(resized_face);

    if(s_face.nums == 1)
    {
        resized_face = prepro_img(resized_face).clone();
        feature = s_sface.infer(resized_face,s_face.faces.row(0).colRange(0,s_face.faces.cols-1)).clone();
        resized_face.release();
    }
    else
    {
        return cv::Mat();
    }
    return feature;
}
//获取人员信息库中人员个数和已考勤的人数
std::pair<int,int>GetFaceDBNum()
{
    int total = facedb.size();
    int attendence_num = 0;
    for(auto& user:facedb)
    {
        if(user.absence_state == PRESENT)
            attendence_num +=1;
    }
  return std::make_pair(total, attendence_num);
}

//当收到考勤时间将所有人的考勤信息初始化
void UpdateUserAttendce()
{
    for(auto& user:facedb)
    {
       user.absence_state = NOTSTARTED;
    }
}

QString ConvertAttendenceType(AtendenceType type)
{
    QString str;
    if(type == PRESENT)
    {
        str = "出勤";
    }
    else if(type == NOTSTARTED)
    {
        str = "未到";
    }
    else if(type == LATE)
    {
        str = "迟到";
    }
    else if(type== ABSENT)
    {
        str = "缺勤";
    }
    else if(type== NON)
    {
        str = "无";
    }
    return str;
}

//发送考勤信息给服务器
void ShowAttendence(QTcpSocket *socket)
{
    QList<QStringList> userList;
    for(auto& user:facedb)
    {
        QString type;
        type = ConvertAttendenceType(user.absence_state);
        QStringList userString;
        userString.append(QString::number(user.GetID()));
        userString.append(QString::fromStdString(user.GetName()));
        userString.append(QString::fromStdString(user.GetDept()));
        userString.append(type);
        userList.append(userString);
    }
    QByteArray message;
    QDataStream stream(&message,QIODevice::WriteOnly);
    std::pair<int,int>res = GetFaceDBNum();
    int total = res.first;
    int attendence_num = res.second;

    stream << SHOW_ATTEND_RES;
    stream << total;
    stream << attendence_num;
    stream << userList;
    socket->write(message);
    socket->flush();
}
//发送考勤成功的用户信息给窗口显示并进行语音播报
void SendUser(User &user,Widget &widget)
{
    int id = user.GetID();
    QString img_path = QString::fromStdString(user.GetFacePath());
    QString name = QString::fromStdString(user.GetName());
    QString dept = QString::fromStdString(user.GetDept());
    AtendenceType type = user.absence_state;
    QString data = GetCurrentDate();
    QString time = GetCurrentTime();
    
    item_info item(id,img_path,name,dept,type,data,time);
    emit widget.detectedFace(item); 
}
//遍历内存中的用户信息，返回考勤成功的用户
std::pair<User,bool> GetUser(SFace &sface,cv::Mat &feature,Widget &widget)
{
    std::pair<double, bool> res;
     double max_threshold = 0;
    // double min_threshold = 2;
    User person;
    bool result;

    for(auto &user : facedb)
    {
        if(!user.feature.empty() && !feature.empty())
        {
            res = sface.match(feature,user.feature);
        }
        //有结果而且此时用户考勤状态为空
        qDebug()<<user.absence_state;
        if(res.second && res.first > max_threshold)//大于阈值的用户进入筛选
        {
            max_threshold = res.first;//更新用户当前匹配数据库的最高的距离(欧式距离)
            person = user;//记录当前匹配用户最高的数据库用户信息
            if(user.absence_state == NOTSTARTED)//如果当前用户并未考勤
            {
                std::cout<<res.first<<std::endl;
                result = 1;//用户打卡标志位
                person.absence_state = user.isAbsence(&widget);//更改用户的考勤状态
            }
            else
                result = 0;      
        }
    }
    return std::make_pair(person, result);
}

//对图像进行白平衡
cv::Mat applyWhiteBalance(cv::Mat& image){
    // 将图像转换为 LAB 颜色空间
    cv::Mat lab_image;
    cvtColor(image, lab_image, cv::COLOR_BGR2Lab);
    
    // 分离 LAB 通道
    std::vector<Mat> lab_planes(3);
    split(lab_image, lab_planes);
    
    // 计算 L 通道的均值
    Scalar l_mean = mean(lab_planes[0]);
    
    // 计算 A 和 B 通道的均值
    Scalar a_mean = mean(lab_planes[1]);
    Scalar b_mean = mean(lab_planes[2]);
    
    // 计算增益系数
    float a_gain = l_mean[0] / a_mean[0];
    float b_gain = l_mean[0] / b_mean[0];
    
    // 调整 A 和 B 通道
    lab_planes[1] = lab_planes[1] * a_gain;
    lab_planes[2] = lab_planes[2] * b_gain;
    
    // 合并 LAB 通道
    merge(lab_planes, lab_image);
    
    // 转换回 BGR 颜色空间
    Mat result;
    cvtColor(lab_image, result, COLOR_Lab2BGR);
    
    return result;
}

// 亮度调整函数
cv::Mat adjustBrightness(cv::Mat& image, double alpha, int beta)
{
    Mat new_image = Mat::zeros(image.size(), image.type());
    
    for (int y = 0; y < image.rows; y++) {
        for (int x = 0; x < image.cols; x++) {
            for (int c = 0; c < 3; c++) {
                new_image.at<Vec3b>(y,x)[c] = 
                    saturate_cast<uchar>(alpha * image.at<Vec3b>(y,x)[c] + beta);
            }
        }
    }
    
    return new_image;
}

// 光照强度计算并调整函数
cv::Mat adjustLighting(cv::Mat& image)
{
    Scalar mean_val = mean(image);
    double alpha;
    int beta;
    
    if (mean_val[0] > 150) { // 光照过强
        alpha = 0.5; // 降低亮度
        beta = -30;
    } else if (mean_val[0] < 100) { // 光照过弱
        alpha = 1.5; // 增强亮度
        beta = 30;
    } else {
        alpha = 1.0; // 保持原亮度
        beta = 0;
    }
    return adjustBrightness(image, alpha, beta);
}
//对需要检测的图像进行预处理
cv::Mat prepro_img(cv::Mat& image)
{
    cv::Mat temp;
    cv::Mat res;
    temp = adjustLighting(image);//先进行亮度调整
    res = applyWhiteBalance(temp);//在进行白平衡处理

    return res;
}
// 将RGB图像转换为HSV空间
cv::Mat convertToHSV(cv::Mat& image)
{
    Mat hsvImage;
    cvtColor(image, hsvImage, COLOR_BGR2HSV);
    return hsvImage;
}
// 判断V分量是否过曝光
bool isOverexposedRegion(double meanV)
{
    // 根据实际情况设定过曝光区域的判定条件
    return (meanV < 55 || meanV > 100);
}

/*
    人脸识别图像预处理
*/
//对人脸图像区域进行过曝光判断和处理
void face_proc(cv::Mat& image,DetectedFace& dface)
{
    cv::Rect faceRegion = cv::Rect(dface.x2,dface.y2,dface.w,dface.h);
    //裁剪人脸图像区域
    cv::Mat face = image(cv::Rect(dface.x2,dface.y2,dface.w,dface.h));
    // 转换为HSV空间
    cv::Mat hsvFace = convertToHSV(face);

    int blockSize = 8; // 定义窗口大小为8x8
    int numOverexposedRegions = 0;
    int totalRegions = 0;

    // 遍历整个V分量图像，计算每个窗口的均值并判断过曝光
    for (int y = 0; y < hsvFace.rows; y += blockSize) {
        for (int x = 0; x < hsvFace.cols; x += blockSize) {
            // 确定当前窗口的范围
            Rect roi(x, y, blockSize, blockSize);
            roi &= Rect(0, 0, hsvFace.cols, hsvFace.rows); // 确保ROI在图像范围内

            // 计算当前窗口内V分量的均值
            Scalar meanV = mean(hsvFace(roi));

            // 判断该区域是否过曝光
            if (isOverexposedRegion(meanV[2])) {
                numOverexposedRegions++;
            }
            totalRegions++;
        }
    }

    // 计算过曝光指标
    double overexposureIndex = static_cast<double>(numOverexposedRegions) / totalRegions;

    // 根据指标判断是否进行过曝光处理
    if (overexposureIndex > OVEREXPOSURE_THRESHOLD) {
        // 进行过曝光处理，例如降低图像亮度等
        dynamicRangeCompression(face, 0.8);  // 0.5是伽马值，可以根据实际情况调整
        face.copyTo(image(faceRegion));  // 将处理后的人脸区域复制回原图像
        cout << "图像进行过曝光处理.." << endl;
    } else {
        // 图像正常，可以进行其他处理
        // cout << "Image is not overexposed. Normal processing..." << endl;
    }

}
// 动态范围压缩函数
void dynamicRangeCompression(Mat& image, double gamma)
{
    Mat lookUpTable(1, 256, CV_8U);
    uchar* p = lookUpTable.ptr();
    double inverseGamma = 1.0 / gamma;

    for (int i = 0; i < 256; ++i) {
        p[i] = saturate_cast<uchar>(pow(i / 255.0, inverseGamma) * 255.0);
    }
    LUT(image, lookUpTable, image);
}
// 提取rPPG信号
std::vector<double> extractRppgSignal(cv:: Mat& frame, cv:: Rect& faceRegion)
{
    std::vector<double> signal;

    // 提取人脸区域
    cv::Mat face = frame(faceRegion);

    // 转换到YCrCb颜色空间
    Mat ycrcb;
    cvtColor(face, ycrcb, COLOR_BGR2YCrCb);

    // 提取Cr通道
    vector<Mat> channels;
    split(ycrcb, channels);
    Mat cr = channels[1];

    // 计算每帧的平均Cr值作为rPPG信号
    Scalar meanCr = mean(cr);
    signal.push_back(meanCr[0]);

    return signal;
}
// 活体检测
bool isLiveFace(std:: vector<double>& rppgSignal)
{
    if (rppgSignal.size() < 2) {
        return false;
    }
    double mean = accumulate(rppgSignal.begin(), rppgSignal.end(), 0.0) / rppgSignal.size();
    double variance = 0.0;
    for (double value : rppgSignal) {
        variance += (value - mean) * (value - mean);
    }
    variance /= rppgSignal.size();

    // 如果方差超过某个阈值，认为是活体
    return variance > 0.25;
}









