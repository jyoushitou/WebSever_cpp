// ============================================================
// mysql.cpp — MySQL 数据库操作实现
// 功能：数据库连接管理、用户鉴权、数据库创建
// ===========================================================

#include "../header/MyMySQL.h"

namespace MySQL{
    // ==================== 构造函数 ====================
    
    // 默认构造函数：连接 "web_server" 数据库
    // 流程：mysql_init → mysql_real_connect → mysql_select_db
    mysql::mysql(){
        conn = mysql_init(nullptr);             // 1. 分配并初始化 MYSQL 句柄
        database = "web_server";                // 默认数据库名
        // 连接 MySQL 服务器并检查句柄有效性
        if(Connect() || Create_Socket()){       // 注意：这里逻辑有误，应先 Create_Socket 再 Connect
            throw std::invalid_argument("key error");
        }
        Set_databases(database);                // 切换到默认数据库
    }

    // 指定数据库名的构造函数
    mysql::mysql(std::string str):conn (mysql_init(nullptr)){
        conn = mysql_init(nullptr);             // 分配 MYSQL 句柄
        Set_databases(str);                     // 设置数据库名
        if(Create_Socket()||Connect()){         // 检查句柄并连接
            throw std::invalid_argument("key error");
        }
    }

    // ==================== 工具方法 ====================

    // 检查 MYSQL 句柄是否创建成功
    // ★ 函数名 Create_Socket 有误导性 — 它只是检查 null，不创建任何东西
    bool mysql::Create_Socket(){
        if (conn == nullptr) {
            Tools::Out_System_Error("mysql_init() 失败");
            return 1;                           // 返回 1 表示失败
        }
        return 0;                               // 返回 0 表示成功
    }

    // 创建数据库（如果不存在）
    // 使用 "CREATE DATABASE IF NOT EXISTS" SQL 语句
    bool mysql::Create_DataBases(){
       return Create_DataBases(database);       // 使用当前默认数据库名
    }

    bool mysql::Create_DataBases(std::string str){
        std::string sql = "CREATE DATABASE IF NOT EXISTS " + str;
        if (mysql_query(conn, sql.c_str())) {
            Tools::Out_System_Error("创建数据库失败: " + std::string(mysql_error(conn)));
            return 1;
        }
        Tools::Out_System_Mysql("数据库 " + database + " 已就绪");
        if(database != str){
            database = str;
            Tools::Out_System_Mysql("数据库"+database+"已经设置为默认数据库");
        }
        return 0;
    }

    // 连接到 MySQL 服务器
    // mysql_real_connect 是同步阻塞的
    bool mysql::Connect(){
        if (mysql_real_connect(conn, host.c_str(), 
        user.c_str(), password.c_str(), 
        nullptr, port, nullptr, 0) == nullptr) {
            Tools::Out_System_Error("mysql_real_connect() 失败: " + std::string(mysql_error(conn)));
            mysql_close(conn);
            return 1;
        }
        Tools::Out_System_Mysql("成功连接到 MySQL 服务器！");
        return 0;
    }

    // 切换到指定数据库
    // mysql_select_db 相当于执行 "USE database_name"
    bool mysql::Set_databases(std::string str){
        if (mysql_select_db(conn, str.c_str())) {
            Tools::Out_System_Error("选择数据库失败: " + std::string(mysql_error(conn)));
            return 1;
        }
        database = str;
        Tools::Out_System_Mysql("已设置数据库: " + database);
        return 0;
    }

    // 设置字符集为 utf8mb4
    // utf8mb4 支持完整的 Unicode（包括 emoji），优于 MySQL 的 utf8mb3
    bool mysql::Set_UTF8(){
        if (mysql_set_character_set(conn, "utf8mb4") != 0) {
            Tools::Out_System_Error("设置字符集失败: " + std::string(mysql_error(conn)));
            return 1;
        }
        Tools::Out_System_Mysql("字符集已设置为 utf8mb4");
        return 0;
    }

    // ==================== 用户鉴权 ====================

    // 用户登录鉴权
    // 查询 users 表，匹配用户名和密码，返回权限级别
    // 参数：name=用户名, user_password=密码
    // 返回值：>0=权限级别（登录成功），0=密码错误/用户不存在，-1=数据库错误
    // 
    // ⚠️ 安全注意：这里直接拼接 SQL 字符串，存在 SQL 注入风险！
    // 生产环境应使用参数化查询（Prepared Statement）
    int mysql::User(const std::string name , const std::string user_password, std::string* userAvatar){
        // 查当前数据库（调试用）
        mysql_query(conn, "SELECT DATABASE();");
        MYSQL_RES* dbRes = mysql_store_result(conn);
        if (dbRes) {
            MYSQL_ROW dbRow = mysql_fetch_row(dbRes);
            std::cout << "[MySQL Debug] Current database: " << (dbRow ? dbRow[0] : "NULL") << std::endl;
            mysql_free_result(dbRes);
        }

                // 查询用户信息
        // ⚠️ SQL 注入风险：name 如果包含单引号可被利用
        std::string sql = "select name, password, permission, avatar from users where name = '" + name + "';";
        std::cout << "[MySQL Debug] SQL: " << sql << std::endl;
        if (mysql_query(conn, sql.c_str())) {
            Tools::Out_System_Error("查询失败: " + std::string(mysql_error(conn)));
            return -1;                          // 数据库错误
        }
        MYSQL_RES* result = mysql_store_result(conn);  // 获取结果集

        if(!result){
            Tools::Out_System_Error("获取结果失败: " + std::string(mysql_error(conn)));
            return -1;                          // 数据库错误
        }

        MYSQL_ROW row = mysql_fetch_row(result);  // 取第一行数据
        if(!row){
            Tools::Out_System_Mysql("未找到用户");
            mysql_free_result(result);
            return 0;                           // 用户不存在
        }

                // 解析结果行：row[0]=name, row[1]=password, row[2]=permission, row[3]=avatar
        std::string db_password = row[1] ? row[1] : "";
        int db_permission = 0;
        if (row[2]) {
            db_permission = std::stoi(row[2]);
        }

        // 密码匹配 → 登录成功
        if (db_password == user_password) {
            Tools::Out_System_Mysql("欢迎用户 " + name + "，权限: " + std::to_string(db_permission));
            // 如果 userAvatar 指针不为空，填入查询到的头像
            if (userAvatar != nullptr && row[3]) {
                *userAvatar = row[3];
            }
            mysql_free_result(result);
            return db_permission;               // 返回权限级别
        } else {
            Tools::Out_System_Mysql("密码错误");
            mysql_free_result(result);
            return 0;                           // 密码错误
        }
    }

    // 关闭数据库连接
    bool mysql::Close(){
        mysql_close(conn);                      // 释放 MYSQL 句柄和相关资源
        Tools::Out_System_Mysql("连接已关闭");
        return 0;
    }

        // 根据用户名查询用户 ID
    int mysql::Get_UserId(std::string name){
        std::string sql = "select id from users where name = '" + name + "';";
        if (mysql_query(conn, sql.c_str())) {
            Tools::Out_System_Error("查询用户ID失败: " + std::string(mysql_error(conn)));
            return -1;
        }
        MYSQL_RES* result = mysql_store_result(conn);
        if(!result){
            Tools::Out_System_Error("获取结果失败: " + std::string(mysql_error(conn)));
            return -1;
        }
        MYSQL_ROW row = mysql_fetch_row(result);
        if(!row){
            Tools::Out_System_Mysql("未找到用户");
            mysql_free_result(result);
            return 0;
        }
        int user_id = row[0] ? std::stoi(row[0]) : 0;
        mysql_free_result(result);              // 必须释放结果集，否则内存泄漏
        return user_id;
    }

    // 根据用户名查询用户头像
    std::string mysql::Get_Avatar(std::string name){
        std::string sql = "select avatar from users where name = '" + name + "';";
        if (mysql_query(conn, sql.c_str())) {
            Tools::Out_System_Error("查询头像失败: " + std::string(mysql_error(conn)));
            return "";
        }
        MYSQL_RES* result = mysql_store_result(conn);
        if(!result){
            Tools::Out_System_Error("获取头像结果失败: " + std::string(mysql_error(conn)));
            return "";
        }
        MYSQL_ROW row = mysql_fetch_row(result);
        if(!row){
            mysql_free_result(result);
            return "";
        }
        std::string avatar = row[0] ? row[0] : "";
        mysql_free_result(result);
        return avatar;
    }

    // 析构函数：自动关闭连接
    mysql::~mysql(){
        Close();
    }

    // 全局函数：创建数据库（使用已有连接句柄）
    bool Create_DataBases(MYSQL* conn, std::string str){
        std::string sql = "CREATE DATABASE IF NOT EXISTS " + str;
        if (mysql_query(conn, sql.c_str())) {
            Tools::Out_System_Error("创建数据库失败: " + std::string(mysql_error(conn)));
            return 1;
        }
        return 0;
    }
}