// ============================================================
// FrameWork.h — 核心框架头文件
// 功能：全局变量声明、结构体定义、函数声明
// ============================================================

#ifndef FARMEWORK_H
#define FARMEWORK_H

// ===== 标准库头文件 =====
#include <iostream>             // 标准输入输出流
#include <sstream>              // 字符串流
#include <string>               // 字符串
#include <map>                  // map 容器
#include <unordered_map>        // 哈希表容器
#include <functional>           // 函数对象、std::function
#include <thread>               // 多线程
#include <chrono>               // 时间工具
#include <vector>               // 动态数组
#include <cstdlib>              // C 标准库工具（如 system()、rand()）

#include <algorithm>            // 算法（如 find()、sort()）
#include <condition_variable>   // 条件变量（线程同步）
#include <mutex>                // 互斥锁
#include <memory>               // 智能指针（unique_ptr 等）
#include <ctime>                // 时间日期处理

// ===== 平台相关头文件 =====
#ifdef _WIN32
    // Windows 平台：避免 min/max 宏冲突
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #include <winsock2.h>       // Windows Socket API
    #include <windows.h>        // Windows API
    #include <process.h>        // 进程/线程管理（_beginthreadex 等）
#else
    // Linux/Unix 平台
    #include <unistd.h>         // POSIX API（fork()、sleep() 等）
    #include <sys/wait.h>       // wait() 系列函数
    #include <signal.h>         // 信号处理
#endif

// ===== 项目内部头文件 =====
#include "MyMySQL.h"            // MySQL 数据库操作封装
#include "https_api.h"          // HTTP/HTTPS API 接口
#include "Tools.h"              // 通用工具函数
#include "Token.h"              // Token 验证与生成
#include "Json.h"               // JSON 解析与构建
#include "Task.h"               // 任务系统：任务类型、状态、结果、线程管理

// ===== extern 全局变量声明（定义在 FrameWork.cpp） =====
extern Http::http_server* g_server;    // 全局 HTTP 服务器实例指针
extern std::atomic<bool> g_running;    // 程序运行状态标志（原子操作，线程安全）

// ============================================================
// 函数声明
// ============================================================

/// @brief 用户工作线程的主循环函数
/// @param info 用户线程信息指针
void UserWorkerRoutine(UserThreadInfo* info);

/// @brief 创建指定用户的工作线程
/// @param userId  用户 ID
/// @param name    用户名称
/// @param level   用户等级
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