/*
This file is part of MeshMagick - An Ogre mesh file manipulation tool.
Copyright (C) 2007 - Daniel Wickert

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

#ifndef __MM_MERGE_TOOL_H__
#define __MM_MERGE_TOOL_H__

#include "MeshMagickPrerequisites.h"
#include "Tool.h"
#include "OptionsParser.h"

#ifdef __APPLE__
#	include <Ogre/OgreMesh.h>
#	include <Ogre/OgreSkeleton.h>
#else
#	include <OgreMesh.h>
#	include <OgreSkeleton.h>
#endif

namespace meshmagick
{
	class _MeshMagickExport MeshMergeTool : public Tool
    {
    public:
        MeshMergeTool();
		~MeshMergeTool();

		void addMesh(Ogre::MeshPtr mesh);
		Ogre::MeshPtr bake(const Ogre::String& meshname);

	private: 
		Ogre::SkeletonPtr mBaseSkeleton;
		std::vector<Ogre::MeshPtr> mMeshes;

		const Ogre::String findSubmeshName(Ogre::MeshPtr m, Ogre::ushort sid) const;

		void doInvoke(const OptionList& toolOptions,
			const Ogre::StringVector& inFileNames,
			const Ogre::StringVector& outFileNames);
    };
}
#endif
