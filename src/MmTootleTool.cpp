/*
This file is part of MeshMagick - An Ogre mesh file manipulation tool.
Copyright (C) 2007-2009 Daniel Wickert

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "MmTootleTool.h"

#include <tootlelib.h>

#include <OgreMesh.h>
#include <OgreSubMesh.h>

#include "MmOgreEnvironment.h"
#include "MmStatefulMeshSerializer.h"

using namespace Ogre;

namespace meshmagick
{
	TootleTool::TootleTool()
		: Tool()
	{
	}

	TootleTool::~TootleTool()
	{
	}

	Ogre::String TootleTool::getName() const
	{
		return "Tootle";
	}

	void TootleTool::doInvoke(
		const OptionList &toolOptions, 
		const Ogre::StringVector &inFileNames, 
		const Ogre::StringVector &outFileNamesArg)
	{
		// Name count has to match, else we have no way to figure out how to apply output
		// names to input files.
		if (!(outFileNamesArg.empty() || inFileNames.size() == outFileNamesArg.size()))
		{
			fail("number of output files must match number of input files.");
		}

		StringVector outFileNames = outFileNamesArg.empty() ? inFileNames : outFileNamesArg;

		// Process the meshes
		for (size_t i = 0, end = inFileNames.size(); i < end; ++i)
		{
			if (StringUtil::endsWith(inFileNames[i], ".mesh", true))
			{
				processMeshFile(toolOptions, inFileNames[i], outFileNames[i]);
			}
			else
			{
				warn("unrecognised name ending for file " + inFileNames[i]);
				warn("file skipped.");
			}
		}
	}

	void TootleTool::processMeshFile(
		const OptionList &toolOptions, Ogre::String inFile, Ogre::String outFile)
	{
		StatefulMeshSerializer* meshSerializer =
			OgreEnvironment::getSingleton().getMeshSerializer();

		print("Loading mesh " + inFile + "...");
		MeshPtr mesh;
		try
		{
			mesh = meshSerializer->loadMesh(inFile);
		}
		catch(std::exception& e)
		{
			warn(e.what());
			warn("Unable to open mesh file " + inFile);
			warn("file skipped.");
			return;
		}
		print("Processing mesh...");

		// TODO

	}

}
