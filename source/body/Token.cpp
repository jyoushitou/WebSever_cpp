#include "Token.h"

// 全局变量定义（唯一实例）
std::unordered_map<std::string, SessionInfo> g_tokenStore;
std::mutex g_tokenMutex;

std::string GenerateToken() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

std::string GetTokenFromRequest(const Http::HttpRequest& req) {
    auto authIt = req.headers.find("Authorization");
    if (authIt == req.headers.end()) {
        authIt = req.headers.find("authorization");
    }
    if (authIt == req.headers.end()) return "";

    std::string auth = authIt->second;
    size_t bearerPos = auth.find("Bearer ");
    if (bearerPos == std::string::npos) {
        bearerPos = auth.find("bearer ");
    }
    if (bearerPos == std::string::npos) return auth;

    return auth.substr(bearerPos + 7);
}

SessionInfo* ValidateToken(const std::string& token) {
    if (token.empty()) return nullptr;
    std::lock_guard<std::mutex> lock(g_tokenMutex);
    auto it = g_tokenStore.find(token);
    if (it == g_tokenStore.end()) return nullptr;
    return &it->second;
}