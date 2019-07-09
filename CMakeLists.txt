cmake_minimum_required(VERSION 3.7)
cmake_policy(VERSION 3.7.2)

project(QmlOsgEarthTest LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_PREFIX CACHE STRING "${CMAKE_BINARY_DIR}/install")

### Output dirs
set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

set(Qt5_COMPONENTS
    Core
    Gui
    OpenGL
    Qml
    Quick
    QuickWidgets
)

set(Qt5_INSTALL_SYSTEM_RUNTIME_DESTINATION lib${LIB_POSTFIX})
find_package(Qt5 COMPONENTS Core Quick REQUIRED)

find_package(OpenSceneGraph REQUIRED COMPONENTS osgDB osgUtil osgGA osgViewer osgShadow osgTerrain osgSim osgManipulator)

find_package(OsgEarth)

set(MAIN_SRCS
    ${CMAKE_SOURCE_DIR}/main.cpp
    ${CMAKE_SOURCE_DIR}/osgearth_item.cpp
    ${CMAKE_SOURCE_DIR}/osgearth_item.h
)
list(APPEND ALL_SRCS
    ${MAIN_HDRS}
    ${MAIN_SRCS}
)

set(QT_MOC_HDRS
    ${CMAKE_SOURCE_DIR}/osgearth_item.h
)

unset(QT_MOC_SRCS)
qt5_wrap_cpp(QT_MOC_SRCS ${QT_MOC_HDRS})

list(APPEND ALL_SRCS
    ${QT_MOC_SRCS}
    ${QT_COMP_RCS}
)

set(TARGET_NAME ${PROJECT_NAME})

qt5_add_resources(QRC_FILES "${CMAKE_SOURCE_DIR}/resources.qrc")
add_executable(${TARGET_NAME}
    ${ALL_SRCS}
    ${ALL_RCS}
    ${QRC_FILES}
)

target_include_directories(${TARGET_NAME} PRIVATE
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
    ${OSGEARTH_INCLUDE_DIRS}
)

target_link_libraries(${TARGET_NAME}
    ${Qt5_LIBRARIES}
    Qt5::Core Qt5::Quick GL
    ${OPENSCENEGRAPH_LIBRARIES}
    ${OSGEARTH_LIBRARY}
    ${OSGEARTHANNOTATION_LIBRARY}
    ${OSGEARTHFEATURES_LIBRARY}
    ${OSGEARTHSYMBOLOGY_LIBRARY}
    ${OSGEARTHUTIL_LIBRARY}
)

target_compile_definitions(
    ${TARGET_NAME} PRIVATE
    ${CONFIGURATION_FLAGS}
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>
)
