#验证模型在验证集上的表现效果


import torch  # 导入PyTorch库
import torchvision.transforms as transforms  # 导入torchvision的transforms模块
import torch.nn.functional as F  # 导入PyTorch的神经网络功能模块

from .verification import evaluate  # 从本地模块中导入evaluate函数

from datetime import datetime  # 导入datetime模块
import matplotlib.pyplot as plt  # 导入matplotlib库用于绘图
plt.switch_backend('agg')  # 设置matplotlib为非交互式后端

import numpy as np  # 导入NumPy库
from PIL import Image  # 导入Pillow库中的Image模块
import mxnet as mx  # 导入MXNet库
import io  # 导入io模块
import os  # 导入os模块
import pickle  # 导入pickle模块
import sklearn  # 导入sklearn库
import time  # 导入time模块

def get_time():
    # 获取当前时间并格式化为字符串
    return (str(datetime.now())[:-10]).replace(' ', '-').replace(':', '-')

def load_bin(path, image_size=[112, 112]):
    # 从二进制文件中加载数据并处理图像以用于验证
    bins, issame_list = pickle.load(open(path, 'rb'), encoding='bytes')  # 加载二进制文件
    data_list = []  # 初始化数据列表
    for flip in [0, 1]:  # 循环两次，分别处理原图和翻转后的图像
        data = torch.zeros((len(issame_list) * 2, 3, image_size[0], image_size[1]))  # 创建一个空的Tensor存储图像数据
        data_list.append(data)  # 将创建的Tensor添加到数据列表中
    for i in range(len(issame_list) * 2):  # 遍历所有图像
        _bin = bins[i]  # 获取当前图像的二进制数据
        img = mx.image.imdecode(_bin)  # 解码二进制数据为图像
        if img.shape[1] != image_size[0]:  # 如果图像大小不匹配，则调整大小
            img = mx.image.resize_short(img, image_size[0])
        img = mx.nd.transpose(img, axes=(2, 0, 1))  # 转换图像的通道顺序
        for flip in [0, 1]:  # 再次循环处理原图和翻转后的图像
            if flip == 1:  # 如果是翻转后的图像
                img = mx.ndarray.flip(data=img, axis=2)  # 对图像进行水平翻转
            data_list[flip][i][:] = torch.tensor(img.asnumpy())  # 将图像数据转换为Tensor并存储
        if i % 1000 == 0:  # 每处理1000张图像打印一次进度
            print('loading bin', i)
    print(data_list[0].shape)  # 打印数据形状
    return data_list, issame_list  # 返回处理后的数据和标签列表

def get_val_pair(path, name):
    # 获取验证数据对
    ver_path = os.path.join(path, name + ".bin")  # 拼接验证数据文件路径
    assert os.path.exists(ver_path)  # 确认文件存在
    data_set, issame = load_bin(ver_path)  # 加载验证数据
    print('ver', name)  # 打印验证集名称
    return data_set, issame  # 返回数据集和标签

def get_val_data(data_path, targets):
    # 获取所有目标验证数据
    assert len(targets) > 0  # 确认目标列表不为空
    vers = []  # 初始化验证数据列表
    for t in targets:  # 遍历所有目标
        data_set, issame = get_val_pair(data_path, t)  # 获取每个目标的验证数据
        vers.append([t, data_set, issame])  # 将目标名、数据集和标签添加到列表中
    return vers  # 返回验证数据列表


def separate_mobilefacenet_bn_paras(modules):
    # 分离MobileFaceNet模型的BatchNorm层参数和非BatchNorm层参数
    if not isinstance(modules, list):  # 如果模块不是列表
        modules = [*modules.modules()]  # 将模块展开为列表
    paras_only_bn = []  # 初始化BatchNorm参数列表
    paras_wo_bn = []  # 初始化非BatchNorm参数列表
    for layer in modules:  # 遍历所有层
        if 'mobilefacenet' in str(layer.__class__) or 'container' in str(layer.__class__):  # 跳过MobileFaceNet层和容器层
            continue
        if 'batchnorm' in str(layer.__class__):  # 如果是BatchNorm层
            paras_only_bn.extend([*layer.parameters()])  # 添加BatchNorm参数
        else:
            paras_wo_bn.extend([*layer.parameters()])  # 添加非BatchNorm参数

    return paras_only_bn, paras_wo_bn  # 返回BatchNorm参数和非BatchNorm参数

def gen_plot(fpr, tpr):
    # 生成并保存ROC曲线图
    plt.figure()  # 创建新的图像
    plt.xlabel("FPR", fontsize=14)  # 设置X轴标签
    plt.ylabel("TPR", fontsize=14)  # 设置Y轴标签
    plt.title("ROC Curve", fontsize=14)  # 设置图像标题
    plot = plt.plot(fpr, tpr, linewidth=2)  # 绘制ROC曲线
    buf = io.BytesIO()  # 创建内存缓冲区
    plt.savefig(buf, format='jpeg')  # 将图像保存到缓冲区
    buf.seek(0)  # 将缓冲区指针移到开始位置
    plt.close()  # 关闭图像

    return buf  # 返回缓冲区

def perform_val(multi_gpu, device, embedding_size, batch_size, backbone, data_set, issame, nrof_folds=10):
    # 执行验证步骤
    if multi_gpu:  # 如果使用多GPU
        backbone = backbone.module  # 从DataParallel中取出模型
        backbone = backbone.to(device)  # 将模型移动到指定设备
    else:
        backbone = backbone.to(device)  # 将模型移动到指定设备
    backbone.eval()  # 切换到评估模式

    embeddings_list = []  # 初始化嵌入列表
    for carray in data_set:  # 遍历所有数据集
        idx = 0  # 初始化索引
        embeddings = np.zeros([len(carray), embedding_size])  # 创建嵌入数组
        with torch.no_grad():  # 禁用梯度计算
            while idx + batch_size <= len(carray):  # 按批处理数据
                batch = carray[idx:idx + batch_size]  # 获取当前批次数据
                embeddings[idx:idx + batch_size] = backbone(batch.to(device)).cpu()  # 获取嵌入并存储
                idx += batch_size  # 更新索引
            if idx < len(carray):  # 处理剩余数据
                batch = carray[idx:]  # 获取剩余数据
                embeddings[idx:] = backbone(batch.to(device)).cpu()  # 获取嵌入并存储
        embeddings_list.append(embeddings)  # 将嵌入添加到列表中

    _xnorm = 0.0  # 初始化XNorm
    _xnorm_cnt = 0  # 初始化XNorm计数器
    for embed in embeddings_list:  # 遍历所有嵌入
        for i in range(embed.shape[0]):  # 遍历每个嵌入
            _em = embed[i]  # 获取当前嵌入
            _norm = np.linalg.norm(_em)  # 计算嵌入的L2范数
            _xnorm += _norm  # 累加范数
            _xnorm_cnt += 1  # 增加计数器
    _xnorm /= _xnorm_cnt  # 计算平均范数

    embeddings = embeddings_list[0] + embeddings_list[1]  # 合并正反翻转的嵌入
    embeddings = sklearn.preprocessing.normalize(embeddings)  # 对嵌入进行归一化
    print(embeddings.shape)  # 打印嵌入的形状

    tpr, fpr, accuracy, best_thresholds = evaluate(embeddings, issame, nrof_folds)  # 评估嵌入
    buf = gen_plot(fpr, tpr)  # 生成ROC曲线
    roc_curve = Image.open(buf)  # 打开缓冲区中的图像
    roc_curve_tensor = transforms.ToTensor()(roc_curve)  # 将图像转换为Tensor

    return accuracy.mean(), accuracy.std(), _xnorm, best_thresholds.mean(), roc_curve_tensor  # 返回评估结果

def buffer_val(writer, db_name, acc, std, xnorm, best_threshold, roc_curve_tensor, batch):
    # 记录验证结果到日志中
    writer.add_scalar('Accuracy/{}_Accuracy'.format(db_name), acc, batch)  # 记录准确率
    writer.add_scalar('Std/{}_Std'.format(db_name), std, batch)  # 记录标准差
    writer.add_scalar('XNorm/{}_XNorm'.format(db_name), xnorm, batch)  # 记录XNorm
    writer.add_scalar('Threshold/{}_Best_Threshold'.format(db_name), best_threshold, batch)  # 记录最佳阈值
    writer.add_image('ROC/{}_ROC_Curve'.format(db_name), roc_curve_tensor, batch)  # 记录ROC曲线

class AverageMeter(object):
    """计算和存储平均值和当前值的类"""
    def __init__(self):
        self.reset()  # 初始化并重置

    def reset(self):
        # 重置所有计数器
        self.val = 0  # 当前值
        self.avg = 0  # 平均值
        self.sum = 0  # 总和
        self.count = 0  # 计数

    def update(self, val, n=1):
        # 更新计数器
        self.val = val  # 设置当前值
        self.sum += val * n  # 更新总和
        self.count += n  # 更新计数
        self.avg = self.sum / self.count  # 计算平均值

def train_accuracy(output, target, topk=(1,)):
    # 计算top-k准确率
    maxk = max(topk)  # 获取top-k的最大值
    batch_size = target.size(0)  # 获取批次大小

    _, pred = output.topk(maxk, 1, True, True)  # 获取top-k预测
    pred = pred.t()  # 转置预测结果
    correct = pred.eq(target.view(1, -1).expand_as(pred))  # 计算预测结果和目标的匹配

    res = []  # 初始化结果列表
    for k in topk:  # 遍历top-k
        correct_k = correct[:k].view(-1).float().sum(0)  # 计算top-k的正确数
        res.append(correct_k.mul_(100.0 / batch_size))  # 计算top-k准确率并存储

    return res[0]  # 返回top-1准确率