#include <iostream>
#include <gpiod.hpp>
#include <thread>
#include <QtCore/QTimer>
#include <QtCore/QMetaObject>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <string>
#include "voice.h"

using namespace std;
using namespace gpiod;

Voice::Voice(QObject* parent):
            QObject(parent),
            gpioChip("gpiochip0"),// 打开 GPIO 芯片
            gpioLine61(gpioChip.get_line(61)),// 获取 GPIO61对象
            gpioLine62(gpioChip.get_line(62))//获取GPIO62对象
{
    request.request_type = line_request::DIRECTION_OUTPUT;
    request.consumer = "voice-control";
    gpioLine61.request(request);
    gpioLine62.request(request);

    gpioLine61.set_value(1);
    gpioLine62.set_value(1);

}
Voice::~Voice()
{
    gpioLine61.release();
    gpioLine62.release();
}

void Voice::success_voice()
{
  //  timer->setInterval(200);//定时200ms
  std::cout<<"打卡"<<std::endl;
    gpioLine62.set_value(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    gpioLine61.set_value(1);
    gpioLine62.set_value(1);
   // timer->start();
}

void Voice::Face_entry_voice()
{
   // timer->setInterval(200);//定时200ms
    gpioLine61.set_value(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    gpioLine62.set_value(1);
    gpioLine61.set_value(1);
 //   timer->start();
}


// int main(int argc,char*argv[]) {
   
//     QCoreApplication app(argc, argv); // 创建 Qt 应用程序对象

//     Voice voice_device;
//    // voice_device.success_voice();
//     voice_device.Face_entry_voice(); // 设置人脸录入语音提示

//     return app.exec(); // 启动事件循环
// }
