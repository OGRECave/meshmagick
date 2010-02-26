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

#ifndef __MM__TOOTLETOOL_H__
#define __MM__TOOTLETOOL_H__

#include "MeshMagickPrerequisites.h"

#include "MmTool.h"

#include <OgreCommon.h>

namespace meshmagick
{

	class _MeshMagickExport TootleTool : public Tool
	{
	public:
		TootleTool();
		~TootleTool();

		Ogre::String getName() const;

		void processMeshFile(Ogre::String file, Ogre::String outFile);
		void processMesh(Ogre::MeshPtr mesh);
		void processMesh(Ogre::Mesh* mesh);

		unsigned int getVCacheSize() const { return mVCacheSize; }
		void setVCacheSize(unsigned int sz) { mVCacheSize = sz; }

		Ogre::CullingMode getCullingMode() const { return mClockwise ? Ogre::CULL_ANTICLOCKWISE : Ogre::CULL_CLOCKWISE; }
		void setCullingMode(Ogre::CullingMode md) { mClockwise = (md == Ogre::CULL_ANTICLOCKWISE); }

		unsigned int getClusters() const { return mClusters; }
		void setClusters(unsigned int sz) { mClusters = sz; }
	protected:
		virtual void doInvoke(const OptionList& toolOptions,
			const Ogre::StringVector& inFileNames,
			const Ogre::StringVector& outFileNames);

	private:
		unsigned int mVCacheSize;
		bool mClockwise;
		unsigned int mClusters;
		typedef std::vector<Ogre::Vector3> ViewpointList;
		ViewpointList mViewpointList;

		void setOptions(const OptionList& options);
	};

}

#endif // __MM__TOOTLETOOL_H__
