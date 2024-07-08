
# 导入需要的模块和函数
from torch.nn import Linear, Conv2d, BatchNorm1d, BatchNorm2d, PReLU, ReLU, Sigmoid, Dropout2d, Dropout, AvgPool2d, MaxPool2d, AdaptiveAvgPool2d, Sequential, Module, Parameter
import torch.nn.functional as F
import torch
from collections import OrderedDict
import math

# 定义展平层模块，用于将多维张量展平为二维张量
class Flatten(Module):
    def forward(self, input):
        return input.view(input.size(0), -1)  # 将输入的维度展平为 (batch_size, -1)

# 定义卷积块模块，包括卷积层、批归一化层和PReLU激活函数
class Conv_block(Module): # 验证：与 ./fmobilefacenet 中的 `Conv` 相同
    def __init__(self, in_c, out_c, kernel=(1, 1), stride=(1, 1), padding=(0, 0), groups=1):
        super(Conv_block, self).__init__()
        # 创建二维卷积层
        self.conv2d = Conv2d(in_c, out_channels=out_c, kernel_size=kernel, groups=groups, stride=stride, padding=padding, bias=False) # 验证：与 mx.sym.Convolution(data=data, num_filter=num_filter, kernel=kernel, num_group=num_group, stride=stride, pad=pad, no_bias=True) 相同
        # 创建二维批归一化层
        self.batchnorm = BatchNorm2d(out_c, eps=0.001) # 验证：与 mx.sym.BatchNorm(data=conv, fix_gamma=False, momentum=0.9) 相同
        # 创建PReLU激活函数
        self.relu = PReLU(out_c)

    def forward(self, x):
        x = self.conv2d(x)  # 通过卷积层
        x = self.batchnorm(x)  # 通过批归一化层
        x = self.relu(x)  # 通过PReLU激活函数
        return x  # 返回输出

# 定义线性块模块，包括卷积层和批归一化层
class Linear_block(Module): # 验证：与 ./fmobilefacenet 中的 `Linear` 相同
    def __init__(self, in_c, out_c, kernel=(1, 1), stride=(1, 1), padding=(0, 0), groups=1):
        super(Linear_block, self).__init__()
        # 创建二维卷积层
        self.conv2d = Conv2d(in_c, out_channels=out_c, kernel_size=kernel, groups=groups, stride=stride, padding=padding, bias=False)
        # 创建二维批归一化层
        self.batchnorm = BatchNorm2d(out_c, eps=0.001)

    def forward(self, x):
        x = self.conv2d(x)  # 通过卷积层
        x = self.batchnorm(x)  # 通过批归一化层
        return x  # 返回输出

# 定义深度可分离卷积块模块，包括三个卷积块
class Depth_Wise(Module): # 验证：如果 residual 为 False：与 ./fmobilefacenet 中的 `DResidual` 相同；否则：与 `Residual` 相同
     def __init__(self, in_c, out_c, residual = False, kernel=(3, 3), stride=(2, 2), padding=(1, 1), groups=1):
        super(Depth_Wise, self).__init__()
        # 创建第一个卷积块（分离卷积）
        self.conv_sep = Conv_block(in_c, out_c=groups, kernel=(1, 1), padding=(0, 0), stride=(1, 1))
        # 创建第二个卷积块（深度卷积）
        self.conv_dw = Conv_block(groups, groups, groups=groups, kernel=kernel, padding=padding, stride=stride)
        # 创建第三个卷积块（线性卷积）
        self.conv_proj = Linear_block(groups, out_c, kernel=(1, 1), padding=(0, 0), stride=(1, 1))
        self.residual = residual  # 是否使用残差连接

     def forward(self, x):
        if self.residual:
            short_cut = x  # 保存输入以用于残差连接
        x = self.conv_sep(x)  # 通过第一个卷积块
        x = self.conv_dw(x)  # 通过第二个卷积块
        x = self.conv_proj(x)  # 通过第三个卷积块
        if self.residual:
            output = short_cut + x  # 将输入与输出相加
        else:
            output = x  # 输出
        return output  # 返回输出

# 定义残差块模块，包含多个深度可分离卷积块
class Residual(Module): # 验证：与 ./fmobilefacenet 中的 `Residual` 相同
    def __init__(self, c, num_block, groups, kernel=(3, 3), stride=(1, 1), padding=(1, 1)):
        super(Residual, self).__init__()
        modules = OrderedDict()  # 使用有序字典存储模块
        for i in range(num_block):
            # 创建多个深度可分离卷积块，并加入到有序字典中
            modules['block%d'%i] = Depth_Wise(c, c, residual=True, kernel=kernel, padding=padding, stride=stride, groups=groups)
        self.model = Sequential(modules)  # 将这些模块按顺序构建为一个顺序容器

    def forward(self, x):
        return self.model(x)  # 通过顺序容器中的所有模块

# 定义MobileFaceNet模型
class MobileFaceNet(Module):
    def __init__(self, embedding_size):
        super(MobileFaceNet, self).__init__()
        # 定义第一个卷积块
        self.conv_1 = Conv_block(3, 64, kernel=(3, 3), stride=(2, 2), padding=(1, 1))
        # 定义第二个卷积块（深度卷积）
        self.conv_2_dw = Conv_block(64, 64, kernel=(3, 3), stride=(1, 1), padding=(1, 1), groups=64)
        # 定义第一个深度可分离卷积块
        self.dconv_23 = Depth_Wise(64, 64, kernel=(3, 3), stride=(2, 2), padding=(1, 1), groups=128)
        # 定义第一个残差块
        self.res_3 = Residual(64, num_block=4, groups=128, kernel=(3, 3), stride=(1, 1), padding=(1, 1))
        # 定义第二个深度可分离卷积块
        self.dconv_34 = Depth_Wise(64, 128, kernel=(3, 3), stride=(2, 2), padding=(1, 1), groups=256)
        # 定义第二个残差块
        self.res_4 = Residual(128, num_block=6, groups=256, kernel=(3, 3), stride=(1, 1), padding=(1, 1))
        # 定义第三个深度可分离卷积块
        self.dconv_45 = Depth_Wise(128, 128, kernel=(3, 3), stride=(2, 2), padding=(1, 1), groups=512)
        # 定义第三个残差块
        self.res_5 = Residual(128, num_block=2, groups=256, kernel=(3, 3), stride=(1, 1), padding=(1, 1))
        # 定义第四个卷积块
        self.conv_6sep = Conv_block(128, 512, kernel=(1, 1), stride=(1, 1), padding=(0, 0))
        # 定义第最后一个线性卷积块
        self.conv_6dw7_7 = Linear_block(512, 512, groups=512, kernel=(7,7), stride=(1, 1), padding=(0, 0))
        self.conv_6_flatten = Flatten()  # 定义展平层
        self.pre_fc1 = Linear(512, embedding_size)  # 定义全连接层
        self.fc1 = BatchNorm1d(embedding_size, eps=2e-5)  # 定义批归一化层

    def forward(self, x):
        x = x - 127.5  # 图像归一化，减去均值
        x = x * 0.078125  # 图像归一化，乘以系数
        out = self.conv_1(x)  # 通过第一个卷积块
        out = self.conv_2_dw(out)  # 通过第二个卷积块（深度卷积）
        out = self.dconv_23(out)  # 通过第一个深度可分离卷积块
        out = self.res_3(out)  # 通过第一个残差块
        out = self.dconv_34(out)  # 通过第二个深度可分离卷积块
        out = self.res_4(out)  # 通过第二个残差块
        out = self.dconv_45(out)  # 通过第三个深度可分离卷积块
        out = self.res_5(out)  # 通过第三个残差块
        out = self.conv_6sep(out)  # 通过第四个卷积块
        out = self.conv_6dw7_7(out)  # 通过第五个线性卷积块
        out = self.conv_6_flatten(out)  # 展平成二维张量
        out = self.pre_fc1(out)  # 通过全连接层
        out = self.fc1(out)  # 通过批归一化层
        return out  # 返回输出
