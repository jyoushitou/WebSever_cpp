# C++ 后端服务

> **基于原生 Socket 的高性能 HTTP 服务器**，不依赖任何第三方 Web 框架。

## 项目结构

```
cpp/
├── CMakeLists.txt                 # CMake 构建配置（C++17）
└── source/
    ├── header/                    # 头文件
    │   ├── FarmeWork.h            # 核心框架：全局变量、结构体、函数声明
    │   ├── https_api.h            # HTTP 服务器：请求/响应、路由、Socket
    │   ├── MyMySQL.h              # MySQL 数据库操作封装
    │   ├── Task.h                 # 任务系统：任务类型/状态/线程管理
    │   └── Tools.h                # 通用工具：日志、MIME、文件操作 + Tools::Json + Tools::Auth
    └── body/                      # 源文件
        ├── main.cpp               # 程序入口
        ├── FrameWork.cpp          # 核心业务（路由、任务系统、用户线程）
        ├── https_api.cpp          # HTTP 服务器实现
        ├── mysql.cpp              # MySQL 连接与操作
        ├── Task.cpp               # 任务系统实现
        └── Tools.cpp              # 工具函数 + Tools::Json + Tools::Auth 实现
```

## 构建与运行

```bash
cd cpp
mkdir build && cd build
cmake ..
cmake --build .

./WebServer
```

## 命名空间结构

| 命名空间 | 所在文件 | 说明 |
|----------|----------|------|
| `Tools` | `Tools.h/.cpp` | 日志输出、文件读取、MIME 类型 |
| `Tools::Json` | `Tools.h/.cpp` | JSON 字符串解析（无第三方依赖） |
| `Tools::Auth` | `Tools.h/.cpp` | Token 生成/提取/验证、会话管理 |
| `TaskSystem` | `Task.h/.cpp` | 任务队列、用户线程调度 |
| `MySQL` | `MyMySQL.h / mysql.cpp` | 数据库连接与用户鉴权 |
| `Http` | `https_api.h/.cpp` | HTTP 服务器、路由、请求/响应 |

## 核心设计：每用户独立线程

```
用户登录 → Create_User_Thread() → 专属线程
                                    ├── 独立 MySQL 连接
                                    ├── 任务队列
                                    └── 异步处理任务
```

## HTTP 请求处理流程

```
socket() → bind() → listen() → accept() 循环
  └── Handle_Client()
        ├── Request_Parse()     → 手动解析 HTTP 协议
        ├── Default_Request_Handle() → 路由 + 静态文件
        └── Response_Build()    → 构建 HTTP 响应
```

## 数据库配置

默认连接配置在 `mysql.cpp` 中：

| 参数 | 默认值 |
|------|--------|
| host | localhost |
| port | 3306 |
| user | web_server |
| password | 123456 |
| database | web_server |
