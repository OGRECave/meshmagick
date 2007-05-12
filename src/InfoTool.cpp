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

#include "InfoTool.h"

#include "OgreEnvironment.h"
#include "StatefulMeshSerializer.h"
#include "StatefulSkeletonSerializer.h"

#include <OgreStringConverter.h>
#include <OgreSubmesh.h>

using namespace Ogre;

namespace meshmagick
{
    //------------------------------------------------------------------------
    InfoTool::InfoTool()
    {
    }
    //------------------------------------------------------------------------

    void InfoTool::doInvoke(const OptionList& toolOptions,
        const Ogre::StringVector& inFileNames, const Ogre::StringVector& outFileNames)
    {
        // info tool doesn't write anything. Warn, if outfiles given.
        if (!outFileNames.empty())
        {
            warn("info tool doesn't write anything. output files are ignored.");
        }

        for (size_t i = 0, end = inFileNames.size(); i < end; ++i)
        {
            processMesh(inFileNames[i]);
        }
    }
    //------------------------------------------------------------------------

    void InfoTool::processMesh(const String& meshFileName) const
    {
        StatefulMeshSerializer* meshSerializer =
            OgreEnvironment::getSingleton().getMeshSerializer();
        MeshPtr mesh = meshSerializer->loadMesh(meshFileName);

        print("Analysing meshfile " + meshFileName + "...");
        print("File version : " + meshSerializer->getMeshFileVersion());
        print("Endian mode  : " + getEndianModeAsString(meshSerializer->getEndianMode()));
        print(" ");

        if (mesh->sharedVertexData != NULL)
        {
            print("Shared vertex data with " +
                StringConverter::toString(mesh->sharedVertexData->vertexCount) + " vertices");
            if (mVerbosity == V_HIGH)
            {
                processVertexData(mesh->sharedVertexData);
            }
        }
        else
        {
            print("No shared vertices.");
        }

        for(int i = 0;i < mesh->getNumSubMeshes();i++)
        {
            SubMesh* submesh = mesh->getSubMesh(i);
            print("Submesh " + StringConverter::toString(i) + " with " +
                StringConverter::toString(submesh->vertexData->vertexCount) + " vertices");
            print("    Material: " + submesh->getMaterialName());
            if (mVerbosity == V_HIGH)
            {
                processVertexData(submesh->vertexData);
            }
        }
    }
    //------------------------------------------------------------------------

    void InfoTool::processVertexData(const Ogre::VertexData* vd) const
    {
    }
    //------------------------------------------------------------------------

    void InfoTool::processSkeleton(const String& skeletonFileName) const
    {
        StatefulSkeletonSerializer* skeletonSerializer =
            OgreEnvironment::getSingleton().getSkeletonSerializer();
        print("Analysing skeletonfile " + skeletonFileName + "...");
    }
    //------------------------------------------------------------------------

    String InfoTool::getEndianModeAsString(MeshSerializer::Endian endian) const
    {
        if (endian == MeshSerializer::ENDIAN_BIG)
        {
            return "Big Endian";
        }
        else if (endian == MeshSerializer::ENDIAN_LITTLE)
        {
            return "Little Endian";
        }
        else if (endian == MeshSerializer::ENDIAN_NATIVE)
        {
            return "Native Endian";
        }
        else
        {
            return "Unknown Endian";
        }
    }
    //------------------------------------------------------------------------
}
