//MyMySQL.h工具实现文件

#include "MyMySQL.h"

namespace MySQL{
    //构造函数
    mysql::mysql(){
        conn = mysql_init(nullptr);                     // 分配并初始化 MYSQL 句柄
        database = "web_server";                        // 默认数据库名
        if(Connect() || Create_Socket()){               // 连接 MySQL 服务器并检查句柄有效性
            throw std::invalid_argument("key error");   // 构造函数未通过，抛异常"key error"
        }
        Set_Databases(database);                        // 切换到默认数据库
    }

    mysql::mysql(std::string str):conn (mysql_init(nullptr)){
        conn = mysql_init(nullptr);                     // 分配 MYSQL 句柄
        Set_Databases(str);                             // 设置数据库名
        if(Connect() || Create_Socket()){               // 连接 MySQL 服务器并检查句柄有效性
            throw std::invalid_argument("key error");   // 构造函数未通过，抛异常"key error"
        }
    }
    
    //内部工具
    bool mysql::Create_Socket(){                            // 检查 MYSQL 句柄是否创建成功
        if (conn == nullptr) {                              // 检查conn是否为nullptr
            Tools::Out_System_Error("mysql_init() 失败");   // 输出失败
            return 1;                                       // 失败返回 1
        }
        return 0;                                           // 成功返回 0
    }

    bool mysql::Connect(){                                  // 连接到 MySQL服务器,同步阻塞
        if (mysql_real_connect(conn, host.c_str(), 
        user.c_str(), password.c_str(), 
        nullptr, port, nullptr, 0) == nullptr) {                                                    // 检查conn是否成功
            Tools::Out_System_Error("mysql_real_connect() 失败: " + std::string(mysql_error(conn)));// 输出失败
            mysql_close(conn);                                                                      // 关闭连接
            return 1;                                                                               // 失败返回 1
        }
        Tools::Out_System_Mysql("成功连接到 MySQL 服务器！");                                         // 输出连接成功
        return 0;                                                                                   // 成功返回 0
    }

    bool mysql::Close(){                                    // 关闭数据库连接
        mysql_close(conn);                                  // 释放 MYSQL 句柄和相关资源
        Tools::Out_System_Mysql("连接已关闭");               // 输出关闭连接
        return 0;                                           // 成功返回 0
    }

    bool mysql::Set_Databases(std::string str){             // 切换到指定数据库
        if (mysql_select_db(conn, str.c_str())) {                                       // 出入MySQL语句
            Tools::Out_System_Error("选择数据库失败: " + std::string(mysql_error(conn)));// 输出错误
            return 1;                                                                   // 失败返回 1
        }
        database = str;                                                                 // 设置数据库
        Tools::Out_System_Mysql("已设置数据库: " + database);                            // 输出已经设置
        return 0;                                                                       // 成功返回 0
    }

    bool mysql::Set_UTF8(){                                 // 设置字符集为 utf8
        if (mysql_set_character_set(conn, "utf8mb4") != 0) {                                // 输入MySQL
            Tools::Out_System_Error("设置字符集失败: " + std::string(mysql_error(conn)));    // 输出错误信息
            return 1;                                                                       // 失败返回 1
        }
        Tools::Out_System_Mysql("字符集已设置为 utf8mb4");                                   // 输出设置成功
        return 0;                                                                           // 成功返回 0
    }

    // 外部可使用函数
    // 数据库管理
    bool mysql::Create_DataBases(){                 // 创建数据库（无参调用）
       return Create_DataBases(database);       // 使用当前默认数据库名
    }

    bool mysql::Create_DataBases(std::string str){  // 创建数据库有参调用
        std::string sql = "CREATE DATABASE IF NOT EXISTS " + str;                           // 创建、存储MySQL语句(没有数据库则创建数据库)
        if (mysql_query(conn, sql.c_str())) {                                               // 输入MySQL语句
            Tools::Out_System_Error("创建数据库失败: " + std::string(mysql_error(conn)));    // 输出错误信息
            return 1;                                                                       // 失败返回 1
        }
        Tools::Out_System_Mysql("数据库 " + database + " 已就绪");                           // 输出"数据库就绪"
        if(database != str){                                                                // 检验是否为默认数据库
            database = str;                                                                 // 设置成默认数据库
            Tools::Out_System_Mysql("数据库"+database+"已经设置为默认数据库");                // 输出设置后的默认数据库
        }
        return 0;                                                                           // 运行成功返回 0
    }

    // 用户登录鉴权，查询 users 表匹配用户名密码，返回权限级别
    int mysql::User(const std::string name , const std::string user_password, std::string* userAvatar){
        mysql_query(conn, "SELECT DATABASE();");                                // 查当前数据库（调试用）
        MYSQL_RES* dbRes = mysql_store_result(conn);                            // 获取结果集
        if (dbRes) {                                                            // 检查结果集
            MYSQL_ROW dbRow = mysql_fetch_row(dbRes);                           // 取第一行数据
            std::cout << "[MySQL Debug] Current database: " << (dbRow ? dbRow[0] : "NULL") << std::endl;
            mysql_free_result(dbRes);                                           // 释放结果集
        }

        // ⚠️ SQL 注入风险：name 如果包含单引号可被利用
        std::string sql = "select name, password, permission, avatar from users where name = '" + name + "';";// 查询用户SQL
        std::cout << "[MySQL Debug] SQL: " << sql << std::endl;
        if (mysql_query(conn, sql.c_str())) {                                   // 执行查询
            Tools::Out_System_Error("查询失败: " + std::string(mysql_error(conn)));// 输出错误
            return -1;                                                          // 数据库错误
        }
        MYSQL_RES* result = mysql_store_result(conn);                           // 获取结果集

        if(!result){                                                            // 检查结果集
            Tools::Out_System_Error("获取结果失败: " + std::string(mysql_error(conn)));// 输出错误
            return -1;                                                          // 数据库错误
        }

        MYSQL_ROW row = mysql_fetch_row(result);                                // 取第一行数据
        if(!row){                                                               // 检查行
            Tools::Out_System_Mysql("未找到用户");                               // 输出未找到
            mysql_free_result(result);                                          // 释放结果集
            return 0;                                                           // 用户不存在
        }

        // 解析结果行：row[0]=name, row[1]=password, row[2]=permission, row[3]=avatar
        std::string db_password = row[1] ? row[1] : "";                         // 获取密码
        int db_permission = 0;                                                  // 初始化权限
        if (row[2]) {                                                           // 获取权限
            db_permission = std::stoi(row[2]);
        }

        if (db_password == user_password) {                                     // 比对密码
            Tools::Out_System_Mysql("欢迎用户 " + name + "，权限: " + std::to_string(db_permission));// 登录成功
            if (userAvatar != nullptr && row[3]) {                              // 检查头像指针
                *userAvatar = row[3];                                           // 赋值头像
            }
            mysql_free_result(result);                                          // 释放结果集
            return db_permission;                                               // 返回权限级别
        } 
        else {
            Tools::Out_System_Mysql("密码错误");                                // 输出错误
            mysql_free_result(result);                                          // 释放结果集
            return 0;                                                           // 密码错误
        }
    }

    // 根据用户名查询用户 ID
    int mysql::Get_UserId(std::string name){
        std::string sql = "select id from users where name = '" + name + "';";// 查询用户IDSql
        if (mysql_query(conn, sql.c_str())) {                                   // 执行查询
            Tools::Out_System_Error("查询用户ID失败: " + std::string(mysql_error(conn)));// 输出错误
            return -1;                                                          // 数据库错误
        }
        MYSQL_RES* result = mysql_store_result(conn);                           // 获取结果集
        if(!result){                                                            // 检查结果集
            Tools::Out_System_Error("获取结果失败: " + std::string(mysql_error(conn)));// 输出错误
            return -1;                                                          // 数据库错误
        }
        MYSQL_ROW row = mysql_fetch_row(result);                                // 取第一行数据
        if(!row){                                                               // 检查行
            Tools::Out_System_Mysql("未找到用户");                               // 输出未找到
            mysql_free_result(result);                                          // 释放结果集
            return 0;                                                           // 用户不存在
        }
        int user_id = row[0] ? std::stoi(row[0]) : 0;                           // 获取用户ID
        mysql_free_result(result);                                              // 释放结果集
        return user_id;                                                         // 返回用户ID
    }

    // 根据用户名查询用户头像
    std::string mysql::Get_Avatar(std::string name){
        std::string sql = "select avatar from users where name = '" + name + "';";// 查询头像Sql
        if (mysql_query(conn, sql.c_str())) {                                   // 执行查询
            Tools::Out_System_Error("查询头像失败: " + std::string(mysql_error(conn)));// 输出错误
            return "";                                                          // 返回空
        }
        MYSQL_RES* result = mysql_store_result(conn);                           // 获取结果集
        if(!result){                                                            // 检查结果集
            Tools::Out_System_Error("获取头像结果失败: " + std::string(mysql_error(conn)));// 输出错误
            return "";                                                          // 返回空
        }
        MYSQL_ROW row = mysql_fetch_row(result);                                // 取第一行数据
        if(!row){                                                               // 检查行
            mysql_free_result(result);                                          // 释放结果集
            return "";                                                          // 返回空
        }
        std::string avatar = row[0] ? row[0] : "";                              // 获取头像
        mysql_free_result(result);                                              // 释放结果集
        return avatar;                                                          // 返回头像
    }

    // 析构函数：自动关闭连接
    mysql::~mysql(){
        Close();                                                                // 关闭连接
    }

    // 全局函数：创建数据库（使用已有连接句柄）
    bool Create_DataBases(MYSQL* conn, std::string str){
        std::string sql = "CREATE DATABASE IF NOT EXISTS " + str;               // 创建数据库SQL语句
        if (mysql_query(conn, sql.c_str())) {                                   // 执行查询
            Tools::Out_System_Error("创建数据库失败: " + std::string(mysql_error(conn)));// 输出错误
            return 1;                                                           // 失败返回 1
        }
        return 0;                                                               // 成功返回 0
    }
}