//FrameWork.h
//核心框架头文件
//功能：全局变量声明、全局函数声明（用户线程/HTTP服务器/MySQL初始化）

#ifndef FARMEWORK_H
#define FARMEWORK_H

#include <string>           // 字符串
#include <atomic>           // 原子操作

#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX        // 避免 min/max 宏冲突
    #endif
    #include <winsock2.h>   // Windows Socket API
    #include <windows.h>    // Windows API
    #include <process.h>    // 进程/线程管理（_beginthreadex 等）
#else
    #include <unistd.h>     // POSIX API（fork()、sleep() 等）
    #include <sys/wait.h>   // wait() 系列函数
    #include <signal.h>     // 信号处理
#endif

#include "MyMySQL.h"        // MySQL 数据库操作封装
#include "https_api.h"      // HTTP/HTTPS API 接口
#include "Tools.h"          // 通用工具函数（含日志、MIME、JSON解析、Token认证）
#include "Task.h"           // 任务系统：任务类型、状态、结果、线程管理

//外部全局变量（定义在 FrameWork.cpp）
extern Http::http_server* g_server;                 // 全局 HTTP 服务器实例指针
extern std::atomic<bool> g_running;                 // 程序运行状态标志（原子操作，线程安全）

//函数声明
void User_Worker_Routine(TaskSystem::UserThreadInfo* info);  // 用户工作线程的主循环函数
void Create_User_Thread(int userId, const std::string& name, int level);     // 创建指定用户的工作线程
void Http_Server_Routine(int Port);                 // HTTP 服务器线程的主循环函数
void Initiate_Http(int Port);                       // 初始化并启动 HTTP 服务器
MySQL::mysql* Initiate_MySQL();                     // 初始化并获取 MySQL 数据库连接实例

#endif //!FARMEWORK_H