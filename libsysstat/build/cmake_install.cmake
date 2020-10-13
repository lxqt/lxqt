# Install script for directory: /home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xRuntimex" OR NOT CMAKE_INSTALL_COMPONENT)
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libsysstat-qt5.so.0.4.3"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libsysstat-qt5.so.0"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    endif()
  endforeach()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/libsysstat-qt5.so.0.4.3"
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/libsysstat-qt5.so.0"
    )
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libsysstat-qt5.so.0.4.3"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libsysstat-qt5.so.0"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xRuntimex" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libsysstat-qt5.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libsysstat-qt5.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libsysstat-qt5.so"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/libsysstat-qt5.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libsysstat-qt5.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libsysstat-qt5.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libsysstat-qt5.so")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/sysstat-qt5" TYPE FILE FILES
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/version.h"
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/sysstat_global.h"
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/basestat.h"
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/cpustat.h"
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/memstat.h"
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/netstat.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/sysstat-qt5/SysStat" TYPE FILE FILES
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/Version"
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/Global"
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/BaseStat"
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/CpuStat"
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/MemStat"
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/NetStat"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/sysstat-qt5.pc")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake/sysstat-qt5" TYPE FILE FILES
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/sysstat-qt5-config.cmake"
    "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/sysstat-qt5-config-version.cmake"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/sysstat-qt5/sysstat-qt5-targets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/sysstat-qt5/sysstat-qt5-targets.cmake"
         "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/CMakeFiles/Export/share/cmake/sysstat-qt5/sysstat-qt5-targets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/sysstat-qt5/sysstat-qt5-targets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/sysstat-qt5/sysstat-qt5-targets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake/sysstat-qt5" TYPE FILE FILES "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/CMakeFiles/Export/share/cmake/sysstat-qt5/sysstat-qt5-targets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake/sysstat-qt5" TYPE FILE FILES "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/CMakeFiles/Export/share/cmake/sysstat-qt5/sysstat-qt5-targets-debug.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/hangsia/Downloads/Project From Sunny/lxqt/libsysstat/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
