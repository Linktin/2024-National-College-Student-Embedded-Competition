from __future__ import print_function  # 导入 print_function，确保兼容 Python 2 和 3
from __future__ import division  # 导入 division，确保兼容 Python 2 和 3
import torch  # 导入 PyTorch
import torch.nn as nn  # 导入 PyTorch 的神经网络模块
import torch.nn.functional as F  # 导入 PyTorch 的函数模块
from torch.nn import Parameter  # 导入 PyTorch 的 Parameter 类
import math  # 导入数学模块

def xavier_normal_(tensor, gain=1., mode='avg'):
    fan_in, fan_out = nn.init._calculate_fan_in_and_fan_out(tensor)
    if mode == 'avg':
        fan = fan_in + fan_out
    elif mode == 'in':
        fan = fan_in
    elif mode == 'out':
        fan = fan_out
    else:
        raise Exception('wrong mode')
    std = gain * math.sqrt(2.0 / float(fan))

    return nn.init._no_grad_normal_(tensor, 0., std)

class SFaceLoss(nn.Module):  # 定义 SFaceLoss 类，继承自 nn.Module
    #输入特征是网络的
    def __init__(self, in_features, out_features, device_id, s=64.0, k=80.0, a=0.80, b=1.23):
        super(SFaceLoss, self).__init__()  # 调用父类的构造函数
        self.in_features = in_features  # 输入特征数
        self.out_features = out_features  # 输出特征数
        self.device_id = device_id  # 设备 ID
        self.s = s  # 缩放因子
        self.k = k  # 控制因子
        self.a = a  # 角度调整因子
        self.b = b  # 角度调整因子
        self.weight = Parameter(torch.FloatTensor(out_features, in_features))  # 定义权重参数
        xavier_normal_(self.weight, gain=2, mode='out')  # 使用 Xavier 正态初始化权重

    def forward(self, input, label):  # 定义前向传播函数
        if self.device_id is None:  # 如果设备 ID 为空
            cosine = F.linear(F.normalize(input), F.normalize(self.weight))  # 计算余弦相似度
        else:
            x = input  # 输入特征
            sub_weights = torch.chunk(self.weight, len(self.device_id), dim=0)  # 将权重按设备数量拆分
            temp_x = x.cuda(self.device_id[0])  # 将输入特征转移到第一个设备
            weight = sub_weights[0].cuda(self.device_id[0])  # 将权重转移到第一个设备
            cosine = F.linear(F.normalize(temp_x), F.normalize(weight))  # 计算余弦相似度

            for i in range(1, len(self.device_id)):  # 遍历剩余设备
                temp_x = x.cuda(self.device_id[i])  # 将输入特征转移到当前设备
                weight = sub_weights[i].cuda(self.device_id[i])  # 将权重转移到当前设备
                cosine = torch.cat((cosine, F.linear(F.normalize(temp_x), F.normalize(weight)).cuda(self.device_id[0])), dim=1)  # 计算余弦相似度并拼接

        output = cosine * self.s  # 余弦相似度乘以缩放因子

        one_hot = torch.zeros(cosine.size())  # 创建全零张量
        if self.device_id is not None:
            one_hot = one_hot.cuda(self.device_id[0])  # 将全零张量转移到第一个设备
        one_hot.scatter_(1, label.view(-1, 1), 1)  # 将 one-hot 编码标签填入张量
            
        zero_hot = torch.ones(cosine.size())  # 创建全一张量
        if self.device_id is not None:
            zero_hot = zero_hot.cuda(self.device_id[0])  # 将全一张量转移到第一个设备
        zero_hot.scatter_(1, label.view(-1, 1), 0)  # 将 one-hot 编码标签位置填零

        WyiX = torch.sum(one_hot * output, 1)  # 计算每个样本对应类别的加权和
        with torch.no_grad():  # 在不计算梯度的上下文中
            theta_yi = torch.acos(WyiX / self.s)  # 计算角度
            weight_yi = 1.0 / (1.0 + torch.exp(-self.k * (theta_yi - self.a)))  # 计算权重
        intra_loss = - weight_yi * WyiX  # 计算类内损失

        Wj = zero_hot * output  # 计算其他类别的加权和
        with torch.no_grad():  # 在不计算梯度的上下文中
            theta_j = torch.acos(Wj / self.s)  # 计算角度
            weight_j = 1.0 / (1.0 + torch.exp(self.k * (theta_j - self.b)))  # 计算权重
        inter_loss = torch.sum(weight_j * Wj, 1)  # 计算类间损失

        loss = intra_loss.mean() + inter_loss.mean()  # 总损失为类内损失和类间损失的平均值
        Wyi_s = WyiX / self.s  # 归一化类内相似度
        Wj_s = Wj / self.s  # 归一化类间相似度

        return output, loss, intra_loss.mean(), inter_loss.mean(), Wyi_s.mean(), Wj_s.mean()  # 返回输出、损失、类内损失、类间损失、归一化类内相似度、归一化类间相似度
