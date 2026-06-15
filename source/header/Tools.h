// ============================================================
// Tools.h — 通用工具库头文件
// 功能：MIME 类型定义、日志输出、文件读取、控制台输入等
// ============================================================

#include <iostream>
#include <fstream>                      // 文件输入输出流 ifstream/ofstream
#include <sstream>                      // 字符串流 ostringstream
#include <string>                       // C++ 字符串类

// ===== HTTP MIME 类型常量 =====
// MIME (Multipurpose Internet Mail Extensions) 类型告诉浏览器返回内容的格式
#define MIME_HTML "text/html"               // HTML 网页
#define MIME_JS  "application/javascript"   // JavaScript 脚本
#define MIME_CSS "text/css"                 // CSS 样式表
#define MIME_JSON "application/json"        // JSON 数据（RESTful API 常用）

// ============================================================
// Tools 命名空间 — 通用工具函数集合
//
// 所有函数都是静态链接的命名空间函数，不依赖类实例。
// 日志输出按模块分级：System/MySQL/HTTP/Error，便于调试。
// ============================================================
namespace Tools{
    // ===== 日志输出 =====
    void Out_System(std::string);           // 普通系统日志 [输出]
    void Out_System_Mysql(std::string);     // MySQL 相关日志 [MySQL]
    void Out_System_Http(std::string);      // HTTP 相关日志 [HTTP]
    void Out_System_Error(std::string);     // 错误日志 [错误]（输出到 stderr）

    // ===== 控制台输入 =====
    int Input_System_int();                 // 从控制台读取整数
    std::string Input_System_string();      // 从控制台读取字符串

    // ===== 文件操作 =====
    std::string Read_File(const std::string& path);     // 读取文件全部内容到字符串

    // ===== MIME 类型工具 =====
    std::string Get_MimeType(const std::string& ext);   // 根据文件扩展名返回 MIME 类型
}