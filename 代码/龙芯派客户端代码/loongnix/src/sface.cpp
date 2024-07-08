#include "sface.h"
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>

cv::Mat SFace::_preprocess(const cv::Mat& image, const cv::Mat& bbox)
{
    if (bbox.empty()) {
        return image;
    } else {
        cv::Mat aligned_img;
        model->alignCrop(image, bbox, aligned_img); // 调用 FaceRecognizerSF 的 alignCrop 函数对图像进行对齐和裁剪操作
        return aligned_img;
    }
}

cv::Mat SFace::infer(const cv::Mat& image, const cv::Mat& face)
{
    if(image.empty())
    {
        std::cout << "infer图像为空" << std::endl;
    }
    if(face.empty())
    {
        std::cout << "face is empty" << std::endl;
    }
    // 预处理
    cv::Mat inputBlob = _preprocess(image, face);

    cv::Mat features;
    auto tick_meter = cv::TickMeter();
    tick_meter.start();
    model->feature(inputBlob, features); // 提取特征
    tick_meter.stop();

    return features;
}

std::pair<double, bool> SFace::match(const cv::Mat& feature1, const cv::Mat& feature2)
{
    double score;
    bool matchResult;
    if (_disType == 0) { // 若距离类型为 COSINE
        score = model->match(feature1, feature2, 0); // 计算余弦相似度得分
        std::cout << "score :" << score << std::endl;
        matchResult = (score >= _threshold_cosine ? 1 : 0); // 判断匹配结果
    } else { // 若距离类型为 NORM_L2
        score = model->match(feature1, feature2, 1); // 计算 Norm-L2 距离得分
        matchResult = (score <= _threshold_norml2 ? 1 : 0); // 判断匹配结果
        std::cout << "score :" << score << std::endl;
    }
    
    return std::make_pair(score, matchResult); // 返回得分和匹配结果的 pair 对象
};