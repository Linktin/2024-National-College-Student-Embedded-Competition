#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

class PWM {
public:
    PWM(int chip, int channel) : chip(chip), channel(channel) {
        // 构建路径
        basePath = "/sys/class/pwm/pwmchip" + to_string(chip) + "/pwm" + to_string(channel);
        exportPath = "/sys/class/pwm/pwmchip" + to_string(chip) + "/export";
        unexportPath = "/sys/class/pwm/pwmchip" + to_string(chip) + "/unexport";

        // 尝试导出 PWM 通道
        if (!exportPWM()) {
            cerr << "Error: Unable to export PWM channel. Retrying after resetting PWM module." << endl;
            resetPWMModule();
            if (!exportPWM()) {
                cerr << "Error: Failed to export PWM channel after resetting PWM module." << endl;
            }
        }
    }

    ~PWM() {
        // 取消导出 PWM 通道
        unexportPWM();
    }

    void setPeriod(int period) {
        writeToFile(basePath + "/period", period);
    }

    void setDutyCycle(int dutyCycle) {
        writeToFile(basePath + "/duty_cycle", dutyCycle);
    }

    void enable() {
        writeToFile(basePath + "/enable", 1);
    }

    void disable() {
        writeToFile(basePath + "/enable", 0);
    }

private:
    int chip;
    int channel;
    string basePath;
    string exportPath;
    string unexportPath;

    bool exportPWM() {
        return writeToFile(exportPath, channel);
    }

    void unexportPWM() {
        writeToFile(unexportPath, channel);
    }

    bool writeToFile(const string& path, int value) {
        ofstream file(path);
        if (file.is_open()) {
            file << value;
            file.close();
            return true;
        } else {
            cerr << "Error: Unable to open " << path << endl;
            return false;
        }
    }

    void resetPWMModule() {
        // 卸载并重新加载 PWM 模块
        system("sudo rmmod pwm-fsl-ftm");
        usleep(100000); // 等待 100ms
        system("sudo modprobe pwm-fsl-ftm");
    }
};

int main() {
    PWM pwm(1, 2); // 使用 pwmchip1 的 pwm2 通道

    pwm.setPeriod(20000000); // 20ms 周期
    pwm.setDutyCycle(10000000); // 50% 占空比
    pwm.enable();

    sleep(10); // 运行 10 秒钟

    pwm.disable();

    return 0;
}

