// ============================================================
// https_api.h — HTTP 服务器核心头文件
// 功能：定义 HTTP 请求/响应结构体、HTTP服务器类、路由注册接口
// ============================================================

#ifndef HTTPS_API_H
#define HTTPS_API_H

// ===== 跨平台 Socket 头文件 =====
// Windows 使用 Winsock2 库，Linux 使用 POSIX Socket API
// 通过条件编译（#ifdef _WIN32）实现跨平台兼容
#ifdef _WIN32
    #include <winsock2.h>   // Winsock2 核心头文件
    #include <ws2tcpip.h>   // Winsock2 TCP/IP 扩展
    #include <windows.h>    // Windows API
    #include <process.h>    // _getpid
    #pragma comment(lib, "ws2_32.lib")  // 链接 Winsock 库
    // 统一 API 名称：Windows 下 close → closesocket
    #define close closesocket
    // 统一读写接口：封装 recv → read, send → write
    inline int read(SOCKET fd, char* buf, int count) {
        return recv(fd, buf, count, 0);
    }
    inline int write(SOCKET fd, const char* buf, int count) {
        return send(fd, buf, count, 0);
    }
#else
    // Linux/POSIX 标准头文件
    #include <sys/socket.h>     // socket, bind, listen, accept 等
    #include <netinet/in.h>     // sockaddr_in 结构体
    #include <unistd.h>         // close, read, write
    #include <arpa/inet.h>      // inet_ntoa 等
    using SOCKET = int;         // 统一类型名
    // 统一 INVALID_SOCKET 定义（Linux 无此常量）
    #ifndef INVALID_SOCKET
    #define INVALID_SOCKET (-1)
    #endif
#endif

#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <functional>
#include <unordered_map>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <algorithm>
#include <regex>
#include <filesystem>
#include <iostream>
#include <exception>

#include "Tools.h"

namespace fs = std::filesystem;

namespace Http {
    struct HttpRequest {
        std::string method;
        std::string path;
        std::string version;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
        std::string client_ip;
    };
    
    struct HttpResponse {
        int status_code;
        std::string status_text;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
    };

    class http_server {
        private:
        int m_port;
        std::string m_static_dir;
        int m_max_connections;
        SOCKET m_server_socket;
        bool m_running;
        
        std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest&)>> m_get_routes;
        std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest&)>> m_post_routes;
        std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest&)>> m_put_routes;
        std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest&)>> m_delete_routes;

        std::string Get_Static_Dir();
        HttpRequest Request_Parse(const std::string& raw, const std::string& client_ip = "");
        std::string Response_Build(const HttpResponse& res);
        HttpResponse Default_Request_Handle(const HttpRequest& req);
        void Run_Accept_Loop();
        static void Handle_Client(SOCKET clientSocket, http_server* server);

        std::string Get_Json_Value(const std::string& json, const std::string& key);
        std::string Build_Json_Response(int code, const std::string& message, const std::string& data);

        public:
        http_server(int port = 8080, int maxConnections = 10);
        ~http_server();
        
        void Set_Static_Dir(const std::string& dir);
        
        void Get(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler);
        void Post(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler);
        void Put(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler);
        void Delete(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler);
        
        bool Initialize();
        void Start(bool async = false);
        void Stop();
        
        bool Is_Running() const { return m_running; }
        int Get_Port() const { return m_port; }

        // 将 FrameWork.cpp 中的路由注册逻辑集中到这里
        void RegisterBusinessRoutes();
    };
}

#endif // HTTPS_API_H