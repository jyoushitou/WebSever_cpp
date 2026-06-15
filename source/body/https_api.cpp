// ============================================================
// https_api.cpp — HTTP 服务器核心实现
// 功能：Socket 生命周期管理、HTTP 协议解析、路由分发、
//       静态文件服务、JSON 工具函数
// ============================================================

#include "../header/https_api.h"

namespace Http {

    // ==================== 构造/析构 ====================

    http_server::http_server(int port, int maxConnections)
        : m_port(port)
        , m_max_connections(maxConnections)
        , m_server_socket(INVALID_SOCKET)
        , m_running(false)
    {
        m_static_dir = Get_Static_Dir();  // 自动查找静态文件目录
    }

    http_server::~http_server() {
        Stop();  // 析构时自动停止服务器
    }

    void http_server::Set_Static_Dir(const std::string& dir) {
        m_static_dir = dir;
    }

    // ============================================================
    // JSON 工具函数
    // 手工解析 JSON（不依赖第三方库），仅支持简单场景
    // ============================================================

    // 从 JSON 字符串中提取指定 key 的值
    // 支持字符串、数字、布尔、对象、数组
    std::string http_server::Get_Json_Value(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\"";         // 构造 "key":
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return "";

        size_t colonPos = json.find(":", keyPos + searchKey.length());
        if (colonPos == std::string::npos) return "";

        // 跳过冒号后的空白字符
        size_t valueStart = json.find_first_not_of(" \t\n\r", colonPos + 1);
        if (valueStart == std::string::npos) return "";

        // 判断值类型并分别处理
        if (json[valueStart] == '"') {
            // 字符串值：找下个引号
            size_t valueEnd = json.find("\"", valueStart + 1);
            if (valueEnd == std::string::npos) return "";
            return json.substr(valueStart + 1, valueEnd - valueStart - 1);
        } else if (json[valueStart] == '{' || json[valueStart] == '[') {
            // 对象或数组：用栈计数花括号/方括号
            char closeChar = (json[valueStart] == '{') ? '}' : ']';
            int depth = 1;
            size_t pos = valueStart + 1;
            while (depth > 0 && pos < json.length()) {
                if (json[pos] == json[valueStart]) depth++;
                if (json[pos] == closeChar) depth--;
                pos++;
            }
            return json.substr(valueStart, pos - valueStart);
        } else {
            // 数字或布尔值：遇到逗号或括号结束
            size_t valueEnd = json.find_first_of(",}\n\r", valueStart);
            if (valueEnd == std::string::npos) return json.substr(valueStart);
            return json.substr(valueStart, valueEnd - valueStart);
        }
    }

    // 构建标准 JSON 响应
    // 格式：{"code": 200, "message": "...", "data": ...}
    std::string http_server::Build_Json_Response(int code, const std::string& message, const std::string& data) {
        std::string json = "{";
        json += "\"code\": " + std::to_string(code) + ", ";
        json += "\"message\": \"" + message + "\"";
        if (!data.empty()) {
            json += ", \"data\": " + data;
        }
        json += "}";
        return json;
    }

    // ============================================================
    // 路由注册方法
    // 将路径和处理函数（std::function）存入对应的路由表
    // ============================================================

    void http_server::Get(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler) {
        m_get_routes[path] = handler;         // 存入 GET 路由表
    }

    void http_server::Post(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler) {
        m_post_routes[path] = handler;        // 存入 POST 路由表
    }

    void http_server::Put(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler) {
        m_put_routes[path] = handler;         // 存入 PUT 路由表
    }

    void http_server::Delete(const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler) {
        m_delete_routes[path] = handler;      // 存入 DELETE 路由表
    }

        // ============================================================
    // 批量注册所有业务路由
    // 将 FrameWork.cpp::HttpServerRoutine 中的 lambda 路由集中到此
    // 需要在 HttpServerRoutine 创建 server 后调用
    // ============================================================
            void http_server::RegisterBusinessRoutes() {
        // ===== GET /api/hello =====
        Get("/api/hello", [this](const HttpRequest& req) -> HttpResponse {
            HttpResponse res;
            res.status_code = 200;
            res.status_text = "OK";
            res.headers["Content-Type"] = "application/json";
            res.headers["Access-Control-Allow-Origin"] = "*";
            res.body = "{\"message\": \"Hello from server\", \"pid\": " +
                       std::to_string(
        #ifdef _WIN32
                           _getpid()
        #else
                           getpid()
        #endif
                       ) + "}";
            return res;
        });

        // ===== GET /api/contents =====
        Get("/api/contents", [this](const HttpRequest& req) -> HttpResponse {
            HttpResponse res;
            res.status_code = 200;
            res.status_text = "OK";
            res.headers["Content-Type"] = "application/json";
            res.headers["Access-Control-Allow-Origin"] = "*";

            std::string json = "{";
            json += "\"code\": 200, ";
            json += "\"message\": \"success\", ";
            json += "\"data\": [";

            std::vector<std::string> sectionTitles = {"文章", "图片", "视频", "博客"};
            std::vector<std::vector<std::pair<std::string, std::vector<std::string>>>> sectionData;

            for (int s = 0; s < 4; s++) {
                std::vector<std::pair<std::string, std::vector<std::string>>> rows;
                for (int r = 0; r < 3; r++) {
                    std::string label = "标签" + std::to_string(r + 1);
                    std::vector<std::string> imgs;
                    int imgCount = 6 - r;
                    for (int i = 0; i < imgCount; i++) {
                        imgs.push_back("/image/" + std::to_string(s) + "_" + std::to_string(r) + "_" + std::to_string(i) + ".jpg");
                    }
                    rows.push_back({label, imgs});
                }
                sectionData.push_back(rows);
            }

            for (int s = 0; s < (int)sectionData.size(); s++) {
                if (s > 0) json += ",";
                json += "{";
                json += "\"title\":\"" + sectionTitles[s] + "\",";
                json += "\"rows\":[";
                for (int r = 0; r < (int)sectionData[s].size(); r++) {
                    if (r > 0) json += ",";
                    json += "{";
                    json += "\"label\":\"" + sectionData[s][r].first + "\",";
                    json += "\"imgs\":[";
                    for (int i = 0; i < (int)sectionData[s][r].second.size(); i++) {
                        if (i > 0) json += ",";
                        json += "\"" + sectionData[s][r].second[i] + "\"";
                    }
                    json += "]";
                    json += "}";
                }
                json += "]";
                json += "}";
            }

            json += "]}";
            res.body = json;
            return res;
        });

        // ===== GET /api/article =====
        Get("/api/article", [this](const HttpRequest& req) -> HttpResponse {
            HttpResponse res;
            res.status_code = 200;
            res.status_text = "OK";
            res.headers["Content-Type"] = "application/json";
            res.headers["Access-Control-Allow-Origin"] = "*";

            std::string json = "{";
            json += "\"code\": 200, ";
            json += "\"message\": \"success\", ";
            json += "\"data\": {";
            json += "\"title\": \"文章名\",";
            json += "\"chapters\": [";
            json += "{\"name\":\"第一章 开篇\",\"content\":\"清晨的微光透过窗棂洒落，轻轻拂过桌面，为平凡的一日拉开序幕。\"},";
            json += "{\"name\":\"第二章 内容一\",\"content\":\"行走在世间，我们会遇见形形色色的人，经历大大小小的事。\"},";
            json += "{\"name\":\"第三章 内容二\",\"content\":\"我们在成长中学会接纳，在挫折中变得坚强，在陪伴中懂得珍惜。\"},";
            json += "{\"name\":\"第四章 结尾\",\"content\":\"时光缓缓流淌，带走稚嫩，沉淀阅历，让原本懵懂的内心慢慢变得丰盈而笃定。\"}";
            json += "]";
            json += "}}";
            res.body = json;
            return res;
        });
    }

    // ============================================================
    // 获取静态文件目录
    // 自动查找前端构建输出的位置（支持多种目录结构）
    // ============================================================

    std::string http_server::Get_Static_Dir() {
        std::string exeDir;

        // 获取可执行文件所在目录
        #ifdef _WIN32
            char path[MAX_PATH];
            GetModuleFileNameA(NULL, path, MAX_PATH);  // Windows API
            exeDir = std::string(path);
            size_t pos = exeDir.find_last_of("\\/");
            if (pos != std::string::npos) {
                exeDir = exeDir.substr(0, pos);
            }
        #else
            char path[1024];
            ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);  // Linux 读取符号链接
            if (len != -1) {
                path[len] = '\0';
                exeDir = std::string(path);
                size_t pos = exeDir.find_last_of("\\/");
                if (pos != std::string::npos) {
                    exeDir = exeDir.substr(0, pos);
                }
            }
        #endif

        Tools::Out_System_Http("Executable directory: " + exeDir);

        // 尝试多个可能的静态文件位置
        std::vector<std::string> candidates = {
            exeDir + "/../../frontend",
            exeDir + "/../../../frontend",
            exeDir + "/../frontend",
            exeDir + "/frontend",
            "../frontend",
            "../../frontend",
            "../../../frontend",
            "frontend"
        };

        for (const auto& dir : candidates) {
            std::string indexPath = dir + "/index.html";
            Tools::Out_System_Http(std::string("Checking: ") + fs::absolute(indexPath).string());
            if (fs::exists(indexPath)) {
                Tools::Out_System_Http(std::string("Found static directory: ") + fs::absolute(dir).string());
                return dir;
            }
        }

        // 没找到则创建默认目录
        Tools::Out_System_Http("[Debug] Static directory not found, creating...");
        std::string fallbackDir = exeDir + "/frontend";
        fs::create_directories(fallbackDir);
        return fallbackDir;
    }
    

        // ============================================================
    // 初始化服务器
    //
    // TCP Socket 创建流程：
    // 1. socket() — 创建 Socket 文件描述符
    // 2. setsockopt() — 设置 SO_REUSEADDR 允许端口重用
    // 3. bind() — 绑定到指定 IP 和端口
    // 4. listen() — 开始监听（在 Start() 中调用）
    // ============================================================
    bool http_server::Initialize() {
#ifdef _WIN32
        // Windows 下需要先初始化 Winsock2 库
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            Tools::Out_System_Error("WSAStartup failed");
            return false;
        }
#endif

        // 1. 创建 TCP Socket
        // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = 自动选择协议
        m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_server_socket < 0) {
            Tools::Out_System_Error("Failed to create socket");
            return false;
        }

        // 2. 设置 SO_REUSEADDR 选项
        // 允许服务器关闭后立即重用同一端口，避免 TIME_WAIT 状态
        int opt = 1;
        if (setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR,
                       (const char*)&opt, sizeof(opt)) < 0) {
            Tools::Out_System_Error("Failed to set SO_REUSEADDR");
            return false;
        }

        // 3. 绑定到指定端口
        // sockaddr_in 结构：IPv4 地址 + 端口号
        struct sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;                    // IPv4
        server_addr.sin_addr.s_addr = INADDR_ANY;            // 监听所有网络接口
        server_addr.sin_port = htons(m_port);                // 端口号（网络字节序）

        if (bind(m_server_socket, (struct sockaddr*)&server_addr,
                 sizeof(server_addr)) < 0) {
            Tools::Out_System_Error("Failed to bind to port " + std::to_string(m_port));
            return false;
        }

        Tools::Out_System_Http("Server initialized on port " + std::to_string(m_port));
        return true;
    }

    // ============================================================
    // 启动服务器
    //
    // async 参数：
    //   true  — 在独立线程中运行，不阻塞调用者
    //   false — 阻塞运行（通常在子进程中使用）
    // ============================================================
    void http_server::Start(bool async) {
        if (!Initialize()) {
            Tools::Out_System_Error("Server initialization failed");
            return;
        }

        // 开始监听
        // SOMAXCONN = 系统允许的最大等待连接数
        if (listen(m_server_socket, SOMAXCONN) < 0) {
            Tools::Out_System_Error("Failed to listen on socket");
            return;
        }

        m_running = true;
        Tools::Out_System_Http("Server is listening on port " + std::to_string(m_port) +
                               " (async=" + (async ? "true" : "false") + ")");

        if (async) {
            // 异步模式：在独立线程中运行接受循环
            std::thread accept_thread(&http_server::Run_Accept_Loop, this);
            accept_thread.detach();
        } else {
            // 同步模式：直接阻塞
            Run_Accept_Loop();
        }
    }

    // ============================================================
    // 停止服务器
    // 关闭监听 Socket，停止接受新连接
    // ============================================================
    void http_server::Stop() {
        if (!m_running) return;

        m_running = false;
        Tools::Out_System_Http("Server stopping...");

        if (m_server_socket >= 0) {
#ifdef _WIN32
            closesocket(m_server_socket);
            WSACleanup();
#else
            close(m_server_socket);
#endif
            m_server_socket = INVALID_SOCKET;
        }

        Tools::Out_System_Http("Server stopped");
    }

    // ============================================================
    // HTTP 请求解析
    //
    // 解析原始 HTTP 请求报文，提取方法、路径、头部和主体
    // 支持 GET/POST/PUT/DELETE 方法
    // ============================================================
    HttpRequest http_server::Request_Parse(const std::string& raw, const std::string& client_ip) {
        HttpRequest req;
        req.client_ip = client_ip;

        std::istringstream stream(raw);
        std::string line;

        // 1. 解析请求行：METHOD /path HTTP/1.1
        if (std::getline(stream, line)) {
            // 去掉行尾的 \r
            if (!line.empty() && line.back() == '\r') line.pop_back();

            std::istringstream lineStream(line);
            lineStream >> req.method >> req.path >> req.version;
        }

        // 2. 解析请求头
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();

            // 空行 = 头部结束
            if (line.empty()) break;

            size_t colonPos = line.find(": ");
            if (colonPos != std::string::npos) {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 2);
                req.headers[key] = value;
            }
        }

        // 3. 如果请求方法有主体（POST/PUT/DELETE），读取剩余内容
        if (req.method == "POST" || req.method == "PUT" || req.method == "DELETE") {
            // 尝试从 Content-Length 获取主体长度
            auto it = req.headers.find("Content-Length");
            if (it == req.headers.end()) {
                // 不区分大小写查找
                for (const auto& [key, val] : req.headers) {
                    std::string lowerKey;
                    for (char c : key) lowerKey += std::tolower(c);
                    if (lowerKey == "content-length") {
                        it = req.headers.find(key);
                        break;
                    }
                }
            }

            if (it != req.headers.end()) {
                int bodyLength = std::stoi(it->second);
                // 读取剩余内容作为主体
                std::string remaining;
                while (std::getline(stream, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    if (!remaining.empty()) remaining += "\n";
                    remaining += line;
                }
                if ((int)remaining.length() > bodyLength) {
                    remaining = remaining.substr(0, bodyLength);
                }
                req.body = remaining;
            } else {
                // 没有 Content-Length，读取全部剩余内容
                std::string remaining;
                while (std::getline(stream, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    if (!remaining.empty()) remaining += "\n";
                    remaining += line;
                }
                req.body = remaining;
            }
        }

        return req;
    }

    // ============================================================
    // HTTP 响应构建
    //
    // 将 HttpResponse 结构体序列化为 HTTP 响应报文
    // 格式：HTTP/1.1 200 OK\r\nHeader: Value\r\n\r\nBody
    // ============================================================
    std::string http_server::Response_Build(const HttpResponse& res) {
        std::ostringstream response;

        // 1. 状态行
        response << "HTTP/1.1 " << res.status_code << " "
                 << res.status_text << "\r\n";

        // 2. 响应头
        for (const auto& [key, value] : res.headers) {
            response << key << ": " << value << "\r\n";
        }

        // 3. Content-Length（如果没有设置则自动计算）
        if (res.headers.find("Content-Length") == res.headers.end()) {
            response << "Content-Length: " << res.body.length() << "\r\n";
        }

        // 4. 空行分隔头部和主体
        response << "\r\n";

        // 5. 响应主体
        response << res.body;

        return response.str();
    }

    // ============================================================
    // 默认请求处理器
    //
    // 处理流程：
    // 1. 查找路由表 → 匹配则调用对应的处理器
    // 2. 非 API 路径 → 尝试提供静态文件服务
    // 3. 都不匹配 → 返回 404
    // ============================================================
    HttpResponse http_server::Default_Request_Handle(const HttpRequest& req) {
        HttpResponse res;
        res.headers["Access-Control-Allow-Origin"] = "*";
        res.headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
        res.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";

        // 处理 OPTIONS 预检请求（CORS）
        if (req.method == "OPTIONS") {
            res.status_code = 200;
            res.status_text = "OK";
            return res;
        }

        // ---- 1. 查找路由表 ----
        if (req.method == "GET") {
            auto it = m_get_routes.find(req.path);
            if (it != m_get_routes.end()) {
                return it->second(req);
            }
        } else if (req.method == "POST") {
            auto it = m_post_routes.find(req.path);
            if (it != m_post_routes.end()) {
                return it->second(req);
            }
        } else if (req.method == "PUT") {
            auto it = m_put_routes.find(req.path);
            if (it != m_put_routes.end()) {
                return it->second(req);
            }
        } else if (req.method == "DELETE") {
            auto it = m_delete_routes.find(req.path);
            if (it != m_delete_routes.end()) {
                return it->second(req);
            }
        }

        // ---- 2. 尝试提供静态文件 ----
        if (req.method == "GET") {
            // 构建文件路径
            std::string filePath = m_static_dir + req.path;

            // 如果路径以 "/" 结尾，默认提供 index.html
            if (filePath.back() == '/') {
                filePath += "index.html";
            }

            // 检查文件是否存在
            if (fs::exists(filePath) && fs::is_regular_file(filePath)) {
                std::string content = Tools::Read_File(filePath);
                if (!content.empty()) {
                    res.status_code = 200;
                    res.status_text = "OK";

                    // 根据扩展名设置 Content-Type
                    std::string ext = fs::path(filePath).extension().string();
                    res.headers["Content-Type"] = Tools::Get_MimeType(ext);
                    res.body = content;
                    return res;
                }
            }

            // 如果请求路径是 "/"，也尝试直接加载 index.html
            if (req.path == "/") {
                std::string indexPath = m_static_dir + "/index.html";
                if (fs::exists(indexPath)) {
                    std::string content = Tools::Read_File(indexPath);
                    if (!content.empty()) {
                        res.status_code = 200;
                        res.status_text = "OK";
                        res.headers["Content-Type"] = "text/html";
                        res.body = content;
                        return res;
                    }
                }
            }
        }

        // ---- 3. 404 未找到 ----
        res.status_code = 404;
        res.status_text = "Not Found";
        res.headers["Content-Type"] = "application/json";
        res.body = Build_Json_Response(404, "Not Found", "");
        return res;
    }

    // ============================================================
    // 接受连接主循环
    //
    // 持续接受新客户端连接，为每个客户端创建新线程处理
    // ============================================================
    void http_server::Run_Accept_Loop() {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        Tools::Out_System_Http("Accept loop started on port " + std::to_string(m_port));

        while (m_running) {
            // 接受新客户端连接
            // accept() 会阻塞直到有新连接到达
            SOCKET client_socket = accept(m_server_socket,
                                          (struct sockaddr*)&client_addr,
                                          &client_len);

            if (client_socket < 0) {
                if (m_running) {
                    Tools::Out_System_Error("Failed to accept client connection");
                }
                break;
            }

            // 获取客户端 IP 地址
            char client_ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr),
                      client_ip_str, INET_ADDRSTRLEN);
            std::string client_ip(client_ip_str);

            Tools::Out_System_Http("New connection from " + client_ip);

            // 为每个客户端创建独立线程处理
            // 传递 this 指针以访问服务器数据
            std::thread client_thread(Handle_Client, client_socket, this);
            client_thread.detach();  // 分离线程，自动释放资源
        }

        Tools::Out_System_Http("Accept loop ended");
    }

    // ============================================================
    // 处理单个客户端请求
    //
    // 静态方法：作为线程入口点
    // 1. 接收 HTTP 请求数据
    // 2. 解析请求
    // 3. 路由分发
    // 4. 构建并发送响应
    // 5. 关闭连接
    // ============================================================
    void http_server::Handle_Client(SOCKET clientSocket, http_server* server) {
        // 1. 接收请求数据
        std::string raw_request;
        char buffer[4096];
        int bytes_received;

        // 循环读取直到连接关闭或缓冲区满
        while ((bytes_received = read(clientSocket, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_received] = '\0';
            raw_request += buffer;

            // 如果收到的数据包含完整的 HTTP 报文结尾标记 \r\n\r\n
            // 且已经有了 Content-Length 指定的字节数，就停止读取
            if (raw_request.find("\r\n\r\n") != std::string::npos) {
                // 尝试解析 Content-Length
                size_t headerEnd = raw_request.find("\r\n\r\n");
                std::string headers = raw_request.substr(0, headerEnd);
                std::string body_part = raw_request.substr(headerEnd + 4);

                // 查找 Content-Length
                size_t clPos = headers.find("Content-Length: ");
                if (clPos != std::string::npos) {
                    size_t clEnd = headers.find("\r\n", clPos);
                    std::string clStr = headers.substr(clPos + 16, clEnd - clPos - 16);
                    try {
                        int contentLength = std::stoi(clStr);
                        if ((int)body_part.length() >= contentLength) {
                            break;  // 已收到完整请求
                        }
                    } catch (...) {
                        break;  // Content-Length 解析失败
                    }
                } else {
                    break;  // 没有 Content-Length
                }
            }
        }

        if (bytes_received < 0 && raw_request.empty()) {
            // 读取失败且没有数据
            close(clientSocket);
            return;
        }

        // 2. 解析 HTTP 请求
        HttpRequest req = server->Request_Parse(raw_request);

        // 3. 路由分发 -> 获取响应
        HttpResponse res = server->Default_Request_Handle(req);

        // 4. 构建并发送响应
        std::string response_str = server->Response_Build(res);
        write(clientSocket, response_str.c_str(), response_str.length());

        // 5. 关闭客户端连接
        close(clientSocket);
    }

} // namespace Http