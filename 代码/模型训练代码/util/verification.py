# 验证数据集的一些工具类

import numpy as np  # 导入NumPy库用于数值计算
from sklearn.model_selection import KFold  # 导入KFold用于交叉验证
from sklearn.decomposition import PCA  # 导入PCA用于主成分分析
import sklearn  # 导入Scikit-learn库用于机器学习任务
from scipy import interpolate  # 从SciPy导入插值函数
from scipy.spatial.distance import pdist  # 从SciPy导入成对距离函数

# 支持功能：['calculate_roc', 'calculate_accuracy', 'calculate_val', 'calculate_val_far', 'evaluate']

def calculate_roc(thresholds, embeddings1, embeddings2, actual_issame, nrof_folds=10, pca=0):
    # 计算ROC曲线
    assert (embeddings1.shape[0] == embeddings2.shape[0])  # 确认嵌入1和嵌入2的数量相等
    assert (embeddings1.shape[1] == embeddings2.shape[1])  # 确认嵌入1和嵌入2的维度相等
    nrof_pairs = min(len(actual_issame), embeddings1.shape[0])  # 获取实际相同标签的最小长度
    nrof_thresholds = len(thresholds)  # 获取阈值数量
    k_fold = KFold(n_splits=nrof_folds, shuffle=False)  # 创建KFold对象用于交叉验证

    tprs = np.zeros((nrof_folds, nrof_thresholds))  # 初始化TPR数组
    fprs = np.zeros((nrof_folds, nrof_thresholds))  # 初始化FPR数组
    accuracy = np.zeros((nrof_folds))  # 初始化准确率数组
    best_thresholds = np.zeros((nrof_folds))  # 初始化最佳阈值数组
    indices = np.arange(nrof_pairs)  # 创建索引数组

    if pca == 0:
        diff = np.subtract(embeddings1, embeddings2)  # 计算嵌入向量之间的差异
        dist = np.sum(np.square(diff), 1)  # 计算差异的平方和

    for fold_idx, (train_set, test_set) in enumerate(k_fold.split(indices)):
        # 遍历每个折叠，进行训练集和测试集的划分
        if pca > 0:
            print("doing pca on", fold_idx)
            embed1_train = embeddings1[train_set]  # 获取训练集的嵌入1
            embed2_train = embeddings2[train_set]  # 获取训练集的嵌入2
            _embed_train = np.concatenate((embed1_train, embed2_train), axis=0)  # 合并嵌入1和嵌入2
            pca_model = PCA(n_components=pca)  # 创建PCA模型
            pca_model.fit(_embed_train)  # 拟合PCA模型
            embed1 = pca_model.transform(embeddings1)  # 转换嵌入1
            embed2 = pca_model.transform(embeddings2)  # 转换嵌入2
            embed1 = sklearn.preprocessing.normalize(embed1)  # 归一化嵌入1
            embed2 = sklearn.preprocessing.normalize(embed2)  # 归一化嵌入2
            diff = np.subtract(embed1, embed2)  # 计算归一化后嵌入向量之间的差异
            dist = np.sum(np.square(diff), 1)  # 计算差异的平方和

        # 寻找当前折叠的最佳阈值
        acc_train = np.zeros((nrof_thresholds))  # 初始化训练集准确率数组
        for threshold_idx, threshold in enumerate(thresholds):
            _, _, acc_train[threshold_idx] = calculate_accuracy(threshold, dist[train_set], actual_issame[train_set])
        best_threshold_index = np.argmax(acc_train)  # 找到最佳阈值索引
        best_thresholds[fold_idx] = thresholds[best_threshold_index]  # 记录最佳阈值
        for threshold_idx, threshold in enumerate(thresholds):
            tprs[fold_idx, threshold_idx], fprs[fold_idx, threshold_idx], _ = calculate_accuracy(
                threshold, dist[test_set], actual_issame[test_set])
        _, _, accuracy[fold_idx] = calculate_accuracy(thresholds[best_threshold_index], dist[test_set], actual_issame[test_set])

    tpr = np.mean(tprs, 0)  # 计算TPR的平均值
    fpr = np.mean(fprs, 0)  # 计算FPR的平均值
    return tpr, fpr, accuracy, best_thresholds  # 返回TPR、FPR、准确率和最佳阈值

def calculate_accuracy(threshold, dist, actual_issame):
    # 计算给定阈值下的准确率
    predict_issame = np.less(dist, threshold)  # 预测相同
    tp = np.sum(np.logical_and(predict_issame, actual_issame))  # 计算真正例
    fp = np.sum(np.logical_and(predict_issame, np.logical_not(actual_issame)))  # 计算假正例
    tn = np.sum(np.logical_and(np.logical_not(predict_issame), np.logical_not(actual_issame)))  # 计算真反例
    fn = np.sum(np.logical_and(np.logical_not(predict_issame), actual_issame))  # 计算假反例

    tpr = 0 if (tp + fn == 0) else float(tp) / float(tp + fn)  # 计算TPR
    fpr = 0 if (fp + tn == 0) else float(fp) / float(fp + tn)  # 计算FPR
    acc = float(tp + tn) / dist.size  # 计算准确率
    return tpr, fpr, acc  # 返回TPR、FPR和准确率

def calculate_val(thresholds, embeddings1, embeddings2, actual_issame, far_target, nrof_folds=10):
    # 计算验证率
    assert (embeddings1.shape[0] == embeddings2.shape[0])  # 确认嵌入1和嵌入2的数量相等
    assert (embeddings1.shape[1] == embeddings2.shape[1])  # 确认嵌入1和嵌入2的维度相等
    nrof_pairs = min(len(actual_issame), embeddings1.shape[0])  # 获取实际相同标签的最小长度
    nrof_thresholds = len(thresholds)  # 获取阈值数量
    k_fold = KFold(n_splits=nrof_folds, shuffle=False)  # 创建KFold对象用于交叉验证

    val = np.zeros(nrof_folds)  # 初始化验证率数组
    far = np.zeros(nrof_folds)  # 初始化错误接受率数组

    diff = np.subtract(embeddings1, embeddings2)  # 计算嵌入向量之间的差异
    dist = np.sum(np.square(diff), 1)  # 计算差异的平方和
    indices = np.arange(nrof_pairs)  # 创建索引数组

    for fold_idx, (train_set, test_set) in enumerate(k_fold.split(indices)):
        # 遍历每个折叠，进行训练集和测试集的划分

        # 寻找使FAR等于目标FAR的阈值
        far_train = np.zeros(nrof_thresholds)  # 初始化训练集FAR数组
        for threshold_idx, threshold in enumerate(thresholds):
            _, far_train[threshold_idx] = calculate_val_far(threshold, dist[train_set], actual_issame[train_set])
        if np.max(far_train) >= far_target:
            f = interpolate.interp1d(far_train, thresholds, kind='slinear')  # 插值计算阈值
            threshold = f(far_target)  # 获取目标FAR对应的阈值
        else:
            threshold = 0.0

        val[fold_idx], far[fold_idx] = calculate_val_far(threshold, dist[test_set], actual_issame[test_set])

    val_mean = np.mean(val)  # 计算验证率的平均值
    far_mean = np.mean(far)  # 计算错误接受率的平均值
    val_std = np.std(val)  # 计算验证率的标准差
    return val_mean, val_std, far_mean  # 返回验证率平均值、标准差和错误接受率

def calculate_val_far(threshold, dist, actual_issame):
    # 计算给定阈值下的验证率和错误接受率
    predict_issame = np.less(dist, threshold)  # 预测相同
    true_accept = np.sum(np.logical_and(predict_issame, actual_issame))  # 计算真正接受
    false_accept = np.sum(np.logical_and(predict_issame, np.logical_not(actual_issame)))  # 计算假接受
    n_same = np.sum(actual_issame)  # 计算相同的数量
    n_diff = np.sum(np.logical_not(actual_issame))  # 计算不同的数量
    val = float(true_accept) / float(n_same)  # 计算验证率
    far = float(false_accept) / float(n_diff)  # 计算错误接受率
    return val, far  # 返回验证率和错误接受率

def evaluate(embeddings, actual_issame, nrof_folds=10, pca=0):
    # 评估模型
    thresholds = np.arange(0, 4, 0.01)  # 定义阈值范围
    embeddings1 = embeddings[0::2]  # 提取嵌入1
    embeddings2 = embeddings[1::2]  # 提取嵌入2
    tpr, fpr, accuracy, best_thresholds = calculate_roc(thresholds, embeddings1, embeddings2, np.asarray(actual_issame), nrof_folds=nrof_folds, pca=pca)
    # 计算ROC曲线
    return tpr, fpr, accuracy, best_thresholds  # 返回TPR、FPR、准确率和最佳阈值