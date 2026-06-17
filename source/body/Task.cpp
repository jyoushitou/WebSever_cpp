//Task.h工具实现文件

#include "Task.h"
#include <random>       // 随机数
#include <sstream>      // 字符串流
#include <iomanip>      // 输入输出格式化
#include <ctime>        // 时间日期

namespace TaskSystem{
    // 全局变量定义（唯一实例）
    std::map<int, std::unique_ptr<UserThreadInfo>> g_userThreads;       // 用户 ID -> 用户线程信息 映射表
    std::mutex g_userThreadsMutex;                                      // 保护 g_userThreads 的互斥锁
    std::unordered_map<std::string, TaskResult> g_taskResults;         // 任务 ID -> 任务结果 哈希表
    std::mutex g_taskResultsMutex;                                      // 保护 g_taskResults 的互斥锁

    // 外部可使用函数
    // 生成唯一任务ID（时间戳十六进制 + 16位随机十六进制数）
    std::string Generate_Task_Id() {
        std::random_device rd;                          // 硬件随机数种子
        std::mt19937 gen(rd());                         // 梅森旋转算法随机数引擎
        std::uniform_int_distribution<> dis(0, 15);     // 0~15 均匀分布
        std::stringstream ss;
        std::time_t now = std::time(nullptr);           // 取当前时间戳
        ss << std::hex << now;                          // 时间戳转十六进制写入流
        for (int i = 0; i < 16; i++) {                  // 追加16个随机十六进制位
            ss << std::hex << dis(gen);
        }
        return ss.str();                                // 返回组合后的字符串
    }

    // 向指定用户的线程投递一个任务
    std::string Post_User_Task(int userId, UserTaskType type,
                             const std::string& input,
                             const std::string& source) {
        std::lock_guard<std::mutex> lock(g_userThreadsMutex);   // 锁定用户线程表
        auto it = g_userThreads.find(userId);                   // 查找用户线程是否存在
        if (it == g_userThreads.end()) return "";               // 不存在则返回空

        std::string taskId = Generate_Task_Id();                  // 生成唯一任务ID

        UserTask task;                                          // 构造任务对象
        task.taskId = taskId;
        task.type = type;
        task.input = input;
        task.source = source;
        task.createTime = std::time(nullptr);
        task.status = TaskStatus::PENDING;                      // 初始状态：待处理

        {   // 将任务入队到该用户线程的任务队列
            std::lock_guard<std::mutex> qLock(it->second->taskMutex);
            it->second->taskQueue.push_back(std::move(task));
        }

        // 在全局任务结果表中预占一条记录，状态为 PENDING，供前端轮询
        {
            std::lock_guard<std::mutex> rLock(g_taskResultsMutex);
            g_taskResults[taskId] = {taskId, userId, TaskStatus::PENDING,
                                     input, "", "", std::time(nullptr), 0};
        }

        it->second->taskCV.notify_one();    // 唤醒该用户的线程去取任务
        return taskId;                       // 返回 taskId 给调用者
    }

    // 更新任务执行结果（由用户线程在处理完任务后调用）
    void Update_Task_Result(const std::string& taskId, int userId,
                          TaskStatus status,
                          const std::string& output,
                          const std::string& errorMessage) {
        std::lock_guard<std::mutex> lock(g_taskResultsMutex);   // 保护结果表
        auto it = g_taskResults.find(taskId);
        if (it == g_taskResults.end()) return;                  // 任务记录不存在则忽略
        it->second.status = status;                             // 更新状态
        it->second.output = output;                             // 更新输出内容
        it->second.errorMessage = errorMessage;                 // 更新错误信息
        it->second.completeTime = std::time(nullptr);           // 记录完成时间
    }

    // 按 taskId 查询任务结果
    TaskResult* Get_Task_Result(const std::string& taskId) {
        std::lock_guard<std::mutex> lock(g_taskResultsMutex);
        auto it = g_taskResults.find(taskId);
        if (it == g_taskResults.end()) return nullptr;          // 未找到返回空指针
        return &it->second;                                     // 返回结果的指针
    }
}
