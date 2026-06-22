//FrameWork.h工具实现文件
//功能：用户业务线程、HTTP 服务器路由、平台初始化函数

#include "FarmeWork.h"

//全局变量定义
Http::http_server* g_server = nullptr;               // HTTP 服务器全局指针
std::atomic<bool> g_running(true);                   // 全局运行标志

//用户线程主循环（每个用户一个独立的 "main"）

//用户线程主循环函数
//每个用户登录后都会创建一个独立线程，运行此函数
//该线程有自己独立的 DB 连接，像独立进程一样处理该用户的业务
//@param info 用户线程信息结构体指针
void User_Worker_Routine(TaskSystem::UserThreadInfo* info) {
    // 打印用户线程启动日志：包含用户名、ID、权限级别
    Tools::Out_System_user(info->name,"用户线程启动 - 用户: " 
        + info->name +" (ID: " + std::to_string(info->userId) 
        + ", 级别: " + std::to_string(info->level) + ")");

    info->startTime = std::time(nullptr);   // 记录线程启动时间

    // 每个用户线程拥有自己独立的 MySQL 数据库连接（像 main 函数一样有自己的 DB 会话！）
    MySQL::mysql* threadDb = nullptr;
    try {
        threadDb = new MySQL::mysql();      // 为该线程创建一个独立的 DB 连接
        Tools::Out_System_Mysql("用户线程 " + info->name + " 数据库连接成功");
    } catch (const std::exception& e) {
        Tools::Out_System_Error("用户线程 " + info->name + " 数据库连接失败: " + std::string(e.what()));
    }

    // 用户线程主循环 
    // 循环条件：该线程未被标记停止 且 全局服务器仍在运行
    while (info->running && g_running) {
        std::unique_lock<std::mutex> lock(info->taskMutex); // 先获取任务队列锁

        // 条件等待：等待任务到来，最长阻塞 5 秒
        // 5 秒超时后即使没有任务也会返回 false，可用于执行定期后台任务
        if (info->taskCV.wait_for(lock, std::chrono::seconds(5), [info]() {
            // 唤醒条件：队列非空 / 线程被要求停止 / 全局服务器关闭
            return !info->taskQueue.empty() || !info->running || !g_running;
        })) {
            // ─── 有任务到达：逐一处理队列中的所有待办任务 ───
            while (!info->taskQueue.empty() && info->running && g_running) {
                TaskSystem::UserTask task = std::move(info->taskQueue.front()); // 取出队首任务
                info->taskQueue.erase(info->taskQueue.begin());     // 从队列中移除
                lock.unlock();      // 处理任务时释放锁，允许新任务入队

                // 更新任务状态为 PROCESSING（处理中），通知前端任务已在执行
                TaskSystem::Update_Task_Result(task.taskId, info->userId, TaskSystem::TaskStatus::PROCESSING, "");

                try {
                    switch (task.type) {

                        // ===== PING =====
                        case TaskSystem::UserTaskType::PING: {
                            std::string echo = Tools::Json::Parse_Json_String(task.input, "message");
                            std::string result = "{"
                                "\"pong\":true,"
                                "\"echo\":\"" + echo + "\","
                                "\"userId\":" + std::to_string(info->userId) + ","
                                "\"userName\":\"" + info->name + "\"}";
                            TaskSystem::Update_Task_Result(task.taskId, info->userId,
                                             TaskSystem::TaskStatus::COMPLETED, result);
                            break;
                        }

                        // ===== PROCESS_DATA =====
                        case TaskSystem::UserTaskType::PROCESS_DATA: {
                            std::string content = Tools::Json::Parse_Json_String(task.input, "content");
                            std::string dataType = Tools::Json::Parse_Json_String(task.input, "type");

                            Tools::Out_System("用户线程 " + info->name +
                                              " 处理数据: " + dataType +
                                              " / " + content.substr(0, 30) + "...");

                            std::string result;
                            if (threadDb) {
                                // ★ 在这里写你的数据库业务代码 ★
                                // std::string sql = "INSERT INTO logs(user_id,type,content) VALUES("
                                //     + std::to_string(info->userId) + ",'" + dataType + "','" + content + "')";
                                // mysql_query(threadDb->conn, sql.c_str());

                                result = "{"
                                    "\"processed\":true,"
                                    "\"userId\":" + std::to_string(info->userId) + ","
                                    "\"type\":\"" + dataType + "\","
                                    "\"contentLength\":" + std::to_string(content.size()) + ","
                                    "\"savedToDb\":true}";
                            } else {
                                result = "{\"processed\":true,\"savedToDb\":false}";
                            }
                            TaskSystem::Update_Task_Result(task.taskId, info->userId,
                                             TaskSystem::TaskStatus::COMPLETED, result);
                            break;
                        }

                        // ===== SEND_NOTIFICATION =====
                        case TaskSystem::UserTaskType::SEND_NOTIFICATION: {
                            std::string title = Tools::Json::Parse_Json_String(task.input, "title");
                            std::string body  = Tools::Json::Parse_Json_String(task.input, "body");

                            Tools::Out_System("用户线程 " + info->name +
                                              " 通知: " + title + " - " + body);

                            // ★ 在这里写推送逻辑 ★
                            std::string result = "{"
                                "\"notified\":true,"
                                "\"title\":\"" + title + "\","
                                "\"body\":\"" + body + "\","
                                "\"userId\":" + std::to_string(info->userId) + "}";
                            TaskSystem::Update_Task_Result(task.taskId, info->userId,
                                             TaskSystem::TaskStatus::COMPLETED, result);
                            break;
                        }

                        // ===== SYNC_DATABASE =====
                        case TaskSystem::UserTaskType::SYNC_DATABASE: {
                            std::string table = Tools::Json::Parse_Json_String(task.input, "table");
                            std::string data  = Tools::Json::Parse_Json_Value_Raw(task.input, "data");

                            Tools::Out_System("用户线程 " + info->name +
                                              " 同步数据库: " + table);

                            std::string result;
                            if (threadDb) {
                                // ★ 在这里写数据同步代码 ★
                                // std::string sql = "UPDATE " + table + " SET ... WHERE user_id="
                                //     + std::to_string(info->userId);
                                // mysql_query(threadDb->conn, sql.c_str());

                                result = "{"
                                    "\"synced\":true,"
                                    "\"table\":\"" + table + "\","
                                    "\"userId\":" + std::to_string(info->userId) + ","
                                    "\"affectedRows\":1}";
                            } else {
                                result = "{\"synced\":false,\"error\":\"数据库不可用\"}";
                            }
                            TaskSystem::Update_Task_Result(task.taskId, info->userId,
                                             TaskSystem::TaskStatus::COMPLETED, result);
                            break;
                        }

                        // ===== CUSTOM_EVENT =====
                        case TaskSystem::UserTaskType::CUSTOM_EVENT: {
                            std::string eventType = Tools::Json::Parse_Json_String(task.input, "event");
                            if (eventType.empty()) eventType = "unknown";

                            Tools::Out_System("用户线程 " + info->name +
                                              " 事件: " + eventType);

                            std::string result = "{"
                                "\"event\":\"" + eventType + "\","
                                "\"processed\":true,"
                                "\"userId\":" + std::to_string(info->userId) + ","
                                "\"userName\":\"" + info->name + "\","
                                "\"echo\":" + task.input + "}";
                            TaskSystem::Update_Task_Result(task.taskId, info->userId,
                                             TaskSystem::TaskStatus::COMPLETED, result);
                            break;
                        }

                        // ===== SHUTDOWN =====
                        case TaskSystem::UserTaskType::SHUTDOWN: {
                            std::string result = "{\"shutdown\":true,\"userId\":"
                                + std::to_string(info->userId) + "}";
                            TaskSystem::Update_Task_Result(task.taskId, info->userId,
                                             TaskSystem::TaskStatus::COMPLETED, result);
                            Tools::Out_System("用户线程 " + info->name + " 收到关闭指令");
                            info->running = false;
                            break;
                                        }
                    }
                } catch (const std::exception& e) {
                    // 捕获到异常，将任务标记为 FAILED，记录错误消息
                    TaskSystem::Update_Task_Result(task.taskId, info->userId,
                                     TaskSystem::TaskStatus::FAILED, "", e.what());
                    Tools::Out_System_Error("用户线程 " + info->name +
                                           " 任务异常: " + std::string(e.what()));
                }

                                                info->tasksProcessed++;  // 累加已处理任务数（原子操作）
                lock.lock();  // 重新锁上任务队列，准备检查下一轮循环条件
            }
        } else {
            // ─── 5秒超时到达，没有新任务 → 执行定期后台任务 ───
            lock.unlock();  // 释放任务队列锁

            // 使用线程本地变量记录上次心跳时间，避免多线程间共享 static 变量导致竞态
            thread_local std::time_t lastCleanup = 0; // 线程本地上次心跳时间
            std::time_t now = std::time(nullptr);     // 当前时间戳
            if (now - lastCleanup >= 30) {            // 每30秒打印一次心跳日志
                lastCleanup = now;
                Tools::Out_System("用户线程 " + info->name +
                                  " 心跳 (已处理 " + std::to_string(info->tasksProcessed.load()) + " 个任务)");
            }
        }
    }

    // 线程退出前清理：关闭该线程独立的数据库连接
    if (threadDb) {
        Tools::Out_System("用户线程 " + info->name + " 关闭数据库连接");
        delete threadDb;
    }

    // 标记线程已停止（确保其他组件知道此线程不再活跃）
    info->running = false;

    // 分离自身线程，避免 std::thread 析构时调用 std::terminate
    if (info->worker.joinable()) {
        info->worker.detach();
    }

    Tools::Out_System("用户线程 " + info->name + " 退出 (共处理 " +
                      std::to_string(info->tasksProcessed.load()) + " 个任务)");
}

//为用户创建一个独立的业务线程
//每个登录用户会获得一个专属线程，处理该用户的所有异步任务
void Create_User_Thread(int userId, const std::string& name, int level) {
    std::lock_guard<std::mutex> lock(TaskSystem::g_userThreadsMutex);  // 保护用户线程表

    // 尝试重用或清理已退出的用户线程
    auto existing = TaskSystem::g_userThreads.find(userId);
    if (existing != TaskSystem::g_userThreads.end()) {
        if (existing->second->running.load()) {
            Tools::Out_System("用户线程 " + name + " 已有线程在运行");
            return;  // 该用户已有活跃线程，不再重复创建
        }
        // 线程已标记停止但未清理 → 移除旧条目，后续重新创建
        if (existing->second->worker.joinable()) {
            existing->second->worker.detach();  // 确保线程完全分离
        }
        TaskSystem::g_userThreads.erase(existing);
    }
    auto info = std::make_unique<TaskSystem::UserThreadInfo>();  // 创建用户线程信息对象
    info->userId = userId;
    info->name = name;
    info->level = level;
    info->running = true;                  // 标记线程为运行状态
    info->startTime = std::time(nullptr);  // 记录创建时间
    info->worker = std::thread(User_Worker_Routine, info.get());  // 启动工作线程
    TaskSystem::g_userThreads[userId] = std::move(info);  // 移入全局表管理
    Tools::Out_System("为用户 " + name + " (ID: " + std::to_string(userId) +
                      ") 创建了专属业务线程");
}
//HTTP 服务器路由
//HTTP 服务器主函数——在子进程/子线程中运行，启动 HTTP 服务器并注册所有 API 路由
void Http_Server_Routine(int Port) {
    // 打印当前进程 PID，便于调试
    std::cout << "HTTP Server worker started, PID: "
#ifdef _WIN32
              << _getpid()
#else
              << getpid()
#endif
              << std::endl;

    // 创建 HTTP 服务器专用的 MySQL 连接（用于 /api/login 等需要直接查 DB 的路由）
    MySQL::mysql* db = nullptr;
    try {
        db = new MySQL::mysql();
        Tools::Out_System("子进程 MySQL 连接成功");
    } catch (const std::exception& e) {
        Tools::Out_System_Error("子进程 MySQL 连接失败: " + std::string(e.what()));
    }

                    // 动态分配 HTTP 服务器实例，避免局部变量销毁后 g_server 成为悬空指针
    g_server = new Http::http_server(Port, 100);  // 创建 HTTP 服务器实例，最大连接数 100

    // 注册所有业务路由（原分散在下的 lambda 路由已整合到 RegisterBusinessRoutes()）
    g_server->RegisterBusinessRoutes();

        // ===== POST /api/login 用户登录（支持多设备登录） =====
    // 需要 db 局部变量做数据库查询，所以暂不抽离到 RegisterBusinessRoutes()
    g_server->Post("/api/login", [db](const Http::HttpRequest& req) -> Http::HttpResponse {
        Http::HttpResponse res;
        res.status_code = 200;
        res.status_text = "OK";
        res.headers["Content-Type"] = "application/json";
        res.headers["Access-Control-Allow-Origin"] = "*";  // 允许跨域

        // 检查数据库是否可用
        if (!db) {
            res.status_code = 500;
            res.body = "{\"status\": \"error\", \"message\": \"数据库不可用\"}";
            return res;
        }

        // 从请求 JSON body 中解析出用户名、密码、设备名
        std::string name = Tools::Json::Parse_Json_String(req.body, "name");
        std::string password = Tools::Json::Parse_Json_String(req.body, "password");
        std::string deviceName = Tools::Json::Parse_Json_String(req.body, "device_name");

                // 参数非空校验
        if (name.empty() || password.empty()) {
            res.status_code = 400;
            res.body = "{\"status\": \"fail\", \"message\": \"用户名或密码不能为空\"}";
            return res;
        }

        // 从请求头中提取 User-Agent（兼容大小写）
        std::string userAgent;
        auto uaIt = req.headers.find("User-Agent");
        if (uaIt == req.headers.end()) {
            uaIt = req.headers.find("user-agent");
        }
        if (uaIt != req.headers.end()) {
            userAgent = uaIt->second;
        }

        std::string clientIp = req.client_ip;  // 获取客户端IP
        if (clientIp.empty()) clientIp = "unknow";

        // 如果前端未提供设备名，则从 User-Agent 截取前段作为设备名
        if (deviceName.empty()) {
            if (!userAgent.empty()) {
                deviceName = userAgent.substr(0, 30);  // 取前30字符
                size_t spacePos = deviceName.find(' ');
                if (spacePos != std::string::npos && spacePos > 5) {
                    deviceName = deviceName.substr(0, spacePos);  // 只取第一个单词
                }
            } else {
                deviceName = "未知设备";
            }
        }

                                std::cout << "[登录] 尝试登录 - name: '" << name << "', device: '" << deviceName << "', IP: " << clientIp << std::endl;
        std::string userAvatar;
        int permission = db->User(name, password, &userAvatar);  // 验证用户名密码，返回权限级别，同时获取头像

        if (permission > 0) {  // 验证成功（permission > 0）
            int userId = db->Get_UserId(name);      // 获取用户ID

            // 更新用户登录信息：最后登录时间、IP、登录次数
            std::time_t now = std::time(nullptr);
            std::string updateSql = "UPDATE users SET "
                "last_login_time = FROM_UNIXTIME(" + std::to_string(now) + "), "
                "last_login_ip = '" + clientIp + "', "
                "login_count = login_count + 1 "
                "WHERE id = " + std::to_string(userId);
            mysql_query(db->conn, updateSql.c_str());

            std::string token = Tools::Auth::Generate_Token();    // 生成会话 token

            {   // 将登录会话信息存入全局 token 存储
                std::lock_guard<std::mutex> lock(Tools::Auth::g_tokenMutex);
                Tools::Auth::DeviceInfo device;
                device.deviceName = deviceName;
                device.userAgent = userAgent;
                device.clientIp = clientIp;
                device.loginTime = now;
                Tools::Auth::g_tokenStore[token] = {userId, permission, name, device};
            }

                                std::cout << "[登录] 成功 - 用户: " << name
                  << ", 设备: " << deviceName
                  << ", IP: " << clientIp << std::endl;
            Create_User_Thread(userId, name, permission);  // 创建该用户的专属业务线程

            // 向用户线程投递一个登录事件（记录设备信息）
            TaskSystem::Post_User_Task(userId, TaskSystem::UserTaskType::CUSTOM_EVENT,
                         "{\"event\":\"login\",\"device\":\"" + deviceName + "\"}", "登录系统");

            // 收集该用户所有已登录设备列表，返回给前端
            std::string devicesJson = "[";
                        // 遍历全局 token 表，收集该用户的全部登录设备信息
            {
                std::lock_guard<std::mutex> lock(Tools::Auth::g_tokenMutex);
                bool first = true;
                for (const auto& [t, session] : Tools::Auth::g_tokenStore) {
                    if (session.userId == userId) {
                        if (!first) devicesJson += ",";
                        first = false;
                        devicesJson += "{";
                        devicesJson += std::string("\"device_name\":\"") + session.device.deviceName + "\",";
                        devicesJson += std::string("\"client_ip\":\"") + session.device.clientIp + "\",";
                        devicesJson += std::string("\"login_time\":") + std::to_string(session.device.loginTime);
                        devicesJson += "}";
                    }
                }
            }
            devicesJson += "]";

                        // 组装登录成功响应：包含 token、用户信息、设备列表、头像
                        res.body = "{"
                "\"status\": \"success\", "
                "\"message\": \"登录成功\", "
                "\"token\": \"" + token + "\", "
                "\"user_id\": " + std::to_string(userId) + ", "
                "\"level\": " + std::to_string(permission) + ", "
                "\"avatar\": \"" + userAvatar + "\", "
                "\"device_name\": \"" + deviceName + "\", "
                "\"devices\": " + devicesJson + "}";
        } else {
            // 验证失败返回 401
            res.status_code = 401;
            res.body = "{\"status\": \"fail\", \"message\": \"用户名或密码错误\"}";
        }
                return res;
    });

    // ===== POST /api/register 用户注册 =====
    g_server->Post("/api/register", [db](const Http::HttpRequest& req) -> Http::HttpResponse {
        Http::HttpResponse res;
        res.status_code = 200;
        res.status_text = "OK";
        res.headers["Content-Type"] = "application/json";
        res.headers["Access-Control-Allow-Origin"] = "*";

        if (!db) {
            res.status_code = 500;
            res.body = "{\"status\": \"error\", \"message\": \"数据库不可用\"}";
            return res;
        }

        std::string name = Tools::Json::Parse_Json_String(req.body, "name");
        std::string password = Tools::Json::Parse_Json_String(req.body, "password");

        if (name.empty() || password.empty()) {
            res.status_code = 400;
            res.body = "{\"status\": \"fail\", \"message\": \"用户名或密码不能为空\"}";
            return res;
        }

        // 检查用户名是否已存在
        int existingId = db->Get_UserId(name);
        if (existingId > 0) {
            res.body = "{\"status\": \"fail\", \"message\": \"用户名已存在\"}";
            return res;
        }

        // 插入新用户（默认 permission = 1 普通用户）
        std::string sql = "INSERT INTO users (name, password, permission, login_count, created_at) VALUES ('"
            + name + "', '" + password + "', 1, 0, NOW())";
        if (mysql_query(db->conn, sql.c_str())) {
            res.status_code = 500;
            res.body = "{\"status\": \"error\", \"message\": \"注册失败: " + std::string(mysql_error(db->conn)) + "\"}";
            return res;
        }

        Tools::Out_System_Mysql("新用户注册成功: " + name);
        res.body = "{\"status\": \"success\", \"message\": \"注册成功\"}";
        return res;
    });

                // ===== GET /api/userinfo 获取当前用户信息 =====
    g_server->Get("/api/userinfo", [](const Http::HttpRequest& req) -> Http::HttpResponse {
        Http::HttpResponse res;
        res.status_code = 200;
        res.status_text = "OK";
        res.headers["Content-Type"] = "application/json";
        res.headers["Access-Control-Allow-Origin"] = "*";

        // 验证 token
        std::string token = Tools::Auth::Get_Token_From_Request(req);
        Tools::Auth::SessionInfo* session = Tools::Auth::Validate_Token(token);
        if (!session) {
            res.status_code = 401;
            res.body = "{\"status\": \"fail\", \"message\": \"未登录或 token 已失效\"}";
            return res;
        }

        // 返回用户信息：ID、权限级别、用户名、设备名、IP、登录时间
        res.body = "{"
            "\"status\": \"success\", "
            "\"user_id\": " + std::to_string(session->userId) + ", "
            "\"level\": " + std::to_string(session->level) + ", "
            "\"name\": \"" + session->name + "\", "
            "\"device_name\": \"" + session->device.deviceName + "\", "
            "\"client_ip\": \"" + session->device.clientIp + "\", "
            "\"login_time\": " + std::to_string(session->device.loginTime) + "}";
        return res;
    });

                // ===== POST /api/task/result 查询异步任务执行结果 =====
    g_server->Post("/api/task/result", [](const Http::HttpRequest& req) -> Http::HttpResponse {
        Http::HttpResponse res;
        res.status_code = 200;
        res.status_text = "OK";
        res.headers["Content-Type"] = "application/json";
        res.headers["Access-Control-Allow-Origin"] = "*";

        // 从请求体中获取 task_id
        std::string taskId = Tools::Json::Parse_Json_String(req.body, "task_id");
        if (taskId.empty()) {
            res.body = "{\"status\":\"fail\",\"message\":\"请提供 task_id\"}";
            return res;
        }

        // 验证登录 token
        std::string token = Tools::Auth::Get_Token_From_Request(req);
        Tools::Auth::SessionInfo* session = Tools::Auth::Validate_Token(token);
        if (!session) {
            res.status_code = 401;
            res.body = "{\"status\":\"fail\",\"message\":\"未登录\"}";
            return res;
        }

        // 在全局结果表中查找该任务
        TaskSystem::TaskResult* result = TaskSystem::Get_Task_Result(taskId);
        if (!result) {
            res.status_code = 404;
            res.body = "{\"status\":\"fail\",\"message\":\"任务不存在或已过期\"}";
            return res;
        }

        // 权限检查：只能查询自己的任务
        if (result->userId != session->userId) {
            res.status_code = 403;
            res.body = "{\"status\":\"fail\",\"message\":\"无权查看此任务\"}";
            return res;
        }

        // 将枚举状态转为前端可读的字符串
        std::string statusStr;
        switch (result->status) {
            case TaskSystem::TaskStatus::PENDING:    statusStr = "pending";    break;
            case TaskSystem::TaskStatus::PROCESSING: statusStr = "processing"; break;
            case TaskSystem::TaskStatus::COMPLETED:  statusStr = "completed";  break;
            case TaskSystem::TaskStatus::FAILED:     statusStr = "failed";     break;
            default:                     statusStr = "unknown";    break;
        }

        // 组装完整响应：任务ID、状态、输出、错误信息、创建/完成时间
        res.body = "{"
            "\"status\":\"success\","
            "\"task_id\":\"" + result->taskId + "\","
            "\"task_status\":\"" + statusStr + "\","
            "\"output\":" + (result->output.empty() ? "null" : result->output) + ","
            "\"error\":\"" + result->errorMessage + "\","
            "\"create_time\":" + std::to_string(result->createTime) + ","
            "\"complete_time\":" + std::to_string(result->completeTime) + "}";
        return res;
    });

                // ===== GET /api/devices 获取该用户所有已登录设备 =====
    g_server->Get("/api/devices", [](const Http::HttpRequest& req) -> Http::HttpResponse {
        Http::HttpResponse res;
        res.status_code = 200;
        res.status_text = "OK";
        res.headers["Content-Type"] = "application/json";
        res.headers["Access-Control-Allow-Origin"] = "*";

        // 验证 token
        std::string token = Tools::Auth::Get_Token_From_Request(req);
        Tools::Auth::SessionInfo* session = Tools::Auth::Validate_Token(token);
        if (!session) {
            res.status_code = 401;
            res.body = "{\"status\": \"fail\", \"message\": \"未登录或 token 已失效\"}";
            return res;
        }

        int userId = session->userId;
        // 遍历全局 token 表，收集该用户所有设备的登录信息
        std::string devicesJson = "[";
        bool first = true;
        {
            std::lock_guard<std::mutex> lock(Tools::Auth::g_tokenMutex);
            for (const auto& [t, s] : Tools::Auth::g_tokenStore) {
                if (s.userId == userId) {
                    if (!first) devicesJson += ",";
                    first = false;
                    devicesJson += "{";
                    devicesJson += std::string("\"token_prefix\":\"") + t.substr(0, 8) + "...\",";
                    devicesJson += std::string("\"is_current\": ") + ((t == token) ? "true" : "false") + ",";
                    devicesJson += std::string("\"device_name\":\"") + s.device.deviceName + "\",";
                    devicesJson += std::string("\"client_ip\":\"") + s.device.clientIp + "\",";
                    devicesJson += std::string("\"user_agent\":\"") + s.device.userAgent + "\",";
                    devicesJson += std::string("\"login_time\":") + std::to_string(s.device.loginTime);
                    devicesJson += "}";
                }
            }
        }
        devicesJson += "]";

        res.body = "{"
            "\"status\": \"success\", "
            "\"user_id\": " + std::to_string(userId) + ", "
            "\"devices\": " + devicesJson + "}";
        return res;
    });

                // ===== POST /api/logout 退出登录（支持多设备独立退出） =====
    g_server->Post("/api/logout", [](const Http::HttpRequest& req) -> Http::HttpResponse {
        Http::HttpResponse res;
        res.status_code = 200;
        res.status_text = "OK";
        res.headers["Content-Type"] = "application/json";
        res.headers["Access-Control-Allow-Origin"] = "*";

                std::string token = Tools::Auth::Get_Token_From_Request(req);
        std::string deviceName = "未知";
        if (!token.empty()) {
            int userId = -1;
            bool otherDevicesOnline = false;
            std::string userName;

            // 第一阶段：在 Tools::Auth::g_tokenMutex 保护下收集信息并删除 token
            {
                std::lock_guard<std::mutex> lock(Tools::Auth::g_tokenMutex);
                auto it = Tools::Auth::g_tokenStore.find(token);
                if (it == Tools::Auth::g_tokenStore.end()) {
                    // token 不存在，直接返回
                    res.body = "{\"status\": \"success\", \"message\": \"已退出登录\"}";
                    return res;
                }

                deviceName = it->second.device.deviceName;
                userId = it->second.userId;
                userName = it->second.name;

                // 检查该用户是否还有其它设备在线（不包括当前要注销的）
                for (const auto& [otherToken, otherSession] : Tools::Auth::g_tokenStore) {
                    if (otherToken != token && otherSession.userId == userId) {
                        otherDevicesOnline = true;
                        break;
                    }
                }

                Tools::Auth::g_tokenStore.erase(it);  // 删除当前设备的 token
                std::cout << "[退出] 设备 '" << deviceName << "' 已注销登录" << std::endl;
            }
            // Tools::Auth::g_tokenMutex 已释放

            // 第二阶段：根据是否还有其他设备，决定是否关闭用户线程
            if (!otherDevicesOnline && userId > 0) {
                // 没有其他设备在线 → 关闭该用户的专属线程
                // 此时 Tools::Auth::g_tokenMutex 已释放，不会产生死锁
                TaskSystem::Post_User_Task(userId, TaskSystem::UserTaskType::SHUTDOWN, "{}", "logout");

                // 注：不在此处立即删除 TaskSystem::g_userThreads 中的条目。
                // 用户线程处理 SHUTDOWN 任务后会自然退出循环并清理自身。
                // 线程退出后，其对应的 UserThreadInfo 会由专门的清理机制处理。
                std::cout << "[退出] 用户 " << userName
                          << " 已无其他设备在线，已发送线程关闭指令" << std::endl;
            } else if (userId > 0) {
                // 还有其他设备在线 → 保留用户线程，只注销当前设备
                std::cout << "[退出] 用户 " << userName
                          << " 还有其他设备在线，保留用户线程" << std::endl;
            }
        }
        res.body = "{"
            "\"status\": \"success\", "
            "\"message\": \"设备 '";
        res.body += deviceName;
        res.body += "' 已退出登录\"}";
        return res;
    });

                // ===== POST /api/data 提交数据处理任务 =====
    g_server->Post("/api/data", [db](const Http::HttpRequest& req) -> Http::HttpResponse {
        Http::HttpResponse res;
        res.status_code = 200;
        res.status_text = "OK";
        res.headers["Content-Type"] = "application/json";
        res.headers["Access-Control-Allow-Origin"] = "*";

        // 验证登录
        std::string token = Tools::Auth::Get_Token_From_Request(req);
        Tools::Auth::SessionInfo* session = Tools::Auth::Validate_Token(token);
        if (!session) {
            res.status_code = 401;
            res.body = "{\"status\": \"fail\", \"message\": \"未登录\"}";
            return res;
        }

        // 向该用户的线程投递一个 PROCESS_DATA 任务
        std::string taskId = TaskSystem::Post_User_Task(session->userId, TaskSystem::UserTaskType::PROCESS_DATA,
                                          req.body, "POST /api/data");
        if (!taskId.empty()) {
            res.body = "{"
                "\"status\": \"success\", "
                "\"message\": \"数据已提交到用户线程处理\", "
                "\"task_id\": \"" + taskId + "\", "
                "\"user_id\": " + std::to_string(session->userId) + "}";
        } else {
            res.status_code = 500;
            res.body = "{\"status\": \"error\", \"message\": \"用户线程不可用\"}";
        }
        return res;
    });

        // ===== PUT /api/update 提交数据更新任务 =====
    g_server->Put("/api/update", [](const Http::HttpRequest& req) -> Http::HttpResponse {
        Http::HttpResponse res;
        res.status_code = 200;
        res.status_text = "OK";
        res.headers["Content-Type"] = "application/json";
        res.headers["Access-Control-Allow-Origin"] = "*";

        std::string token = Tools::Auth::Get_Token_From_Request(req);
        Tools::Auth::SessionInfo* session = Tools::Auth::Validate_Token(token);
        if (!session) {
            res.status_code = 401;
            res.body = "{\"status\": \"fail\", \"message\": \"未登录\"}";
            return res;
        }

        // 向该用户的线程投递一个 SYNC_DATABASE 任务
        std::string taskId = TaskSystem::Post_User_Task(session->userId, TaskSystem::UserTaskType::SYNC_DATABASE,
                                          req.body, "PUT /api/update");
        if (!taskId.empty()) {
            res.body = "{"
                "\"status\": \"success\", "
                "\"message\": \"更新请求已提交到用户线程\", "
                "\"task_id\": \"" + taskId + "\", "
                "\"user_id\": " + std::to_string(session->userId) + "}";
        } else {
            res.status_code = 500;
            res.body = "{\"status\": \"error\", \"message\": \"用户线程不可用\"}";
        }
        return res;
    });

        // ===== DELETE /api/delete 提交删除任务 =====
    g_server->Delete("/api/delete", [](const Http::HttpRequest& req) -> Http::HttpResponse {
        Http::HttpResponse res;
        res.status_code = 200;
        res.status_text = "OK";
        res.headers["Content-Type"] = "application/json";
        res.headers["Access-Control-Allow-Origin"] = "*";

        std::string token = Tools::Auth::Get_Token_From_Request(req);
        Tools::Auth::SessionInfo* session = Tools::Auth::Validate_Token(token);
        if (!session) {
            res.status_code = 401;
            res.body = "{\"status\": \"fail\", \"message\": \"未登录\"}";
            return res;
        }

        // 向该用户的线程投递一个 CUSTOM_EVENT 任务（action=delete）
        std::string taskId = TaskSystem::Post_User_Task(session->userId, TaskSystem::UserTaskType::CUSTOM_EVENT,
                                          "{\"action\":\"delete\"}", "DELETE /api/delete");
        if (!taskId.empty()) {
            res.body = "{"
                "\"status\": \"success\", "
                "\"message\": \"删除请求已提交到用户线程\", "
                "\"task_id\": \"" + taskId + "\", "
                "\"user_id\": " + std::to_string(session->userId) + "}";
        } else {
            res.status_code = 500;
            res.body = "{\"status\": \"error\", \"message\": \"用户线程不可用\"}";
        }
        return res;
    });

                    // 启动 HTTP 服务器，开始监听并处理请求
    std::cout << "Server is running on port " << Port << " in child process" << std::endl;
    g_server->Start(false);  // false = 不阻塞调用方

    // 服务器关闭后，清理专用数据库连接和 HTTP 服务器实例
    if (g_server) {
        delete g_server;
        g_server = nullptr;
    }
    if (db) {
        delete db;
        db = nullptr;
    }
}

//初始化函数

//初始化并启动 HTTP 服务器
//Windows 下创建线程，Linux 下 fork 子进程，不阻塞主进程
void Initiate_Http(int Port) {
#ifdef _WIN32
    std::thread serverThread(Http_Server_Routine, Port);  // Windows：创建线程
    serverThread.detach();                               // 分离线程，后台运行
    Tools::Out_System("HTTP 服务器线程已启动 (端口: " + std::to_string(Port) + ")");
#else
    pid_t pid = fork();                                  // Linux：frok 子进程
    if (pid == 0) {                                      // 子进程
        Http_Server_Routine(Port);          ·            // 启动 HTTP 服务器
        exit(0);                                         // 子进程退出
    } else if (pid > 0) {                                // 父进程
        Tools::Out_System("HTTP 服务器在子进程中运行 (PID: " + std::to_string(pid) +
                          ", 端口: " + std::to_string(Port) + ")");
    } else {                                             // fork 失败
        Tools::Out_System_Error("创建子进程失败");
    }
#endif
}

//初始化 MySQL 数据库连接
//返回数据库连接对象指针，失败返回 nullptr
MySQL::mysql* Initiate_MySQL() {
    try {
        return new MySQL::mysql();                       // 创建并返回数据库连接对象
    } catch (const std::exception& e) {
        Tools::Out_System_Error("MySQL 初始化失败: " + std::string(e.what()));
        return nullptr;                                  // 初始化失败返回空指针
    } catch (...) {
        Tools::Out_System_Error("MySQL 初始化失败: 未知错误");
        return nullptr;
    }
}