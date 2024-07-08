#ifndef __YUNET_H__
#define __YUNET_H__
#include <iostream>
#include<string>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include<opencv2/opencv.hpp>
#include <opencv2/dnn/dnn.hpp>
#include "face.h"
// using namespace cv;

class YuNet
{
public:
    YuNet(const std::string& model_path,
          const cv::Size& input_size = cv::Size(320,240),
          float conf_threshold = 0.6f,
          float nms_threshold = 0.3f,
          int top_k = 5000,
          int backend_id = 0,
          int target_id = 0)
        : model_path_(model_path), input_size_(input_size),
          conf_threshold_(conf_threshold), nms_threshold_(nms_threshold),
          top_k_(top_k), backend_id_(backend_id), target_id_(target_id)
    {
        model = cv::FaceDetectorYN::create(model_path_, "", input_size_, conf_threshold_, nms_threshold_, top_k_, backend_id_, target_id_);
    }

    /* Overwrite the input size when creating the model. Size format: [Width, Height].
    */
    void setInputSize(const cv::Size& input_size)
    {
        input_size_ = input_size;
        model->setInputSize(input_size_);
    }

    DetectedFace infer(const cv::Mat& image)
    {
        DetectedFace face;
        cv::Mat res;
        model->detect(image, res);
        //人脸矩阵信息
        face.faces = res;
        //人脸个数
        face.nums = res.rows;
        for (int i = 0; i < res.rows; ++i)
        {
            // 人脸矩形
            face.x2 = static_cast<int>(res.at<float>(i, 0))  ;
            face.y2 = static_cast<int>(res.at<float>(i, 1)) ;
            face.w = static_cast<int>(res.at<float>(i, 2))  ;
            face.h = static_cast<int>(res.at<float>(i, 3))  ;

            // 置信度
            face.conf = res.at<float>(i, 14);

            //人脸的五个点
            for (int j = 0; j < 5; ++j)
            {
                face.landmark[j][0] = static_cast<int>(res.at<float>(i, 2*j+4)), face.landmark[j][1] = static_cast<int>(res.at<float>(i, 2*j+5));
            }
        }
        return face;
    }
private:
    cv::Ptr<cv::FaceDetectorYN> model;
    std::string model_path_;
    cv::Size input_size_;
    float conf_threshold_;
    float nms_threshold_;
    int top_k_;
    int backend_id_;
    int target_id_;
};

#endif