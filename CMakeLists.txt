cmake_minimum_required(VERSION 3.16)
project(blue_archive_ui LANGUAGES CXX)

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

set(BLUE_ARCHIVE_UI_SOURCES
    src/md3_theme.cpp
    src/md3_icon.cpp
    src/md3_button.cpp
    src/md3_card.cpp
    src/md3_switch.cpp
    src/md3_text_field.cpp
    src/md3_progress_bar.cpp
    src/md3_slider.cpp
    src/md3_dropdown.cpp
    src/md3_side_bar.cpp
    src/md3_fab.cpp
    src/md3_badge.cpp
    src/md3_chip.cpp
    src/md3_check_box.cpp
    src/md3_radio_button.cpp
    src/md3_segmented_button.cpp
    src/md3_dialog.cpp
    src/monet_theme.cpp
    src/liquid_glass.cpp
    src/liquid_glass_widgets.cpp
    src/ba_style.cpp
    src/ba_icon.cpp
    src/ba_button.cpp
    src/ba_slider.cpp
    src/ba_check_box.cpp
    src/ba_progress_bar.cpp
    src/ba_section_header.cpp
    src/ba_top_bar.cpp
    src/ba_tab_column.cpp
    src/ba_dialog.cpp
    src/ba_background.cpp
    src/ba_panel.cpp
    src/ba_chip.cpp
    src/ba_assets.cpp
    src/ba_navigation_bar.cpp
    src/ba_page_tabs.cpp
    src/ba_bounty_card.cpp
    src/ba_shop_menu_block.cpp
    src/ba_voice_bubble.cpp
    src/ba_mission_card.cpp
    src/ba_info_card.cpp
    src/ba_player_card.cpp
    src/ba_card.cpp
    src/ba_spark.cpp
    ${MCU_SOURCES}
)

# 公开头文件（include/blue_archive_ui/）纳入 target 以便 AUTOMOC 生成 moc/vtable
file(GLOB BLUE_ARCHIVE_UI_HEADERS CONFIGURE_DEPENDS
    include/blue_archive_ui/*.h
)

add_library(blue_archive_ui STATIC ${BLUE_ARCHIVE_UI_SOURCES} ${BLUE_ARCHIVE_UI_HEADERS})

target_include_directories(blue_archive_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/blue_archive_ui
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/material_color_utilities
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(blue_archive_ui PUBLIC Qt6::Widgets Qt6::OpenGL)

# 示例：默认构建（install 时或 DM 关闭）
option(BLUE_ARCHIVE_UI_BUILD_EXAMPLES "构建 blue_archive_ui 示例程序" ON)
if(BLUE_ARCHIVE_UI_BUILD_EXAMPLES)
    add_executable(blue_archive_ui_minimal examples/minimal.cpp)
    target_link_libraries(blue_archive_ui_minimal PRIVATE blue_archive_ui)

    add_executable(blue_archive_ui_ba_demo examples/ba_demo.cpp)
    target_link_libraries(blue_archive_ui_ba_demo PRIVATE blue_archive_ui)

    add_executable(blue_archive_ui_button_demo examples/ba_button_demo.cpp)
    target_link_libraries(blue_archive_ui_button_demo PRIVATE blue_archive_ui)

    add_executable(blue_archive_ui_progress_demo examples/ba_progress_demo.cpp)
    target_link_libraries(blue_archive_ui_progress_demo PRIVATE blue_archive_ui)

    add_executable(blue_archive_ui_info_demo examples/ba_info_card_demo.cpp)
    target_link_libraries(blue_archive_ui_info_demo PRIVATE blue_archive_ui)

    add_executable(blue_archive_ui_card_demo examples/ba_card_demo.cpp)
    target_link_libraries(blue_archive_ui_card_demo PRIVATE blue_archive_ui)

    add_executable(blue_archive_ui_slider_demo examples/ba_slider_demo.cpp)
    target_link_libraries(blue_archive_ui_slider_demo PRIVATE blue_archive_ui)

    add_executable(blue_archive_ui_options_demo examples/ba_options_demo.cpp)
    target_link_libraries(blue_archive_ui_options_demo PRIVATE blue_archive_ui)

    add_executable(blue_archive_ui_all_demo examples/ba_all_demo.cpp)
    target_link_libraries(blue_archive_ui_all_demo PRIVATE blue_archive_ui)

    add_executable(blue_archive_ui_spark_demo examples/ba_spark_demo.cpp)
    target_link_libraries(blue_archive_ui_spark_demo PRIVATE blue_archive_ui)

    add_executable(blue_archive_ui_topbar_demo examples/ba_top_bar_demo.cpp)
    target_link_libraries(blue_archive_ui_topbar_demo PRIVATE blue_archive_ui)

    add_executable(blue_archive_ui_integrated_demo examples/ba_integrated_demo.cpp)
    target_link_libraries(blue_archive_ui_integrated_demo PRIVATE blue_archive_ui)
endif()

install(TARGETS blue_archive_ui
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
