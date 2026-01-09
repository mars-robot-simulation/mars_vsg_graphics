project(mars_vsg_graphics)
set(PROJECT_VERSION 1.0)
set(PROJECT_DESCRIPTION "This library provides a 3d visualization through simple interfaces.")
cmake_minimum_required(VERSION 2.6)

include(FindPkgConfig)
include(CheckIncludeFileCXX)

CHECK_INCLUDE_FILE_CXX("tr1/functional" USE_TR1)
if(${USE_TR1})
    ADD_DEFINITIONS(-DUSE_TR1)
else(${USE_TR1})
    ADD_DEFINITIONS(-DNO_TR1)
endif()

find_package(lib_manager REQUIRED)
lib_defaults()
define_module_info()

setup_opencv()

include_directories(
      src
)

# set the use of C++17 globally as all examples require it
set(CMAKE_CXX_STANDARD 17)

OPTION(DEPTH_IMAGES "Compile with Depth image support, might not work on all machines" true)
if(DEPTH_IMAGES)
    ADD_DEFINITIONS(-DDEPTH_IMAGES)
endif()

set(USE_VERTEX_BUFFER 1)
if(USE_VERTEX_BUFFER)
    ADD_DEFINITIONS(-DUSE_VERTEX_BUFFER)
endif()

set(DEPENDENCIES
    lib_manager
    mars_interfaces
    cfg_manager
    data_broker
    configmaps
    mars_utils
)

find_package(vsg 1.1.11)

vsg_setup_dir_vars()
vsg_setup_build_vars()

find_package(vsgXchange 1.1.7 QUIET)
find_package(vsgQt)

set (QT_USE_QTOPENGL TRUE)
setup_qt()

if (${USE_QT5})
set( OSG_QT openscenegraph-osgQt5)
else (${USE_QT5})
set( OSG_QT openscenegraph-osgQt)
endif (${USE_QT5})

pkg_check_modules(OSGQT ${OSG_QT})
if(OSGQT_FOUND)
    list(APPEND DEPENDENCIES ${OSG_QT})
endif()
pkg_check_modules(Dependencies REQUIRED IMPORTED_TARGET ${DEPENDENCIES})

set(HEADERS
           src/GraphicsManager.hpp
           src/DrawObject.hpp
           src/gui_helper_functions.hpp
)


set(SOURCES
           src/GraphicsManager.cpp
           src/DrawObject.cpp
           src/gui_helper_functions.cpp
)

if (${USE_QT5})
qt5_wrap_cpp( QT_MOC_HEADER_SRC ${QT_MOC_HEADERS})
else (${USE_QT5})
qt4_wrap_cpp( QT_MOC_HEADER_SRC ${QT_MOC_HEADERS})
endif (${USE_QT5})

#cmake variables
configure_file(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h @ONLY)
include_directories("${CMAKE_BINARY_DIR}")

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${QT_MOC_HEADER_SRC} config.h)

if (${USE_QT5})
qt5_use_modules(${PROJECT_NAME} Widgets OpenGL)
#qt5_use_modules(${PROJECT_NAME} MacExtras)
endif (${USE_QT5})

if(APPLE)
 FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
 SET(APPLE_LIBS ${OPENGL_LIBRARY} -L/opt/local/lib opencv_core opencv_highgui opencv_imgproc opencv_imgcodecs)
endif(APPLE)

if(WIN32)
  if(USE_VERTEX_BUFFER)
    SET(WIN_LIBS glew32 opengl32)
  else(USE_VERTEX_BUFFER)
    SET(WIN_LIBS opengl32)
  endif(USE_VERTEX_BUFFER)
endif(WIN32)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        PkgConfig::Dependencies
        ${APPLE_LIBS}
        ${WIN_LIBS}
        ${EXTRA_LIBS}
        pthread
        vsg::vsg
        vsgXchange::vsgXchange
        vsgQt::vsgQt
)

if(WIN32)
  set(LIB_INSTALL_DIR bin) # .dll are in PATH, like executables
else(WIN32)
  set(LIB_INSTALL_DIR lib)
endif(WIN32)

set(_INSTALL_DESTINATIONS
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION ${LIB_INSTALL_DIR}
	ARCHIVE DESTINATION lib
)

# Install the library
install(TARGETS ${PROJECT_NAME} ${_INSTALL_DESTINATIONS})

# Install headers into mars include directory
install(FILES ${HEADERS} DESTINATION include/${PROJECT_NAME})
install(FILES ${HEADERS_2D} DESTINATION include/${PROJECT_NAME}/2d_objects)
install(FILES ${HEADERS_3D} DESTINATION include/${PROJECT_NAME}/3d_objects)
install(FILES ${HEADERS_WRAPPER} DESTINATION include/${PROJECT_NAME}/wrapper)

# Prepare and install necessary files to support finding of the library
# using pkg-config
list(JOIN DEPENDENCIES " " PKGCONFIG_REQUIRES)
configure_file(configuration/${PROJECT_NAME}.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)

#configure_file(configuration/mars_Graphics.in ${CMAKE_BINARY_DIR}/mars_Graphics.yaml @ONLY)
#install(FILES ${CMAKE_BINARY_DIR}/mars_Graphics.yaml DESTINATION configuration/mars_default/)

INSTALL(DIRECTORY resources DESTINATION share/${PROJECT_NAME})
