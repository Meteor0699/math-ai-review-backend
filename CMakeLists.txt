cmake_minimum_required(VERSION 3.16)

project(math-ai-review-backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

# Recent MySQL/MariaDB clients already provide Drogon's non-blocking API.
# Keep the legacy synchronous shim opt-in for genuinely old client libraries.
option(MATHAI_ENABLE_MYSQL_ASYNC_COMPAT "Enable the legacy MySQL async compatibility shim" OFF)

if(MINGW AND TARGET MySQL_lib)
    find_library(MARIADB_SHARED_IMPORT_LIBRARY NAMES mariadb)
    if(MARIADB_SHARED_IMPORT_LIBRARY)
        set_property(TARGET MySQL_lib PROPERTY IMPORTED_LOCATION "${MARIADB_SHARED_IMPORT_LIBRARY}")
    endif()
endif()

file(GLOB_RECURSE PROJECT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/clients/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/controllers/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/dao/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/filters/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/services/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/models/*.cc"
)

set(PROJECT_ENTRY_SOURCES main.cc ${PROJECT_SOURCES})
if(MATHAI_ENABLE_MYSQL_ASYNC_COMPAT)
    list(APPEND PROJECT_ENTRY_SOURCES mysql_async_wrapper.cc)
endif()

add_executable(${PROJECT_NAME} ${PROJECT_ENTRY_SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${MYSQL_INCLUDE_DIRS}
)

find_package(CURL REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Drogon::Drogon
        OpenSSL::Crypto
        CURL::libcurl
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
            shlwapi
            mswsock
            ssp
    )
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(password-util-test
        tests/PasswordUtilTest.cc
        utils/PasswordUtil.cc
    )
    target_include_directories(password-util-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(password-util-test PRIVATE OpenSSL::Crypto)
    add_test(NAME password-util-test COMMAND password-util-test)
endif()
