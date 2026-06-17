//main.cpp
//程序入口点
//功能：初始化 MySQL、启动 HTTP 服务器、保持主进程存活

#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>

#ifdef _WIN32
//注意：winsock2.h 必须在 windows.h 之前包含！
#include <winsock2.h>
#include <windows.h>
#endif

#include "Tools.h"          // 工具库
#include "MyMySQL.h"        // MySQL 封装
#include "https_api.h"      // HTTP 服务器
#include "FarmeWork.h"      // 业务库

//外部变量/函数声明（定义在 FrameWork.cpp）
extern Http::http_server* g_server;         // 全局 HTTP 服务器指针
extern std::atomic<bool> g_running;         // 全局运行状态标志

extern void Initiate_Http(int Port);        // 初始化并启动 HTTP 服务器
extern MySQL::mysql* Initiate_MySQL();      // 初始化 MySQL 数据库连接

//main — 程序入口
int main(){
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);            // 设置控制台 UTF-8 编码（支持中文输出）
#endif
    Tools::Out_System("Web_Server启动");     // 打印启动日志

    MySQL::mysql* web_db = Initiate_MySQL();// 初始化 MySQL

    Initiate_Http(60906);                   // 启动 HTTP 服务器（端口 60906）

    Tools::Out_System("主进程正在运行，HTTP 服务器在子进程中运行...");

    while (g_running) {                     // 主进程保持存活
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    //服务器停止后的收尾
    if (web_db) {
        std::string name, password;
        if(web_db->User(name, password)){   // 注意：传空字符串可能有问题
            return 0;
        }
        else{
            delete web_db;
            return 1;
        }
    }

    delete web_db;                          // delete nullptr 是安全的
    return 0;
}