INCLUDES = ${OGRE_CFLAGS} -I ../include

lib_LTLIBRARIES = libmeshmagick.la
libmeshmagick_la_SOURCES = \
	EditableBone.cpp \
	EditableSkeleton.cpp \
	InfoTool.cpp \
	InfoToolFactory.cpp \
	MeshMergeTool.cpp \
	MeshMergeToolFactory.cpp \
	MeshUtils.cpp \
	OgreEnvironment.cpp \
	OptimiseTool.cpp \
	OptimiseToolFactory.cpp \
	OptionsParser.cpp \
	RenameTool.cpp \
	RenameToolFactory.cpp \
	StatefulMeshSerializer.cpp \
	StatefulSkeletonSerializer.cpp \
	Tool.cpp \
	ToolManager.cpp \
	ToolsUtils.cpp \
	TransformTool.cpp \
	TransformToolFactory.cpp 
libmeshmagick_la_LIBADD = ${OGRE_LIBS}


bin_PROGRAMS = meshmagick
meshmagick_SOURCES = main.cpp 
meshmagick_LDADD = -lmeshmagick
