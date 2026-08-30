cmake_minimum_required(VERSION 3.16)
project(liquid_kit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# 使用方（如 examples/minimal）未通过 find_package 引入 Qt 时，为方便开发默认查找。
# 正式项目应在 add_subdirectory 前自行 find_package(Qt6 ...) 或改用 find_package 下发的配置包。
if(NOT TARGET Qt6::Widgets)
    find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL)
endif()

# Google material-color-utilities（Material You / Monet）官方 C++ 实现
file(GLOB MCU_SOURCES CONFIGURE_DEPENDS
    third_party/material_color_utilities/cpp/*/*.cc
)

set(LIQUID_KIT_SOURCES
    src/md3_theme.cpp
    src/md3_button.cpp
    src/md3_card.cpp
    src/md3_switch.cpp
    src/md3_text_field.cpp
    src/md3_progress_bar.cpp
    src/md3_slider.cpp
    src/md3_dropdown.cpp
    src/md3_side_bar.cpp
    src/monet_theme.cpp
    src/liquid_glass.cpp
    src/liquid_glass_widgets.cpp
    ${MCU_SOURCES}
)

add_library(liquid_kit STATIC ${LIQUID_KIT_SOURCES})

target_include_directories(liquid_kit
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/material_color_utilities
)

target_link_libraries(liquid_kit PUBLIC Qt6::Widgets Qt6::OpenGL)

# 示例：默认构建（install 时或 DM 关闭）
option(LIQUID_KIT_BUILD_EXAMPLES "构建 liquid_kit 示例程序" ON)
if(LIQUID_KIT_BUILD_EXAMPLES)
    add_executable(liquid_kit_minimal examples/minimal.cpp)
    target_link_libraries(liquid_kit_minimal PRIVATE liquid_kit)
endif()

install(TARGETS liquid_kit
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
