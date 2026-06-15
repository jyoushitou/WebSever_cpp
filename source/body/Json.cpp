#include "Json.h"

std::string ParseJsonString(const std::string& body, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";             // 构造 "\"key\""
    size_t keyPos = body.find(searchKey);                  // 找到 key 位置
    if (keyPos == std::string::npos) return "";
    size_t colonPos = body.find(":", keyPos);              // 找到冒号
    if (colonPos == std::string::npos) return "";
    size_t quoteStart = body.find("\"", colonPos);         // 找到值起始引号
    if (quoteStart == std::string::npos) return "";
    size_t quoteEnd = body.find("\"", quoteStart + 1);     // 找到值结束引号
    if (quoteEnd == std::string::npos) return "";
    return body.substr(quoteStart + 1, quoteEnd - quoteStart - 1);  // 截取中间的字符串
}

std::string ParseJsonValueRaw(const std::string& body, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = body.find(searchKey);
    if (keyPos == std::string::npos) return "";
    size_t colonPos = body.find(":", keyPos + searchKey.length());  // 从 key 之后找冒号
    if (colonPos == std::string::npos) return "";
    size_t valueStart = body.find_first_not_of(" \t\n\r", colonPos + 1);  // 跳过空白
    if (valueStart == std::string::npos) return "";
    if (body[valueStart] == '"') {                             // 字符串值：截取引号内容
        size_t valueEnd = body.find("\"", valueStart + 1);
        if (valueEnd == std::string::npos) return "";
        return body.substr(valueStart + 1, valueEnd - valueStart - 1);
    } else {                                                   // 数字/布尔值：截取到逗号、花括号或换行
        size_t valueEnd = body.find_first_of(",}\n\r", valueStart);
        if (valueEnd == std::string::npos) valueEnd = body.length();
        std::string raw = body.substr(valueStart, valueEnd - valueStart);
        // 去除尾部空白
        while (!raw.empty() && (raw.back() == ' ' || raw.back() == '\t')) {
            raw.pop_back();
        }
        return raw;
    }
}