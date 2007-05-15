INCLUDES = ${OGRE_CFLAGS} -I ./include

lib_LTLIBRARIES = libmeshmagick.la
libmeshmagick_la_SOURCES = \
	src/InfoTool.cpp \
	src/InfoToolFactory.cpp \
	src/MeshMergeTool.cpp \
	src/MeshMergeToolFactory.cpp \
	src/OgreEnvironment.cpp \
	src/OptionsParser.cpp \
	src/StatefulMeshSerializer.cpp \
	src/StatefulSkeletonSerializer.cpp \
	src/Tool.cpp \
	src/ToolManager.cpp \
	src/TransformTool.cpp \
	src/TransformToolFactory.cpp 
libmeshmagick_la_LIBADD = ${OGRE_LIBS}


bin_PROGRAMS = meshmagick
meshmagick_SOURCES = src/main.cpp 
meshmagick_LDADD = -lmeshmagick
