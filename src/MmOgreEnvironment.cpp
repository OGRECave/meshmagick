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

#include "MmOgreEnvironment.h"

#include <OgreRoot.h>
#include <OgreLogManager.h>
#include <OgreMaterialManager.h>
#include <OgreMeshManager.h>
#include <OgreResourceGroupManager.h>
#include <OgreSkeletonManager.h>

using namespace Ogre;

template<> meshmagick::OgreEnvironment* Singleton<meshmagick::OgreEnvironment>::ms_Singleton = NULL;

namespace meshmagick
{
    OgreEnvironment::OgreEnvironment()
        : mLogMgr(NULL),
          mResourceGroupMgr(NULL),
          mMath(NULL),
          mMeshMgr(NULL),
#if OGRE_VERSION_MAJOR > 1 || (OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR >= 7)
		  mLodStrategyMgr(NULL),
#endif
          mMaterialMgr(NULL),
          mSkeletonMgr(NULL),
          mMeshSerializer(NULL),
          mSkeletonSerializer(NULL),
          mBufferManager(NULL)
    {
    }

    OgreEnvironment::~OgreEnvironment()
    {
		if (mStandalone)
		{
			delete mBufferManager;
			delete mSkeletonSerializer;
			delete mMeshSerializer;
#if OGRE_VERSION_MAJOR > 1 || (OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR >= 7)
			delete mLodStrategyMgr;
#endif
			delete mMath;
			mLogMgr->destroyLog(mLog);
			delete mLogMgr;
			delete mRoot;
		}
    }

	void OgreEnvironment::initialize(bool standalone, Ogre::Log* log)
	{
		if (standalone)
		{
			mLogMgr = new LogManager();
			mLog = mLogMgr->createLog("meshmagick.log", true, false, true);
			mRoot = new Root();
			mResourceGroupMgr = ResourceGroupManager::getSingletonPtr();
			mMath = new Math();
			mMeshMgr = MeshManager::getSingletonPtr();
			mMeshMgr->setBoundsPaddingFactor(0.0f);
#if OGRE_VERSION_MAJOR > 1 || (OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR >= 7)
			mLodStrategyMgr = new LodStrategyManager();
#endif
			mMaterialMgr = MaterialManager::getSingletonPtr();
			mMaterialMgr->initialise();
			mSkeletonMgr = SkeletonManager::getSingletonPtr();
			mBufferManager = new DefaultHardwareBufferManager();
			mStandalone = true;
		}
		else
		{
			mLog = log;
			mStandalone = false;
		}

		mMeshSerializer = new StatefulMeshSerializer();
        mSkeletonSerializer = new StatefulSkeletonSerializer();
	}

	bool OgreEnvironment::isStandalone() const
	{
		return mStandalone;
	}

	Ogre::Log* OgreEnvironment::getLog() const
	{
		return mLog;
	}

    StatefulMeshSerializer* OgreEnvironment::getMeshSerializer() const
    {
        return mMeshSerializer;
    }

    StatefulSkeletonSerializer* OgreEnvironment::getSkeletonSerializer() const
    {
        return mSkeletonSerializer;
    }
}
