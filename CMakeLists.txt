cmake_minimum_required(VERSION 3.16)

project(math-ai-review-backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

file(GLOB_RECURSE PROJECT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/clients/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/controllers/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/dao/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/filters/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/services/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/models/*.cc"
)

add_executable(${PROJECT_NAME}
    main.cc
    ${PROJECT_SOURCES}
    mysql_async_wrapper.cc
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${MYSQL_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Drogon::Drogon
        OpenSSL::Crypto
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

if(MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            ws2_32
            secur32
            crypt32
            cryptui
            ncrypt
            bcrypt
            zstd
            curl
            shlwapi
            mswsock
            ssp
    )
endif()
