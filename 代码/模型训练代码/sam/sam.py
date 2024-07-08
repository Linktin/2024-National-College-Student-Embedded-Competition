import torch

class SAM(torch.optim.Optimizer):  # SAM类继承自torch.optim.Optimizer
    def __init__(self, params, base_optimizer, rho=0.05, **kwargs):  # 初始化方法，接受参数params、基础优化器base_optimizer、rho和其他可选参数
        assert rho >= 0.0, f"Invalid rho, should be non-negative: {rho}"  # 确保rho非负
        defaults = dict(rho=rho, **kwargs)  # 将rho和其他可选参数存储在defaults字典中
        super(SAM, self).__init__(params, defaults)  # 调用父类的初始化方法
        self.base_optimizer = base_optimizer(self.param_groups, **kwargs)  # 实例化基础优化器
        self.param_groups = self.base_optimizer.param_groups  # 获取基础优化器的参数组

    @torch.no_grad()  # 不计算梯度
    def first_step(self, zero_grad=False):  # 第一阶段方法，接受参数zero_grad决定是否清零梯度
        grad_norm = self._grad_norm()  # 计算梯度范数
        for group in self.param_groups:  # 遍历每个参数组
            scale = group["rho"] / (grad_norm + 1e-12)  # 按比例缩放梯度
            for p in group["params"]:  # 遍历每个参数
                if p.grad is None:  # 如果梯度为None，跳过
                    continue
                e_w = p.grad * scale.to(p)  # 计算e_w
                p.add_(e_w)  # 将e_w加到参数上，朝局部最大值方向前进
                self.state[p]["e_w"] = e_w  # 将e_w保存到状态字典中
        if zero_grad:  # 如果zero_grad为True
            self.zero_grad()  # 清零梯度

    @torch.no_grad()  # 不计算梯度
    def second_step(self, zero_grad=False):  # 第二阶段方法，接受参数zero_grad决定是否清零梯度
        for group in self.param_groups:  # 遍历每个参数组
            for p in group["params"]:  # 遍历每个参数
                if p.grad is None:  # 如果梯度为None，跳过
                    continue
                p.sub_(self.state[p]["e_w"])  # 从参数中减去在第一步中加上的e_w，回到初始位置
        self.base_optimizer.step()  # 执行基础优化器的更新
        if zero_grad:  # 如果zero_grad为True
            self.zero_grad()  # 清零梯度

    def _grad_norm(self):  # 计算梯度范数的方法
        shared_device = self.param_groups[0]["params"][0].device  # 获取设备信息，确保所有操作在同一设备上进行，适用于模型并行化
        norm = torch.norm(  # 计算所有参数梯度的二范数
            torch.stack([
                p.grad.norm(p=2).to(shared_device)  # 计算每个参数梯度的二范数，并将其放到共享设备上
                for group in self.param_groups for p in group["params"]
                if p.grad is not None  # 仅计算非空梯度的参数
            ]),
            p=2
        )
        return norm  # 返回梯度范数
