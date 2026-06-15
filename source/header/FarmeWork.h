// ============================================================
// FrameWork.h — 核心框架头文件
// 功能：全局变量声明、结构体定义、函数声明
// ============================================================

#ifndef FARMEWORK_H
#define FARMEWORK_H

// ===== 标准库头文件 =====
#include <iostream>         // 标准输入输出流
#include <sstream>          // 字符串流
#include <string>           // 字符串
#include <map>              // map 容器
#include <unordered_map>    // 哈希表容器
#include <functional>       // 函数对象、std::function
#include <thread>           // 多线程
#include <chrono>           // 时间工具
#include <vector>           // 动态数组
#include <cstdlib>          // C 标准库工具（如 system、rand）
#include <atomic>           // 原子操作（线程安全）
#include <algorithm>        // 算法（如 find、sort）
#include <condition_variable> // 条件变量（线程同步）
#include <mutex>            // 互斥锁
#include <memory>           // 智能指针（unique_ptr 等）
#include <ctime>            // 时间日期处理

// ===== 平台相关头文件 =====
#ifdef _WIN32
    // Windows 平台：避免 min/max 宏冲突
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #include <winsock2.h>   // Windows Socket API
    #include <windows.h>    // Windows API
    #include <process.h>    // 进程/线程管理（_beginthreadex 等）
#else
    // Linux/Unix 平台
    #include <unistd.h>     // POSIX API（fork、sleep 等）
    #include <sys/wait.h>   // wait 系列函数
    #include <signal.h>     // 信号处理
#endif

// ===== 项目内部头文件 =====
#include "MyMySQL.h"    // MySQL 数据库操作封装
#include "https_api.h"  // HTTP/HTTPS API 接口
#include "Tools.h"      // 通用工具函数
#include "Token.h"      // Token 验证与生成
#include "Json.h"       // JSON 解析与构建

// ===== extern 全局变量声明（定义在 FrameWork.cpp） =====
extern Http::http_server* g_server; // 全局 HTTP 服务器实例指针
extern std::atomic<bool> g_running; // 程序运行状态标志（原子操作，线程安全）

// ============================================================
// 任务系统枚举
// ============================================================

/// @brief 任务状态枚举
enum class TaskStatus {
    PENDING,      // 任务等待处理
    PROCESSING,   // 任务正在处理中
    COMPLETED,    // 任务已完成
    FAILED        // 任务执行失败
};

/// @brief 用户任务类型枚举
enum class UserTaskType {
    PING,              // 心跳检测任务
    PROCESS_DATA,      // 数据处理任务
    SEND_NOTIFICATION, // 发送通知任务
    SYNC_DATABASE,     // 数据库同步任务
    CUSTOM_EVENT,      // 自定义事件任务
    SHUTDOWN           // 关闭/停止任务
};

// ============================================================
// 任务定义结构体
// ============================================================
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

// ============================================================
// 用户线程信息结构体
// ============================================================
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

// ===== 全局容器（extern 声明，定义在 FrameWork.cpp） =====
extern std::map<int, std::unique_ptr<UserThreadInfo>> g_userThreads; // 用户 ID -> 用户线程信息 映射表
extern std::mutex g_userThreadsMutex;                               // 用户线程表全局互斥锁

// ============================================================
// 任务结果结构体
// ============================================================
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

extern std::unordered_map<std::string, TaskResult> g_taskResults; // 任务 ID -> 任务结果 哈希表
extern std::mutex g_taskResultsMutex;                             // 任务结果表全局互斥锁

// ============================================================
// 函数声明
// ============================================================

/// @brief 生成唯一的任务 ID（通常基于时间戳或 UUID）
std::string GenerateTaskId();

/// @brief 向指定用户投递一个任务
/// @param userId  目标用户 ID
/// @param type    任务类型
/// @param input   任务输入数据
/// @param source  任务来源描述（可选，默认为空）
/// @return 返回生成的任务 ID
std::string PostUserTask(int userId, UserTaskType type, const std::string& input, const std::string& source = "");

/// @brief 更新指定任务的执行结果
/// @param taskId       任务 ID
/// @param userId       所属用户 ID
/// @param status       任务状态
/// @param output       执行输出
/// @param errorMessage 错误信息（可选，默认为空）
void UpdateTaskResult(const std::string& taskId, int userId, TaskStatus status, const std::string& output, const std::string& errorMessage = "");

/// @brief 根据任务 ID 获取任务结果指针
/// @param taskId 任务 ID
/// @return 指向 TaskResult 的指针，未找到时返回 nullptr
TaskResult* GetTaskResult(const std::string& taskId);

/// @brief 用户工作线程的主循环函数
/// @param info 用户线程信息指针
void UserWorkerRoutine(UserThreadInfo* info);

/// @brief 创建指定用户的工作线程
/// @param userId 用户 ID
/// @param name   用户名称
/// @param level  用户等级
void CreateUserThread(int userId, const std::string& name, int level);

/// @brief HTTP 服务器线程的主循环函数
/// @param Port 监听端口号
void HttpServerRoutine(int Port);

/// @brief 初始化并启动 HTTP 服务器
/// @param Port 监听端口号
void Initiave_Http(int Port);

/// @brief 初始化并获取 MySQL 数据库连接实例
/// @return MySQL 对象指针
MySQL::mysql* Initiave_MySQL();

#endif //!FARMEWORK_H