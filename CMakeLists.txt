cmake_minimum_required(VERSION 3.16)
project(CS171-hw2 LANGUAGES C CXX)

option(USE_FILESYSTEM "use std::filesystem" off)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# glfw
option(GLFW_BUILD_TESTS off)
option(GLFW_BUILD_EXAMPLES off)
option(GLFW_BUILD_DOCS off)
add_subdirectory(libs/glfw)
include_directories(libs/glfw/include)
list(APPEND LIBRARIES glfw ${glfw_LIBRARIES})

# glew
option(BUILD_UTILS off)
add_subdirectory(libs/glew/build/cmake)
include_directories(libs/glew/include)
list(APPEND LIBRARIES glew_s)

# glm
add_subdirectory(libs/glm)
list(APPEND LIBRARIES glm::glm)

# 1. 获取 imgui 源文件
file(GLOB IMGUI_SRC_FILES imgui/*.cpp)

# 2. 获取 src 下的所有 cpp 文件
file(GLOB ALL_SRC_FILES src/*.cpp)

# 3. 定义你的三个入口文件路径 (使用绝对路径以确保 list REMOVE_ITEM 能正确匹配)
set(MAIN_2D_BAKING "${CMAKE_CURRENT_SOURCE_DIR}/src/main_2d_baking.cpp")
set(MAIN_2D        "${CMAKE_CURRENT_SOURCE_DIR}/src/main_2d.cpp")
set(MAIN_3D        "${CMAKE_CURRENT_SOURCE_DIR}/src/main_3d.cpp")

# 4. 从总列表中移除这三个入口文件，剩下的就是公共源文件(Shared Sources)
#    (假设 src 下还有其他文件是这三个程序公用的，比如 utils.cpp, mesh.cpp 等)
set(SHARED_SRC_FILES ${ALL_SRC_FILES})
list(REMOVE_ITEM SHARED_SRC_FILES 
    ${MAIN_2D_BAKING}
    ${MAIN_2D}
    ${MAIN_3D}
)

# 5. 定义一个辅助宏，用来配置通用的编译选项 (避免写三遍重复代码)
macro(setup_target target_name source_file)
    add_executable(${target_name} 
        ${source_file}          # 特定的 main 文件
        ${SHARED_SRC_FILES}     # 公共源文件
        ${IMGUI_SRC_FILES}      # ImGui
        ${INTERNAL_SRC_FILES}
    )

    target_include_directories(${target_name} PRIVATE
        include
        ${CMAKE_SOURCE_DIR}/imgui
    )
    target_compile_features(${target_name} PRIVATE cxx_std_17)
    target_compile_definitions(${target_name} PRIVATE ${DEFINES})
    target_link_libraries(${target_name} PRIVATE ${LIBRARIES})
endmacro()

# 6. 分别创建三个可执行文件
setup_target(main_2d        ${MAIN_2D})
setup_target(main_3d        ${MAIN_3D})
