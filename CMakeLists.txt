project(meshmagick)

cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0003 NEW)
cmake_policy(SET CMP0005 OLD)

include(FeatureSummary)

set(MESHMAGICK_MAJOR_VERSION 0)
set(MESHMAGICK_MINOR_VERSION 5)
set(MESHMAGICK_PATCH_VERSION 1)

# include our custom cmake scripts
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# dependencies
find_package(PkgConfig)
find_package(OGRE 1.6 REQUIRED)

set(MESHMAGICK_SOURCE
src/EditableBone.cpp
src/EditableMesh.cpp
src/EditableSkeleton.cpp
src/InfoTool.cpp
src/InfoToolFactory.cpp
src/Makefile.am
src/MeshMergeTool.cpp
src/MeshMergeToolFactory.cpp
src/MeshUtils.cpp
src/OgreEnvironment.cpp
src/OptimiseTool.cpp
src/OptimiseToolFactory.cpp
src/OptionsParser.cpp
src/RenameTool.cpp
src/RenameToolFactory.cpp
src/StatefulMeshSerializer.cpp
src/StatefulSkeletonSerializer.cpp
src/Tool.cpp
src/ToolManager.cpp
src/ToolsUtils.cpp
src/TransformTool.cpp
src/TransformToolFactory.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${OGRE_INCLUDE_DIRS})

add_library(meshmagick_shared_lib SHARED ${MESHMAGICK_SOURCE})
set_target_properties(meshmagick_shared_lib PROPERTIES
	OUTPUT_NAME meshmagick
	VERSION ${MESHMAGICK_MAJOR_VERSION}.${MESHMAGICK_MINOR_VERSION}.${MESHMAGICK_PATCH_VERSION}
	SOVERSION ${MESHMAGICK_MAJOR_VERSION}.${MESHMAGICK_MINOR_VERSION}
	DEFINE_SYMBOL MESHMAGICK_EXPORTS)
target_link_libraries(meshmagick_shared_lib ${OGRE_LIBRARIES})

add_executable(meshmagick_bin src/main.cpp)
set_target_properties(meshmagick_bin PROPERTIES
	OUTPUT_NAME meshmagick
	DEFINE_SYMBOL MESHMAGICK_IMPORTS)
target_link_libraries(meshmagick_bin meshmagick_shared_lib ${OGRE_LIBRARIES})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meshmagick.pc.in ${CMAKE_CURRENT_BINARY_DIR}/meshmagick.pc)

install(TARGETS meshmagick_bin meshmagick_shared_lib
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib)
	
if(NOT WIN32)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/meshmagick.pc
	        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
endif(NOT WIN32)

install(FILES
include/EditableBone.h
include/EditableMesh.h
include/EditableSkeleton.h
include/InfoToolFactory.h
include/InfoTool.h
include/MeshMagickPrerequisites.h
include/MeshMergeToolFactory.h
include/MeshMergeTool.h
include/MeshUtils.h
include/OgreEnvironment.h
include/OptimiseToolFactory.h
include/OptimiseTool.h
include/OptionsParser.h
include/RenameToolFactory.h
include/RenameTool.h
include/StatefulMeshSerializer.h
include/StatefulSkeletonSerializer.h
include/ToolFactory.h
include/Tool.h
include/ToolManager.h
include/ToolUtils.h
include/TransformToolFactory.h
include/TransformTool.h
DESTINATION ${CMAKE_INSTALL_PREFIX}/include/meshmagick)

include(CPack)
