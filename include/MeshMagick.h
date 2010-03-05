/*
This file is part of MeshMagick - An Ogre mesh file manipulation tool.
Copyright (C) 2007-2010 Daniel Wickert

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef __MM__MESHMAGICK_H__
#define __MM__MESHMAGICK_H__

#include "MeshMagickPrerequisites.h"

#include <OgreLog.h>
#include <OgreSingleton.h>

#include "MmOgreEnvironment.h"
#include "MmTool.h"
#include "MmToolManager.h"

#include "MmInfoTool.h"
#include "MmMeshMergeTool.h"
#include "MmOptimiseTool.h"
#include "MmRenameTool.h"
#include "MmTransformTool.h"

namespace meshmagick
{
	/** This class provides the entry point for MeshMagick usage.
	@par
		In order to use MeshMagick's tools from your program,
		create a MeshMagick instance. Call the getXxxTool() member function
		in order to retrieve a tool. Use the tool as desired.
		Delete this class when done with the tools. Don't delete a Tool,
		it gets destroyed when MeshMagick gets destroyed.
	*/
	class _MeshMagickExport MeshMagick : public Ogre::Singleton<MeshMagick>
	{
	public:
		MeshMagick(Ogre::Log* log = NULL);
		~MeshMagick();

		InfoTool* getInfoTool();
		MeshMergeTool* getMeshMergeTool();
		TransformTool* getTransformTool();

	private:
		InfoTool* mInfoTool;
		MeshMergeTool* mMeshMergeTool;
		TransformTool* mTransformTool;

		ToolManager* mToolManager;
		OgreEnvironment* mOgreEnvironment;
	};
}
#endif
