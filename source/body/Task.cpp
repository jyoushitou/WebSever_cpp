#include "Task.h"

/**
 * GenerateTaskId - 生成一个唯一任务ID
 * 原理：当前时间戳(十六进制) + 16位随机十六进制数
 * 返回：形如 "5f8a9b3ca1b2c3d4e5f6a7b8" 的字符串
 */

std::string GenerateTaskId() {
    std::random_device rd;                          // 硬件随机数种子
    std::mt19937 gen(rd());                         // 梅森旋转算法随机数引擎
    std::uniform_int_distribution<> dis(0, 15);     // 0~15 均匀分布
    std::stringstream ss;
    std::time_t now = std::time(nullptr);           // 取当前时间戳
    ss << std::hex << now;                          // 时间戳转十六进制写入流
    for (int i = 0; i < 16; i++) ss << std::hex << dis(gen);  // 追加16个随机十六进制位
    return ss.str();                                // 返回组合后的字符串
}

/**
 * PostUserTask - 向指定用户的线程投递一个任务
 * @param userId  目标用户ID
 * @param type    任务类型 (PING / PROCESS_DATA / SEND_NOTIFICATION 等)
 * @param input   任务的 JSON 输入数据
 * @param source  任务来源说明（日志用）
 * @return 生成的 taskId（前端通过这个 taskId 查询结果）
 */
std::string PostUserTask(int userId, UserTaskType type,
                         const std::string& input,
                         const std::string& source) {
    std::lock_guard<std::mutex> lock(g_userThreadsMutex);   // 锁定用户线程表
    auto it = g_userThreads.find(userId);                   // 查找用户线程是否存在
    if (it == g_userThreads.end()) return "";               // 不存在则返回空

    std::string taskId = GenerateTaskId();                  // 生成唯一任务ID

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

/**
 * UpdateTaskResult - 更新任务执行结果（由用户线程在处理完任务后调用）
 * @param taskId       任务ID
 * @param userId       用户ID
 * @param status       完成状态 (COMPLETED / FAILED 等)
 * @param output       任务输出的 JSON 字符串
 * @param errorMessage 失败时的错误信息
 */
void UpdateTaskResult(const std::string& taskId, int userId,
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

/**
 * GetTaskResult - 按 taskId 查询任务结果
 * @param taskId 任务ID
 * @return 指向 TaskResult 对象的指针，不存在则返回 nullptr
 */
TaskResult* GetTaskResult(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(g_taskResultsMutex);
    auto it = g_taskResults.find(taskId);
    if (it == g_taskResults.end()) return nullptr;          // 未找到返回空指针
    return &it->second;                                     // 返回结果的指针
}
