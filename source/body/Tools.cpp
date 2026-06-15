// ============================================================
// Tools.cpp — 通用工具函数实现
// 功能：日志输出、文件读取、MIME 类型识别
// ============================================================

#include "../header/Tools.h"

namespace Tools{
    // ===== 日志输出实现 =====
    // 不同前缀便于区分日志来源，错误日志使用 cerr（无缓冲输出）
    void Out_System(std::string str){
        std::cout << "[输出]" << str << std::endl;          // 普通信息：std::cout
    }

    void Out_System_Mysql(std::string str){
        std::cout << "[MySQL]" << str << std::endl;         // MySQL 操作日志
    }

    void Out_System_Http(std::string str){
        std::cout << "[HTTP]" << str << std::endl;          // HTTP 请求日志
    }

    void Out_System_Error(std::string str){
        std::cerr << "[error]" << str << std::endl;         // 错误信息：std::cerr（不缓冲，立即输出）
    }

    void Out_System_exit(std::string str){
        std::cout << "[exit]" << str <<std::endl;           // 退出日志
    }

    // ===== 控制台输入 =====
    int Input_System_int(){
        int a;
        std::cin >> a;
        return a;
    }

    std::string Input_System_string(){
        std::string a;
        std::cin >> a;
        return a;
    }

    // ===== 读取文件内容 =====
    // 使用 ifstream 的 rdbuf() 方法将整个文件读入 string
    // 二进制模式（ios::binary）确保不改变换行符
    std::string Read_File(const std::string& path) {
        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file) return "";                               // 打开失败返回空字符串
        std::ostringstream ss;                              // 字符串输出流
        ss << file.rdbuf();                                 // 将文件内容读入流
        return ss.str();                                    // 返回完整字符串
    }

    // ===== MIME 类型映射 =====
    // 根据文件扩展名返回对应的 MIME 类型
    // 浏览器根据 MIME 类型决定如何渲染内容
    std::string Get_MimeType(const std::string& ext) {
        if (ext == ".html" || ext == ".htm") return MIME_HTML;  // 网页
        if (ext == ".js")   return MIME_JS;                     // JavaScript
        if (ext == ".css")  return MIME_CSS;                    // 样式表
        if (ext == ".json") return MIME_JSON;                   // JSON 数据
        return "application/octet-stream";                      // 二进制流（下载处理）
    }
}