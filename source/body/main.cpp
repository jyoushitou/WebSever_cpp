// ============================================================
// main.cpp — 程序入口点
// 功能：初始化 MySQL、启动 HTTP 服务器、保持主进程存活
// ============================================================

#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>

#ifdef _WIN32
// ⚠️ Win32 注意事项：winsock2.h 必须在 windows.h 之前包含！
#include <winsock2.h>
#include <windows.h>
#endif

#include "Tools.h"          // 工具库
#include "MyMySQL.h"        // MySQL 封装
#include "https_api.h"      // HTTP 服务器
#include "FarmeWork.h"      // 业务库
// ===== extern 声明 =====
// 告诉编译器这些变量/函数在别的 .cpp 文件中定义（FrameWork.cpp）
// extern 只声明不定义，链接时由链接器寻找实际定义
extern Http::http_server* g_server;         // 全局 HTTP 服务器指针
extern std::atomic<bool> g_running;         // 全局运行状态标志（原子类型，线程安全）

extern void Initiave_Http(int Port);        // 初始化并启动 HTTP 服务器
extern MySQL::mysql* Initiave_MySQL();      // 初始化 MySQL 数据库连接

// ============================================================
// main — 程序入口
//
// 整个程序的启动顺序：
// 1. 设置控制台 UTF-8 编码
// 2. 初始化 MySQL 数据库连接
// 3. 启动 HTTP 服务器（放在子进程/子线程中）
// 4. 主进程进入休眠循环等待
// 5. 服务器停止后执行收尾清理
// ============================================================
int main(){
    // ===== 1. 设置控制台编码 =====
#ifdef _WIN32
    // Windows 控制台默认代码页是 GBK，设置成 UTF-8 以正确输出中文
    SetConsoleOutputCP(CP_UTF8);
#endif
    Tools::Out_System("Web_Server启动");    // 打印启动日志

    // ===== 2. 初始化 MySQL =====
    MySQL::mysql* web_db = Initiave_MySQL();

    // ===== 3. 启动 HTTP 服务器 =====
    // 端口 60906 — 服务器会在子进程/子线程中运行，不阻塞主进程
    Initiave_Http(60906);

    // ===== 4. 主进程保持存活 =====
    // HTTP 服务器在子进程/子线程运行，主进程不能直接退出
    // 通过 while(g_running) 循环保持进程存活
    Tools::Out_System("主进程正在运行，HTTP 服务器在子进程中运行...");

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // ===== 5. 服务器停止后的收尾 =====
    if (web_db) {
        std::string name, password;
        if(web_db->User(name, password)){   // 注意：此处传空字符串调用 User() 可能有问题
            return 0;
        }
        else{
            delete web_db;
            return 1;
        }
    }

    delete web_db;  // delete nullptr 是安全的，C++ 标准保证
    return 0;
}