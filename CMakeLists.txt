cmake_minimum_required(VERSION 3.3)

# Name of the project (will be the name of the plugin)
project(findfile-matlab)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# Set the default installation directory
if (WIN32)
  set(MATLAB_USER_DIR "$ENV{USERPROFILE}/Documents/MATLAB")
else()
  set(MATLAB_USER_DIR "$ENV{home}/Documents/MATLAB")
endif()
if (NOT EXISTS ${MATLAB_USER_DIR})
  # if not found, show message and use the default CMake install directory
  message("Matlab user directory not found at ${MATLAB_USER_DIR}. It will be created if intallation directory not changed.")
elseif (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set (CMAKE_INSTALL_PREFIX "${MATLAB_USER_DIR}" CACHE PATH "Installation directory" FORCE)
endif()
set(MATLAB_FINDFILE_TOOLBOX_DIR "findfile" CACHE PATH "Installation subdirectory for Matlab-FindFile package")


find_package(Matlab REQUIRED COMPONENTS MX_LIBRARY)

# set(CMAKE_CXX_STANDARD 17)

set(TARGET_NAME "findfile")

matlab_add_mex(NAME ${TARGET_NAME} SRC findfile.cpp)

# if the compiler is MS VisualC++, add 'mexFunction' to the exported symbols
if(MSVC)
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS /EXPORT:mexFunction)
  target_compile_options(${TARGET_NAME} PRIVATE /wd4996)
endif()

file(RELATIVE_PATH DstRelativePath ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION "${MATLAB_FINDFILE_TOOLBOX_DIR}/${DstRelativePath}")
install(FILES "${TARGET_NAME}.m" DESTINATION "${MATLAB_FINDFILE_TOOLBOX_DIR}/${DstRelativePath}")
