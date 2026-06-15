# ============================================================
# CMakeLists.txt — CMake 构建配置文件
# 功能：配置 C++ 编译选项、查找 MySQL 库、链接线程库
# 学习重点：CMake 跨平台构建、库查找、平台条件编译
# ============================================================

cmake_minimum_required(VERSION 3.10)          # 最低 CMake 版本要求

project(WebServer VERSION 1.0 LANGUAGES CXX)  # 项目名、版本号、语言（C++）

set(CMAKE_CXX_STANDARD 17)                    # C++17 标准（支持 filesystem）
set(CMAKE_CXX_STANDARD_REQUIRED ON)           # 强制使用 C++17，不允许降级

# ===== 线程支持 =====
find_package(Threads REQUIRED)                # 查找系统的线程库（pthread 等）

# ==================== MySQL 配置 ====================
# Windows 使用 MySQL Connector/C（libmysql）
# Linux  使用系统的 libmysqlclient（通过 pkg-config 或手动查找）
if(WIN32)
    set(MYSQL_DIR "C:/Program Files/MySQL/MySQL Server 8.0" CACHE PATH "MySQL install dir")
    include_directories("${MYSQL_DIR}/include")
    set(MYSQL_LIB "${MYSQL_DIR}/lib/libmysql.lib")
    set(MYSQL_DLL "${MYSQL_DIR}/lib/libmysql.dll")
else()
    # Linux：通过 pkg-config 查找 MySQL 客户端库
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(MYSQL QUIET mysqlclient)
    endif()
    
    if(NOT MYSQL_FOUND)
        # 备选方案：手动搜索常见路径
        find_path(MYSQL_INCLUDE_DIR
            NAMES mysql.h
            PATHS /usr/include/mysql /usr/local/include/mysql
        )
        find_library(MYSQL_LIBRARY
            NAMES mysqlclient libmysqlclient
            PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
        )
        if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
            set(MYSQL_INCLUDE_DIRS "${MYSQL_INCLUDE_DIR}")
            set(MYSQL_LIBRARIES "${MYSQL_LIBRARY}")
            set(MYSQL_FOUND TRUE)
        endif()
    else()
        set(MYSQL_INCLUDE_DIRS "${MYSQL_INCLUDE_DIRS}")
        set(MYSQL_LIBRARIES "${MYSQL_LIBRARIES}")
    endif()
    
    if(NOT MYSQL_FOUND)
        message(WARNING "MySQL client library not found! Build will likely fail.")
        message(WARNING "Install with: sudo apt install libmysqlclient-dev")
    endif()
    
    include_directories(${MYSQL_INCLUDE_DIRS})
endif()

# ===== 头文件目录 =====
include_directories("source/header")

# ===== 源文件列表 =====
add_executable(${PROJECT_NAME}
    source/body/main.cpp          # 程序入口
    source/body/FrameWork.cpp     # 核心框架（服务器逻辑、鉴权系统、任务系统）
    source/body/mysql.cpp         # MySQL 数据库操作
    source/body/Tools.cpp         # 工具函数
    source/body/https_api.cpp     # HTTP 服务器实现
    source/body/Token.cpp         # Token验证
    source/body/Json.cpp          # Json
)

# ===== 链接库 =====
# 线程库（所有平台都需要）
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

# ===== 平台相关链接 =====
if(WIN32)
    # Windows：MySQL 静态库 + Winsock2 网络库
    target_link_libraries(${PROJECT_NAME} PRIVATE "${MYSQL_LIB}" ws2_32)

    # 构建后自动复制 MySQL DLL 到输出目录（否则运行时找不到）
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MYSQL_DLL}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMENT "Copying libmysql.dll to output directory..."
    )
else()
    # Linux：MySQL 客户端库
    if(MYSQL_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${MYSQL_LIBRARIES})
    endif()

    # Linux 上 pthread 有时需要显式链接
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif()
