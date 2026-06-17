//https_api.h
//HTTP 服务器核心头文件
//功能：定义 HTTP 请求/响应结构体、HTTP服务器类、路由注册接口

#ifndef HTTPS_API_H
#define HTTPS_API_H

//跨平台 Socket 头文件
//Windows 使用 Winsock2 库，Linux 使用 POSIX Socket API
#ifdef _WIN32
    #include <winsock2.h>   // Winsock2 核心头文件
    #include <ws2tcpip.h>   // Winsock2 TCP/IP 扩展
    #include <windows.h>    // Windows API
    #include <process.h>    // _getpid
    #pragma comment(lib, "ws2_32.lib")  // 链接 Winsock 库
    #define close closesocket           // 统一 API 名称：close → closesocket
    inline int read(SOCKET fd, char* buf, int count) {      // 统一读写接口：recv → read
        return recv(fd, buf, count, 0);
    }
    inline int write(SOCKET fd, const char* buf, int count) {// 统一读写接口：send → write
        return send(fd, buf, count, 0);
    }
#else
    #include <sys/socket.h>     // socket, bind, listen, accept 等
    #include <netinet/in.h>     // sockaddr_in 结构体
    #include <unistd.h>         // close, read, write
    #include <arpa/inet.h>      // inet_ntoa 等
    using SOCKET = int;         // 统一类型名
    #ifndef INVALID_SOCKET
    #define INVALID_SOCKET (-1) // Linux 无此常量，手动定义
    #endif
#endif

#include <fstream>          // 文件输入输出流
#include <sstream>          // 字符串流
#include <string>           // 字符串
#include <thread>           // 线程
#include <functional>       // 函数对象
#include <unordered_map>    // 无序映射容器（哈希表）
#include <vector>           // 动态数组
#include <cstdlib>          // C 标准库工具
#include <cstring>          // C 字符串操作
#include <ctime>            // 时间日期
#include <algorithm>        // 算法
#include <regex>            // 正则表达式
#include <filesystem>       // 文件系统
#include <iostream>         // 输入输出流
#include <exception>        // 异常处理

#include "Tools.h"

namespace fs = std::filesystem;

namespace Http {
    // HTTP 请求结构体
    struct HttpRequest {
        std::string method;                                         // 请求方法（GET/POST/PUT/DELETE）
        std::string path;                                           // 请求路径
        std::string version;                                        // HTTP 版本
        std::unordered_map<std::string, std::string> headers;       // 请求头
        std::string body;                                           // 请求体
        std::string client_ip;                                      // 客户端 IP
    };
    
    // HTTP 响应结构体
    struct HttpResponse {
        int status_code;                                            // 状态码
        std::string status_text;                                    // 状态文本
        std::unordered_map<std::string, std::string> headers;       // 响应头
        std::string body;                                           // 响应体
    };

    // http_server 类 — 完整的 HTTP 服务器封装
    class http_server {
        private:
            //参数
            int m_port;                                             // 监听端口号
            std::string m_static_dir;                               // 静态文件目录
            int m_max_connections;                                  // 最大连接数
            SOCKET m_server_socket;                                 // 服务器 Socket
            bool m_running;                                         // 运行状态标志
            
            //路由表
            std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest&)>> m_get_routes;    // GET 路由表
            std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest&)>> m_post_routes;   // POST 路由表
            std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest&)>> m_put_routes;    // PUT 路由表
            std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest&)>> m_delete_routes; // DELETE 路由表

            //内部工具
            std::string Get_Static_Dir();                                                               // 自动查找静态文件目录
            HttpRequest Request_Parse(const std::string& raw, const std::string& client_ip = "");        // 解析 HTTP 请求报文
            std::string Response_Build(const HttpResponse& res);                                         // 构建 HTTP 响应报文
            HttpResponse Default_Request_Handle(const HttpRequest& req);                                 // 默认请求处理器（路由分发+静态文件）
            void Run_Accept_Loop();                                                                     // 接受连接主循环
            static void Handle_Client(SOCKET clientSocket, http_server* server);                         // 处理单个客户端请求（静态线程入口）

            //JSON 工具
            std::string Get_Json_Value(const std::string& json, const std::string& key);                // 从 JSON 字符串提取指定 key 的值
            std::string Build_Json_Response(int code, const std::string& message, const std::string& data); // 构建标准 JSON 响应

        public:
            //构造函数
            http_server(int port = 8080, int maxConnections = 10);  // 默认构造：端口 8080，最大连接数 10
            ~http_server();                                         // 析构时自动停止服务器

            //配置方法
            void Set_Static_Dir(const std::string& dir);            // 设置静态文件目录
            
            //路由注册
            void Get(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler);    // 注册 GET 路由
            void Post(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler);   // 注册 POST 路由
            void Put(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler);    // 注册 PUT 路由
            void Delete(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler); // 注册 DELETE 路由
            
            //服务器生命周期
            bool Initialize();                                      // 初始化服务器（Socket 创建+绑定）
            void Start(bool async = false);                         // 启动服务器（async=true 异步线程模式）
            void Stop();                                            // 停止服务器
            
            //外部可使用的工具
            bool Is_Running() const { return m_running; }           // 检查服务器是否运行中
            int Get_Port() const { return m_port; }                 // 获取监听端口号

            //业务路由注册
            void RegisterBusinessRoutes();                          // 批量注册所有业务路由
    };
}

#endif //!HTTPS_API_H