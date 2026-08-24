# CMakeLists.txt for simple-ldapd
# Simple LDAP Daemon - A lightweight directory service
# Copyright 2026 SimpleDaemons

cmake_minimum_required(VERSION 3.16)
project(simple-ldapd VERSION 0.10.0 LANGUAGES CXX)

set(PROJECT_DESCRIPTION "Simple LDAP Daemon - A lightweight directory service")

include(GNUInstallDirs)
if(UNIX AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Installation prefix" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(PLATFORM_NAME "Windows")
elseif(APPLE)
    set(PLATFORM_NAME "macOS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(PLATFORM_NAME "FreeBSD")
elseif(UNIX)
    set(PLATFORM_NAME "Linux")
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_TESTS "Enable tests" ON)
option(ENABLE_PACKAGING "Enable package generation" ON)
option(ENABLE_SSL "Enable SSL/TLS support" ON)
option(ENABLE_JSON "Enable JSON support" ON)
option(ENABLE_STATIC_LINKING "Enable static linking for self-contained binaries" OFF)

find_package(Threads REQUIRED)

if(ENABLE_SSL)
    if(APPLE)
        foreach(_openssl_hint IN ITEMS
                "/opt/homebrew/opt/openssl@3"
                "/opt/homebrew/opt/openssl"
                "/usr/local/opt/openssl@3"
                "/usr/local/opt/openssl")
            if(EXISTS "${_openssl_hint}")
                list(PREPEND CMAKE_PREFIX_PATH "${_openssl_hint}")
            endif()
        endforeach()
    endif()
    find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)
    if(NOT TARGET OpenSSL::SSL)
        add_library(OpenSSL::SSL UNKNOWN IMPORTED)
        set_target_properties(OpenSSL::SSL PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${OPENSSL_SSL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
    endif()
    if(NOT TARGET OpenSSL::Crypto)
        add_library(OpenSSL::Crypto UNKNOWN IMPORTED)
        set_target_properties(OpenSSL::Crypto PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
    endif()
endif()

if(ENABLE_JSON)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(JSONCPP jsoncpp)
    endif()
    if(NOT JSONCPP_FOUND)
        message(WARNING "jsoncpp not found. JSON configuration support will be disabled.")
        set(ENABLE_JSON OFF)
    endif()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE LIB_SOURCES
    "src/simple-ldapd/*.cpp"
    "src/simple-ldapd/*.c"
)
file(GLOB_RECURSE HEADERS "include/*.hpp" "include/*.h")

add_library(${PROJECT_NAME}_lib STATIC ${LIB_SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME}_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC Threads::Threads)

if(WIN32)
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC ws2_32)
endif()

if(ENABLE_SSL)
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(${PROJECT_NAME}_lib PUBLIC SIMPLE_LDAPD_SSL=1)
endif()

if(ENABLE_JSON)
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC ${JSONCPP_LIBRARIES})
    target_include_directories(${PROJECT_NAME}_lib PUBLIC ${JSONCPP_INCLUDE_DIRS})
    target_compile_options(${PROJECT_NAME}_lib PRIVATE ${JSONCPP_CFLAGS_OTHER})
    target_link_directories(${PROJECT_NAME}_lib PUBLIC ${JSONCPP_LIBRARY_DIRS})
    target_compile_definitions(${PROJECT_NAME}_lib PUBLIC SIMPLE_LDAPD_JSON=1)
endif()

set(_ldap_warning_flags)
if(MSVC)
    set(_ldap_warning_flags /W3 /WX)
else()
    set(_ldap_warning_flags -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND _ldap_warning_flags -Wno-unused-private-field)
    endif()
endif()
target_compile_options(${PROJECT_NAME}_lib PRIVATE ${_ldap_warning_flags})

if(ENABLE_STATIC_LINKING)
    set(BUILD_SHARED_LIBS OFF)
    if(ENABLE_SSL)
        set(OPENSSL_USE_STATIC_LIBS TRUE)
    endif()
    if(MSVC)
        target_compile_options(${PROJECT_NAME}_lib PRIVATE /MT)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${PROJECT_NAME}_lib PRIVATE -static-libgcc -static-libstdc++)
        target_link_options(${PROJECT_NAME}_lib PRIVATE -static-libgcc -static-libstdc++)
    endif()
endif()

set(LDAP_BINARIES
    simple-ldapd
    ldapsearch
    ldapadd
    ldapmodify
    ldapdelete
    ldappasswd
)

foreach(_bin IN LISTS LDAP_BINARIES)
    add_executable(${_bin} main/${_bin}.cpp)
    target_link_libraries(${_bin} PRIVATE ${PROJECT_NAME}_lib)
    target_compile_options(${_bin} PRIVATE ${_ldap_warning_flags})
endforeach()

set(LDAP_RUNTIME_TARGETS ${LDAP_BINARIES})

if(APPLE)
    install(TARGETS ${LDAP_RUNTIME_TARGETS}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    install(DIRECTORY config/
        DESTINATION /etc/${PROJECT_NAME}
        FILES_MATCHING PATTERN "*.conf" PATTERN "*.example" PATTERN "*.ldif"
    )
    install(DIRECTORY schemas/
        DESTINATION /etc/${PROJECT_NAME}/schemas
        FILES_MATCHING PATTERN "*.schema"
    )
    install(FILES deployment/launchd/com.simpledaemons.${PROJECT_NAME}.plist
        DESTINATION /Library/LaunchDaemons
    )
else()
    install(TARGETS ${LDAP_RUNTIME_TARGETS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    install(DIRECTORY config/
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}
        FILES_MATCHING PATTERN "*.conf" PATTERN "*.example" PATTERN "*.ldif"
    )
    install(DIRECTORY schemas/
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}/schemas
        FILES_MATCHING PATTERN "*.schema"
    )
    if(UNIX AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        install(FILES deployment/systemd/${PROJECT_NAME}.service
            DESTINATION lib/systemd/system
        )
    endif()
endif()

if(ENABLE_TESTS)
    enable_testing()
    function(add_ldap_test name source)
        add_executable(${name} ${source})
        target_link_libraries(${name} PRIVATE ${PROJECT_NAME}_lib)
        target_compile_options(${name} PRIVATE ${_ldap_warning_flags})
        add_test(NAME ${name} COMMAND ${name})
    endfunction()

    add_ldap_test(test_ldap_config tests/unit/test_ldap_config.cpp)
    add_ldap_test(test_ldap_acl tests/unit/test_ldap_acl.cpp)
    add_ldap_test(test_ldap_backend tests/unit/test_ldap_backend.cpp)
    add_ldap_test(test_ldap_protocol tests/unit/test_ldap_protocol.cpp)
    add_ldap_test(test_ldap_cli tests/integration/test_ldap_cli.cpp)
    add_ldap_test(test_ldap_bind_search tests/integration/test_ldap_bind_search.cpp)
    add_ldap_test(test_ldap_write tests/integration/test_ldap_write.cpp)
    add_ldap_test(test_ldap_schema tests/integration/test_ldap_schema.cpp)
    add_ldap_test(test_ldap_gssapi tests/unit/test_ldap_gssapi.cpp)
    add_ldap_test(test_ldap_sasl tests/integration/test_ldap_sasl.cpp)
    add_ldap_test(test_ldap_passwd tests/integration/test_ldap_passwd.cpp)
    add_ldap_test(test_ldap_tls tests/integration/test_ldap_tls.cpp)
    add_ldap_test(test_ldap_security tests/security/test_ldap_security.cpp)
    add_ldap_test(test_ldap_performance tests/performance/test_ldap_performance.cpp)
endif()

if(ENABLE_PACKAGING)
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VENDOR "SimpleDaemons")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_CONTACT "info@simpledaemons.com")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/simpledaemons/${PROJECT_NAME}")

    if(APPLE)
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/LICENSE.txt")
        set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/packaging/assets/readme.html")
        set(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_CURRENT_SOURCE_DIR}/packaging/assets/welcome.html")
    else()
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
        set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    endif()

    if(WIN32)
        set(CPACK_GENERATOR "WIX;ZIP;NSIS")
        set(CPACK_WIX_PRODUCT_GUID "3e7c1a9b-5d24-4f81-9c6a-a1b2c3d4e5f6")
        set(CPACK_WIX_UPGRADE_GUID "8b4e2d1c-6a37-4e90-b7d5-f1e2d3c4b5a6")
        set(CPACK_WIX_PROPERTY_ARPHELPLINK "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_WIX_PROPERTY_ARPNOREPAIR "1")
        set(CPACK_WIX_PROPERTY_ARPNOMODIFY "1")
        set(CPACK_ZIP_COMPONENT_INSTALL ON)
        set(CPACK_NSIS_DISPLAY_NAME "Simple LDAP Daemon")
        set(CPACK_NSIS_PACKAGE_NAME "simple-ldapd")
    elseif(APPLE)
        set(CPACK_GENERATOR "DragNDrop;productbuild")
        set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
        set(CPACK_SET_DESTDIR TRUE)
        set(CPACK_PRODUCTBUILD_DOMAINS TRUE)
        set(CPACK_PRODUCTBUILD_DOMAINS_ANYWHERE TRUE)
        set(CPACK_PRODUCTBUILD_DOMAINS_ROOT TRUE)
        set(CPACK_PACKAGE_RELOCATABLE FALSE)
        set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME}")
        set(CPACK_DMG_FORMAT "UDBZ")
        set(CPACK_PRODUCTBUILD_IDENTIFIER "com.simpledaemons.${PROJECT_NAME}")
        set(CPACK_PRODUCTBUILD_PACKAGE_NAME "${PROJECT_NAME}")
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        set(CPACK_GENERATOR "FREEBSD")
        set(CPACK_PACKAGING_INSTALL_PREFIX "/")
        set(CPACK_SET_DESTDIR TRUE)
        set(CPACK_FREEBSD_PACKAGE_MAINTAINER "SimpleDaemons <info@simpledaemons.com>")
        set(CPACK_FREEBSD_PACKAGE_ORIGIN "net/${PROJECT_NAME}")
        set(CPACK_FREEBSD_PACKAGE_LICENSE "APACHE20")
        if(ENABLE_STATIC_LINKING)
            set(CPACK_FREEBSD_PACKAGE_RUNTIME_DEPENDS "")
        else()
            set(CPACK_FREEBSD_PACKAGE_RUNTIME_DEPENDS "openssl")
        endif()
    else()
        set(CPACK_GENERATOR "DEB;RPM")
        set(CPACK_PACKAGING_INSTALL_PREFIX "/")
        set(CPACK_SET_DESTDIR TRUE)
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "SimpleDaemons <info@simpledaemons.com>")
        set(CPACK_DEBIAN_PACKAGE_SECTION "net")
        set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
        if(ENABLE_STATIC_LINKING)
            set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
        else()
            set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl3 (>= 3.0.0)")
        endif()
        set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "systemd")
        set(CPACK_DEBIAN_PACKAGE_SUGGESTS "logrotate")
        set(CPACK_DEBIAN_PACKAGE_CONFLICTS "")
        set(CPACK_DEBIAN_PACKAGE_PROVIDES "${PROJECT_NAME}")
        set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
        set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
        set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
        set(CPACK_RPM_PACKAGE_URL "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_RPM_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
        if(ENABLE_STATIC_LINKING)
            set(CPACK_RPM_PACKAGE_REQUIRES "")
        else()
            set(CPACK_RPM_PACKAGE_REQUIRES "openssl-libs")
        endif()
        set(CPACK_RPM_PACKAGE_SUGGESTS "systemd")
        set(CPACK_RPM_PACKAGE_PROVIDES "${PROJECT_NAME}")
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
        set(PACKAGE_ARCH "amd64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "aarch64")
        set(PACKAGE_ARCH "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "armv7hl")
        set(PACKAGE_ARCH "armhf")
    else()
        set(PACKAGE_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    if(APPLE)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(PLATFORM_ID "macos-apple")
        else()
            set(PLATFORM_ID "macos-intel")
        endif()
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        set(PLATFORM_ID "freebsd")
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}")
        set(CPACK_FREEBSD_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}")
    elseif(UNIX)
        set(PLATFORM_ID "linux-generic")
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}")
        set(CPACK_DEBIAN_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}")
        set(CPACK_RPM_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}")
    else()
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_NAME}-${PACKAGE_ARCH}")
    endif()

    set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
    set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
    set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-src")
    set(CPACK_SOURCE_IGNORE_FILES
        "/build/"
        "/dist/"
        "/.git/"
        "/.gitignore"
        "/.vscode/"
        "/.vs/"
        "/CMakeCache.txt"
        "/CMakeFiles/"
        "/cmake_install.cmake"
        "/Makefile"
        "/install_manifest.txt"
        "/compile_commands.json"
        "/CTestTestfile.cmake"
        "/_CPack_Packages/"
        "/.DS_Store"
        "/Thumbs.db"
    )
    include(CPack)
endif()

message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  Platform: ${PLATFORM_NAME}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Tests enabled: ${ENABLE_TESTS}")
message(STATUS "  Packaging enabled: ${ENABLE_PACKAGING}")
message(STATUS "  SSL support: ${ENABLE_SSL}")
message(STATUS "  JSON support: ${ENABLE_JSON}")
message(STATUS "")
