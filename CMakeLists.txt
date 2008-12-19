project(meshmagick)

cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0003 NEW)
cmake_policy(SET CMP0005 OLD)

include(FeatureSummary)

set(MESHMAGICK_MAJOR_VERSION 0)
set(MESHMAGICK_MINOR_VERSION 5)
set(MESHMAGICK_PATCH_VERSION 0)

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

include_directories(include ${OGRE_INCLUDE_DIRS})

add_library(meshmagick_shared_lib SHARED ${MESHMAGICK_SOURCE})
set_target_properties(meshmagick_shared_lib PROPERTIES
	OUTPUT_NAME meshmagic
	VERSION ${MESHMAGICK_MAJOR_VERSION}.${MESHMAGICK_MINOR_VERSION}.${MESHMAGICK_PATCH_VERSION}
	SOVERSION ${MESHMAGICK_MAJOR_VERSION}.${MESHMAGICK_MINOR_VERSION}
	DEFINE_SYMBOL MESHMAGICK_EXPORTS)
target_link_libraries(meshmagick_shared_lib ${OGRE_LIBRARIES})

add_executable(meshmagick_bin src/main.cpp)
set_target_properties(meshmagick_bin PROPERTIES
	OUTPUT_NAME meshmagic
	DEFINE_SYMBOL MESHMAGICK_IMPORTS)
target_link_libraries(meshmagick_bin meshmagick_shared_lib ${OGRE_LIBRARIES})
