# 导入必要的库
import torchvision.transforms as transforms  # 导入图像变换模块
import torch.utils.data as data  # 导入数据处理模块
import numpy as np  # 导入numpy用于数组处理
import os  # 导入os模块用于操作系统相关操作
# import cv2
import torch  # 导入PyTorch库

import mxnet as mx  # 导入MXNet库
from mxnet import ndarray as nd  # 导入MXNet的ndarray模块
from mxnet import io  # 导入MXNet的io模块
from mxnet import recordio  # 导入MXNet的recordio模块
import logging  # 导入日志模块
import numbers  # 导入numbers模块用于检查数字
import random  # 导入随机数模块
logger = logging.getLogger()  # 获取日志记录器
from IPython import embed  # 导入IPython的嵌入模块，用于调试

# 定义一个名为 FaceDataset 的数据集类，继承自 PyTorch 的 Dataset
class FaceDataset(data.Dataset):
    def __init__(self, path_imgrec, rand_mirror):
        # 初始化函数，接受两个参数：路径和是否随机镜像
        self.rand_mirror = rand_mirror  # 是否随机镜像的标志
        assert path_imgrec  # 确保路径不为空
        if path_imgrec:
            logging.info('loading recordio %s...', path_imgrec)  # 记录日志信息
            # 获取索引文件路径
            path_imgidx = path_imgrec[0:-4] + ".idx"
            print(path_imgrec, path_imgidx)  # 打印路径信息
            # 打开记录文件
            self.imgrec = recordio.MXIndexedRecordIO(path_imgidx, path_imgrec, 'r')
            s = self.imgrec.read_idx(0)  # 读取索引0的数据
            header, _ = recordio.unpack(s)  # 解包记录文件的头部信息
            if header.flag > 0:  # 如果标志大于0
                print('header0 label', header.label)  # 打印头部标签信息
                self.header0 = (int(header.label[0]), int(header.label[1]))  # 设置头部标签
                # 创建一个空的索引列表和ID范围字典
                self.imgidx = []  # 初始化索引列表
                self.id2range = {}  # 初始化ID范围字典
                # 创建一个身份序列
                self.seq_identity = range(int(header.label[0]), int(header.label[1]))  # 初始化身份序列
                for identity in self.seq_identity:  # 遍历身份序列
                    s = self.imgrec.read_idx(identity)  # 读取身份索引的数据
                    header, _ = recordio.unpack(s)  # 解包记录文件的头部信息
                    a, b = int(header.label[0]), int(header.label[1])  # 获取标签范围
                    count = b - a  # 计算标签范围内的数量
                    self.id2range[identity] = (a, b)  # 将范围存入字典
                    self.imgidx += range(a, b)  # 将范围内的索引添加到索引列表
                print('id2range', len(self.id2range))  # 打印ID范围字典的长度
            else:
                self.imgidx = list(self.imgrec.keys)  # 获取所有键值作为索引
            self.seq = self.imgidx  # 设置序列为索引列表

    def __getitem__(self, index):
        # 获取数据和标签
        idx = self.seq[index]  # 获取当前索引
        s = self.imgrec.read_idx(idx)  # 读取索引的数据
        header, s = recordio.unpack(s)  # 解包记录文件的头部信息和数据
        label = header.label  # 获取标签
        if not isinstance(label, numbers.Number):  # 如果标签不是数字
            label = label[0]  # 获取标签的第一个元素
        _data = mx.image.imdecode(s)  # 解码图像数据
        if self.rand_mirror:  # 如果需要随机镜像
            _rd = random.randint(0, 1)  # 随机生成0或1
            if _rd == 1:
                _data = mx.ndarray.flip(data=_data, axis=1)  # 水平翻转图像

        _data = nd.transpose(_data, axes=(2, 0, 1))  # 转置图像数据
        _data = _data.asnumpy()  # 转换为 numpy 数组
        img = torch.from_numpy(_data)  # 转换为 PyTorch 张量

        return img, label  # 返回图像和标签

    def __len__(self):
        return len(self.seq)  # 返回数据集的大小

if __name__ == '__main__':
    root = './data/train/train.rec'  # 数据集路径

    # 创建数据集对象
    dataset = FaceDataset(path_imgrec=root, rand_mirror=False)
    # 创建数据加载器
    trainloader = data.DataLoader(dataset, batch_size=32, shuffle=True, num_workers=2, drop_last=False)
    embed()  # 调试工具
    print(len(dataset))  # 打印数据集大小
    for data, label in trainloader:
        data_nd = io.DataBatch([data], [label])  # 创建数据批次
        print(data_nd.data, data_nd.label)  # 打印数据和标签
