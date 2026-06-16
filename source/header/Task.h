#include <iostream>
#include <string>
#include <random>
#include <mutex>
#include <iomanip>
#include <map>
#include <unordered_map>
#include <atomic>

/// @brief 用户任务类型枚举
enum class UserTaskType {
    PING,              // 心跳检测任务
    PROCESS_DATA,      // 数据处理任务
    SEND_NOTIFICATION, // 发送通知任务
    SYNC_DATABASE,     // 数据库同步任务
    CUSTOM_EVENT,      // 自定义事件任务
    SHUTDOWN           // 关闭/停止任务
};

/// @brief 任务状态枚举
enum class TaskStatus {
    PENDING,      // 任务等待处理
    PROCESSING,   // 任务正在处理中
    COMPLETED,    // 任务已完成
    FAILED        // 任务执行失败
};

/// @brief 任务执行结果的持久化数据结构
struct TaskResult {
    std::string taskId;          // 任务唯一 ID
    int userId;                  // 所属用户 ID
    TaskStatus status;           // 任务最终状态
    std::string input;           // 输入数据（用于回溯）
    std::string output;          // 输出结果
    std::string errorMessage;    // 错误信息
    std::time_t createTime;      // 创建时间
    std::time_t completeTime;    // 完成时间
};

/// @brief 用户任务数据结构，包含任务的完整生命周期信息
struct UserTask {
    std::string taskId;          // 任务唯一 ID
    UserTaskType type;           // 任务类型
    std::time_t createTime;      // 任务创建时间
    std::string source;          // 任务来源（如客户端 IP、模块名称等）
    std::string input;           // 任务输入数据
    TaskStatus status;           // 当前任务状态
    std::string output;          // 任务执行输出结果
    std::string errorMessage;    // 错误信息（任务失败时填充）
    std::time_t completeTime;    // 任务完成时间
};

/// @brief 每个用户对应的独立工作线程信息
struct UserThreadInfo {
    int userId;                          // 用户 ID
    int level;                           // 用户等级/优先级
    std::string name;                    // 用户名称
    std::thread worker;                  // 工作线程对象
    std::atomic<bool> running{true};     // 线程运行标志（默认 true）
    std::vector<UserTask> taskQueue;     // 该用户的任务队列
    std::mutex taskMutex;                // 任务队列互斥锁
    std::condition_variable taskCV;      // 任务条件变量（用于唤醒等待中的线程）
    std::atomic<int> tasksProcessed{0};  // 已处理任务计数
    std::time_t startTime;               // 线程启动时间
};

extern std::map<int, std::unique_ptr<UserThreadInfo>> g_userThreads; // 用户 ID -> 用户线程信息 映射表
extern std::mutex g_userThreadsMutex;
extern std::unordered_map<std::string, TaskResult> g_taskResults;   // 任务 ID -> 任务结果 哈希表
extern std::mutex g_taskResultsMutex;

/// @brief 生成一个唯一的任务 ID（基于时间戳+随机数）
/// @return 任务 ID 字符串
std::string GenerateTaskId();

/// @brief 向指定用户投递一个新任务
/// @param userId  目标用户 ID
/// @param type    任务类型
/// @param input   任务输入数据
/// @param source  任务来源描述（可选，默认为空字符串）
/// @return 生成的任务 ID
std::string PostUserTask(int userId, UserTaskType type,
    const std::string& input, const std::string& source = "");

/// @brief 更新指定任务的执行结果
/// @param taskId       任务 ID
/// @param userId       所属用户 ID
/// @param status       任务状态
/// @param output       执行输出
/// @param errorMessage 错误信息（可选，默认为空字符串）
void UpdateTaskResult(const std::string& taskId, int userId,
    TaskStatus status, const std::string& output,
    const std::string& errorMessage = "");

/// @brief 根据任务 ID 获取任务结果指针
/// @param taskId 任务 ID
/// @return 指向 TaskResult 的指针，未找到时返回 nullptr
TaskResult* GetTaskResult(const std::string& taskId);