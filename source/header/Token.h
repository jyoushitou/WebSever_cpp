#include <iostream>     // 输入输出流操作
#include <random>        // 随机数生成（用于生成 Token）
#include <string>        // 字符串
#include <iomanip>       // 输入输出格式化（如十六进制输出）
#include <mutex>         // 互斥锁，保证 Token 存储的线程安全

#include "https_api.h"   // HTTP 请求/响应类型与工具函数

// 设备信息，记录用户登录时的设备详情
struct DeviceInfo {
    std::string deviceName;   // 自定义设备名（前端传入）
    std::string userAgent;    // User-Agent 请求头
    std::string clientIp;     // 客户端 IP 地址
    std::time_t loginTime;    // 登录时间戳
};

// 会话信息，存储已认证用户的会话数据
struct SessionInfo {
    int userId;               // 用户 ID
    int level;                // 用户权限等级
    std::string name;         // 用户显示名称
    DeviceInfo device;        // 当前会话的设备信息
};

// 全局 Token 存储表：Token 字符串 -> 对应的会话信息
extern std::unordered_map<std::string, SessionInfo> g_tokenStore;
// 保护 g_tokenStore 并发访问的互斥锁
extern std::mutex g_tokenMutex;

// 生成一个新的安全认证 Token
std::string GenerateToken();

// 从 HTTP 请求中提取 Token 字符串（如从 Authorization 请求头中提取）
std::string GetTokenFromRequest(const Http::HttpRequest&);

// 验证 Token 并返回对应的 SessionInfo；
// 如果 Token 不存在或无效则返回 nullptr
SessionInfo* ValidateToken(const std::string&);