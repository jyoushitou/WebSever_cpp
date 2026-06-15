// ============================================================
// MyMySQL.h — MySQL 数据库操作封装头文件
// 功能：封装 MySQL C API，提供连接管理、用户鉴权、数据库创建等
// ============================================================

#ifndef MYMYSQL_H
#define MYMYSQL_H
#include <iostream>
#include <string>           // C++ 字符串类

#include "Tools.h"          // 工具库（用于日志输出）

#include <mysql.h>          // MySQL C API 头文件（需安装 libmysqlclient-dev）

// ============================================================
// MySQL 命名空间 — 封装所有数据库相关操作
//
// 设计思路：
// 使用 MySQL C API（mysql_init / mysql_real_connect / mysql_query 等）
// 而不是 C++ connector，更接近底层，便于理解数据库连接的工作原理。
// ======================================================
namespace MySQL{
    // 全局函数：检查并创建数据库（接受外部的 MYSQL* 句柄）
    bool Create_DataBases(MYSQL* ,std::string);

    // ===== mysql 类 — 数据库连接和操作封装 =====
    // 封装了一个完整的 MySQL 连接生命周期：
    // 构造 → 创建句柄 → 连接服务器 → 选择数据库 → 操作 → 析构断开
        class mysql{
        public:
        mysql();                                             // 默认构造函数：连接 "web_server" 数据库
        mysql(std::string str);                              // 指定数据库名的构造函数

        MYSQL* conn;                                         // MySQL 连接句柄（核心资源）

        // 数据库管理
        bool Create_DataBases();                             // 创建/验证当前默认数据库
        bool Create_DataBases(std::string);                  // 创建/验证指定数据库并设为默认

                // 用户鉴权
        int User(std::string, std::string, std::string* userAvatar = nullptr);  // 用户名+密码鉴权，返回权限级别（0=无权限，>0=有权限），可选输出头像
        int Get_UserId(std::string);                         // 根据用户名查询用户 ID
        std::string Get_Avatar(std::string);                 // 根据用户名查询用户头像

        ~mysql();                                            // 析构时自动关闭连接

                private:
        // ===== MySQL 连接参数 =====
        const std::string host = "localhost";                // 数据库服务器地址（本地）
        const int port = 3306;                               // MySQL 默认端口
        const std::string user = "web_server";               // 数据库用户名（需提前创建）
        const std::string password = "123456";               // 数据库密码
        std::string database;                                // 当前选中的数据库名称

        // ===== 私有工具方法 =====
        bool Create_Socket();                                // 检查句柄是否创建成功
        bool Connect();                                      // 连接到 MySQL 服务器
        bool Close();                                        // 关闭数据库连接

        bool Set_UTF8();                                     // 设置字符集为 utf8mb4（支持中文和 emoji）
        bool Set_databases(std::string);                     // 切换到指定数据库

    };
}
#endif //!MYMYSQL_H