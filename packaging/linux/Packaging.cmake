# Included after all application targets and install rules.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  return()
endif()

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "worksidekick")
set(CPACK_PACKAGE_VENDOR "WorkSidekick")
set(CPACK_PACKAGE_CONTACT "WorkSidekick maintainers")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Explicit selection AI assistant")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_DEBIAN_PACKAGE_VERSION "${PROJECT_VERSION}~rc1")
set(CPACK_DEBIAN_PACKAGE_NAME "worksidekick")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/wzk111/leadership-dandang-win-linux")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# dlopen() plugins/services are not discovered by dpkg-shlibdeps. On Ubuntu
# 22.04 qt6-qpa-plugins owns platforms/libqxcb.so; qt6-wayland owns the Wayland QPA
# plugins. Keep both explicit even when the build only exercised X11.
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6gui6 (>= 6.2), qt6-qpa-plugins (>= 6.2), qt6-wayland (>= 6.2), at-spi2-core, ca-certificates")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "gnome-keyring")
set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
set(CPACK_PACKAGE_CHECKSUM "SHA256")

find_package(Git REQUIRED)
execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  OUTPUT_VARIABLE WS_PACKAGE_SOURCE_SHA OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE ws_git_result)
if(NOT ws_git_result EQUAL 0)
  message(FATAL_ERROR "Package provenance requires the source Git SHA")
endif()
execute_process(COMMAND "${GIT_EXECUTABLE}" status --porcelain --untracked-files=no
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  OUTPUT_VARIABLE ws_git_changes OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE ws_git_result)
if(NOT ws_git_result EQUAL 0)
  message(FATAL_ERROR "Cannot inspect package source status")
endif()
set(WS_PACKAGE_SOURCE_DIRTY "false")
if(ws_git_changes)
  set(WS_PACKAGE_SOURCE_DIRTY "true")
endif()
file(READ "/etc/os-release" WS_PACKAGE_OS_RELEASE)
configure_file("${CMAKE_CURRENT_LIST_DIR}/build-info.txt.in"
  "${CMAKE_CURRENT_BINARY_DIR}/package-build-info.txt" @ONLY)
install(FILES "${PROJECT_SOURCE_DIR}/resources/worksidekick.svg"
  DESTINATION share/icons/hicolor/scalable/apps)
install(FILES "${PROJECT_SOURCE_DIR}/README.md"
  "${PROJECT_SOURCE_DIR}/docs/privacy.md"
  "${PROJECT_SOURCE_DIR}/docs/troubleshooting.md"
  "${PROJECT_SOURCE_DIR}/packaging/THIRD_PARTY.md"
  DESTINATION share/doc/worksidekick)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/package-build-info.txt"
  DESTINATION share/doc/worksidekick RENAME build-info.txt)
include(CPack)
