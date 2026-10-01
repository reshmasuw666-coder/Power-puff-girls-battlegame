cmake_minimum_required(VERSION 3.16)
project(ppg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(SFML 2.6 COMPONENTS graphics window system audio REQUIRED)

add_executable(ppg
    main.cpp
    GameWindow.cpp GameWindow.h
    GameManager.cpp GameManager.h
    Character.cpp Character.h
)

target_link_libraries(ppg PRIVATE Qt6::Widgets sfml-graphics sfml-window sfml-system sfml-audio)

# Windows: copy the SFML + OpenAL DLLs next to ppg.exe after every build
if(WIN32)
    get_filename_component(SFML_ROOT "${SFML_DIR}/../../.." ABSOLUTE)
    file(GLOB SFML_DLLS "${SFML_ROOT}/bin/*.dll")
    if(SFML_DLLS)
        add_custom_command(TARGET ppg POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SFML_DLLS} $<TARGET_FILE_DIR:ppg>)
    endif()
endif()
