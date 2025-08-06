cmake_minimum_required(VERSION 3.24.0)

# 限制 c++ 20 以上
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

# 简易的事件管理器
project(xd-event-mgr)

# 添加源代码
set(ALL_SRC
  # include (方便代码提示)
  ./include/staticEventMgr.hpp

  # uuid (方便代码提示)
  ./3rd/uuid.h

  # obj
  uuidGen.cpp
  staticEventMgr.cpp
)

# 包含目录
include_directories(./ ./include)
add_library(xd-event-mgr STATIC ${ALL_SRC})
target_include_directories(xd-event-mgr INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
