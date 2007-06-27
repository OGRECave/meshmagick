INCLUDES = ${OGRE_CFLAGS} -I ../include

lib_LTLIBRARIES = libmeshmagick.la
libmeshmagick_la_SOURCES = \
	EditableSkeleton.cpp \
	InfoTool.cpp \
	InfoToolFactory.cpp \
	MeshMergeTool.cpp \
	MeshMergeToolFactory.cpp \
	OgreEnvironment.cpp \
	OptionsParser.cpp \
	RenameTool.cpp \
	RenameToolFactory.cpp \
	StatefulMeshSerializer.cpp \
	StatefulSkeletonSerializer.cpp \
	Tool.cpp \
	ToolManager.cpp \
	TransformTool.cpp \
	TransformToolFactory.cpp 
libmeshmagick_la_LIBADD = ${OGRE_LIBS}


bin_PROGRAMS = meshmagick
meshmagick_SOURCES = main.cpp 
meshmagick_LDADD = -lmeshmagick
