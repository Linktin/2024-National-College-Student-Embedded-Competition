#pragma once
#ifndef __SFACE_H__
#define __SFACE_H__
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <string>

class SFace
{
private:
    int backend_id_;
    int target_id_;
    int _disType; // 距离类型，0 表示余弦相似度，1 表示 Norm-L2 距离
    std::string model_path;
    cv::Ptr<cv::FaceRecognizerSF> model;
    double _threshold_cosine; // 余弦相似度匹配阈值
    double _threshold_norml2; // Norm-L2 距离匹配阈值

public:
    SFace(const std::string &model_path, int target_id = 0, int backend_id = 0, int distype = 0):
        model_path(model_path),
        target_id_(target_id),
        backend_id_(backend_id),
        _disType(distype)
    {
        assert(_disType == 0 || _disType == 1); // 断言距离类型为 0 或 1
        _threshold_cosine = 0.363; // 设置余弦相似度匹配阈值
        _threshold_norml2 = 1.025; // 设置 Norm-L2 距离匹配阈值
        model = cv::FaceRecognizerSF::create(model_path, "", backend_id, target_id);
    }

    // 对图像进行预处理操作
    cv::Mat _preprocess(const cv::Mat& image, const cv::Mat& bbox);

    // 提取图片的人脸特征
    cv::Mat infer(const cv::Mat& image, const cv::Mat& face);

    // 进行人脸匹配的成员函数，返回匹配得分和匹配结果
    std::pair<double, bool> match(const cv::Mat& feature1, const cv::Mat& feature2);
};

#endif 