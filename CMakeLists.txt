# CMakeLists.txt for simple-ldapd
# Simple LDAP Daemon - A lightweight directory service
# Copyright 2026 SimpleDaemons

cmake_minimum_required(VERSION 3.16)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
endif()
project(simple-ldapd VERSION 1.0.0 LANGUAGES CXX)

set(PROJECT_DESCRIPTION "Simple LDAP Daemon - A lightweight directory service")

if(UNIX AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Installation prefix")
    endif()
endif()
include(GNUInstallDirs)

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
option(ENABLE_SQLITE "Enable SQLite directory backend" ON)
option(ENABLE_STATIC_LINKING "Enable static linking for self-contained binaries" OFF)
set(LDAP_CLI_PREFIX "" CACHE STRING "Prefix for OpenLDAP-style CLI tool names (e.g. simple-)")

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

if(ENABLE_SQLITE)
    if(APPLE)
        foreach(_sqlite_hint IN ITEMS
                "/opt/homebrew/opt/sqlite"
                "/usr/local/opt/sqlite")
            if(EXISTS "${_sqlite_hint}")
                list(PREPEND CMAKE_PREFIX_PATH "${_sqlite_hint}")
            endif()
        endforeach()
    endif()
    find_package(SQLite3)
    if(SQLite3_FOUND AND SQLite3_LIBRARIES)
        target_include_directories(${PROJECT_NAME}_lib PUBLIC ${SQLite3_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME}_lib PUBLIC ${SQLite3_LIBRARIES})
        target_compile_definitions(${PROJECT_NAME}_lib PUBLIC SIMPLE_LDAPD_SQLITE=1)
    else()
        message(WARNING "SQLite3 not found. sqlite backend will be unavailable.")
        set(ENABLE_SQLITE OFF)
    endif()
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
    ldapcompare
    ldapwhoami
)

foreach(_bin IN LISTS LDAP_BINARIES)
    add_executable(${_bin} main/${_bin}.cpp)
    target_link_libraries(${_bin} PRIVATE ${PROJECT_NAME}_lib)
    target_compile_options(${_bin} PRIVATE ${_ldap_warning_flags})
    if(LDAP_CLI_PREFIX AND NOT _bin STREQUAL "simple-ldapd")
        set_target_properties(${_bin} PROPERTIES OUTPUT_NAME "${LDAP_CLI_PREFIX}${_bin}")
    endif()
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
    install(FILES deployment/launchd/com.simpledaemons.${PROJECT_NAME}.plist
        DESTINATION /Library/LaunchDaemons
    )
    install(DIRECTORY DESTINATION /var/lib/${PROJECT_NAME})
    install(DIRECTORY DESTINATION /var/log/${PROJECT_NAME})
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
    install(DIRECTORY DESTINATION /var/lib/${PROJECT_NAME})
    install(DIRECTORY DESTINATION /var/log/${PROJECT_NAME})
    if(UNIX AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        install(FILES deployment/systemd/${PROJECT_NAME}.service
            DESTINATION lib/systemd/system
        )
        install(FILES deployment/systemd/${PROJECT_NAME}.sysusers
            DESTINATION lib/sysusers.d
            RENAME ${PROJECT_NAME}.conf
        )
        install(FILES deployment/systemd/${PROJECT_NAME}.tmpfiles
            DESTINATION lib/tmpfiles.d
            RENAME ${PROJECT_NAME}.conf
        )
        install(FILES deployment/logrotate.d/${PROJECT_NAME}
            DESTINATION /etc/logrotate.d
        )
    endif()
endif()

if(NOT WIN32)
    # FHS /etc, not ${prefix}/etc (CMAKE_INSTALL_SYSCONFDIR is relative).
    set(_ldap_confdir "/etc/${PROJECT_NAME}")
    install(DIRECTORY config/templates/
        DESTINATION ${_ldap_confdir}/templates
        FILES_MATCHING PATTERN "*.conf"
    )
    install(DIRECTORY config/examples/
        DESTINATION ${_ldap_confdir}/examples
    )
    install(FILES config/README.md
        DESTINATION ${_ldap_confdir}
    )
    install(DIRECTORY schemas/
        DESTINATION ${_ldap_confdir}/schemas
    )
    install(DIRECTORY DESTINATION ${_ldap_confdir}/tls)
    install(FILES LICENSE README.md CHANGELOG.md
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
    )
    install(DIRECTORY docs/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/docs
        FILES_MATCHING PATTERN "*.md"
    )
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
    add_ldap_test(test_ldap_password tests/unit/test_ldap_password.cpp)
    add_ldap_test(test_ldap_backend tests/unit/test_ldap_backend.cpp)
    add_ldap_test(test_ldap_protocol tests/unit/test_ldap_protocol.cpp)
    add_ldap_test(test_ldap_cli tests/integration/test_ldap_cli.cpp)
    add_ldap_test(test_ldap_bind_search tests/integration/test_ldap_bind_search.cpp)
    add_ldap_test(test_ldap_write tests/integration/test_ldap_write.cpp)
    add_ldap_test(test_ldap_ops tests/integration/test_ldap_ops.cpp)
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
        set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
            "CreateDirectory '$COMMON_APPDATA\\\\simple-ldapd'\\nCreateDirectory '$COMMON_APPDATA\\\\simple-ldapd\\\\logs'\\nCreateDirectory '$COMMON_APPDATA\\\\simple-ldapd\\\\tls'")
        set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
            "nsExec::ExecToLog 'sc stop simple-ldapd'\\nnsExec::ExecToLog 'sc delete simple-ldapd'")
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
        set(CPACK_PRODUCTBUILD_PACKAGE_NAME "${PROJECT_NAME}")
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
        set(CPACK_POSTFLIGHT_Unspecified_SCRIPT
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/pkg/scripts/postinstall")
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
            set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl3 (>= 3.0.0), libsqlite3-0")
        endif()
        set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "systemd")
        set(CPACK_DEBIAN_PACKAGE_SUGGESTS "logrotate")
        set(CPACK_DEBIAN_PACKAGE_CONFLICTS "")
        set(CPACK_DEBIAN_PACKAGE_PROVIDES "${PROJECT_NAME}")
        set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
        set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/deb/postinst"
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/deb/prerm"
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/deb/postrm"
        )
        set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
        set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
        set(CPACK_RPM_PACKAGE_URL "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_RPM_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
        if(ENABLE_STATIC_LINKING)
            set(CPACK_RPM_PACKAGE_REQUIRES "")
        else()
            set(CPACK_RPM_PACKAGE_REQUIRES "openssl-libs, sqlite-libs")
        endif()
        set(CPACK_RPM_PACKAGE_SUGGESTS "systemd")
        set(CPACK_RPM_PACKAGE_PROVIDES "${PROJECT_NAME}")
        set(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm/preinstall.sh")
        set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm/postinstall.sh")
        set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm/preuninstall.sh")
        set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm/postuninstall.sh")
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
        set(CPACK_DEBIAN_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}.deb")
        set(CPACK_RPM_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}.rpm")
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
message(STATUS "  SQLite support: ${ENABLE_SQLITE}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  CLI prefix: ${LDAP_CLI_PREFIX}")
message(STATUS "")
