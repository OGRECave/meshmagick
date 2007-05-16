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

#include <OgreAnimation.h>
#include <OgreStringConverter.h>

#include <algorithm>
#include <functional>

using namespace Ogre;

namespace meshmagick
{
    struct FindSubMeshNameByIndex
        : public std::binary_function<Mesh::SubMeshNameMap::value_type, unsigned short, bool>
    {
        bool operator()(const Mesh::SubMeshNameMap::value_type& entry, unsigned short index) const
        {
            return entry.second == index;
        }
    };

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
        }
        else
        {
            print("No shared vertices.");
        }

        print(" ");

        const Mesh::SubMeshNameMap& subMeshNames = mesh->getSubMeshNameMap();
        for(int i = 0;i < mesh->getNumSubMeshes();i++)
        {
            // Has the submesh got a name?
            Mesh::SubMeshNameMap::const_iterator it = std::find_if(subMeshNames.begin(),
                subMeshNames.end(), std::bind2nd(FindSubMeshNameByIndex(), i));

            String name = it == subMeshNames.end() ? String("unnamed") : it->first;
            print("Submesh " + StringConverter::toString(i) + " (" + name + ") : ");
            processSubMesh(mesh->getSubMesh(i));
        }

        print(" ");

        // Animation detection

        // Morph animations ?
        if (mesh->getNumAnimations() > 0)
        {
            // Yes, list them
            for (unsigned short i = 0, end = mesh->getNumAnimations(); i < end; ++i)
            {
                Animation* ani = mesh->getAnimation(i);
                print("Morph animation " + ani->getName() + " with length of " +
                    StringConverter::toString(ani->getLength()) + " seconds.");
            }
        }
        else
        {
            print("Mesh does not have morph animations.");
        }

        // Poses?
        PoseList poses = mesh->getPoseList();
        if (!poses.empty())
        {
            print("Mesh has " + StringConverter::toString(poses.size()) + " poses.");
        }
        else
        {
            print("Mesh does not have poses.");
        }

        // Is there a skeleton linked and are we supposed to follow it?
        if (mFollowSkeletonLink && mesh->hasSkeleton())
        {
            processSkeleton(mesh->getSkeletonName());
        }
    }
    //------------------------------------------------------------------------

    void InfoTool::processSubMesh(Ogre::SubMesh* submesh) const
    {
        // vertices
        if (submesh->useSharedVertices)
        {
            print("    shared vertex data used.");
        }
        else if (submesh->vertexData != NULL)
        {
            print("    " +
                StringConverter::toString(submesh->vertexData->vertexCount) + " vertices.");
        }

        // indices
        if (submesh->indexData != NULL)
        {
            HardwareIndexBufferSharedPtr indexBuffer = submesh->indexData->indexBuffer;
            print("    " + StringConverter::toString(indexBuffer->getNumIndexes() / 3) +
                " triangles.");
            if (indexBuffer->getType() == HardwareIndexBuffer::IT_16BIT)
            {
                print("    16 bit index buffer");
            }
            else
            {
                print("    32 bit index buffer");
            }
        }
    }
    //------------------------------------------------------------------------

    void InfoTool::processSkeleton(const String& skeletonFileName) const
    {
        StatefulSkeletonSerializer* skeletonSerializer =
            OgreEnvironment::getSingleton().getSkeletonSerializer();

        SkeletonPtr skeleton;
        try
        {
            skeleton = skeletonSerializer->loadSkeleton(skeletonFileName);
        }
        catch(std::exception& e)
        {
            warn(e.what());
            warn("Unable to open skeleton file " + skeletonFileName);
            return;
        }

        print("Analysing skeletonfile " + skeletonFileName + "...");

        print("Skeleton has " + StringConverter::toString(skeleton->getNumBones()) + " bones.");
        print("Skeleton has " + StringConverter::toString(skeleton->getNumAnimations())
            + " animations.");
        for (unsigned short i = 0, end = skeleton->getNumAnimations(); i < end; ++i)
        {
            Animation* ani = skeleton->getAnimation(i);
            print("    " + ani->getName() + " with length of " +
                StringConverter::toString(ani->getLength()) + " seconds.");
        }
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