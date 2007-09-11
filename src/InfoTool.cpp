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

#include "MeshUtils.h"
#include "OgreEnvironment.h"
#include "StatefulMeshSerializer.h"
#include "StatefulSkeletonSerializer.h"
#include "ToolUtils.h"

#include <OgreAnimation.h>
#include <OgreBone.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreStringConverter.h>

#include <algorithm>
#include <functional>
#include <map>

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
            warn("info tool doesn't write anything. Output files are ignored.");
        }

        for (size_t i = 0, end = inFileNames.size(); i < end; ++i)
        {
            if (StringUtil::endsWith(inFileNames[i], ".mesh", true))
            {
                processMesh(inFileNames[i]);
            }
            else if (StringUtil::endsWith(inFileNames[i], ".skeleton", true))
            {
                processSkeleton(inFileNames[i]);
            }
            else
            {
                warn("unrecognised name ending for file " + inFileNames[i]);
                warn("file skipped.");
            }
        }
    }
    //------------------------------------------------------------------------

    void InfoTool::processMesh(const String& meshFileName) const
    {
        StatefulMeshSerializer* meshSerializer =
            OgreEnvironment::getSingleton().getMeshSerializer();

        MeshPtr mesh;
        try
        {
            mesh = meshSerializer->loadMesh(meshFileName);
        }
        catch(std::exception& e)
        {
            warn(e.what());
            warn("Unable to open mesh file " + meshFileName);
            warn("file skipped.");
            return;
        }

        print("Analysing meshfile " + meshFileName + "...");
        print("File version : " + meshSerializer->getMeshFileVersion());
        print("Endian mode  : " + getEndianModeAsString(meshSerializer->getEndianMode()));
        print(" ");

        AxisAlignedBox meshAabb = mesh->getBounds();
        AxisAlignedBox actualAabb = MeshUtils::getMeshAabb(mesh);
        // If AABB set to the mesh and actual AABB are equal only print one, else print both
        if (meshAabb == actualAabb)
        {
            print("Bounding box: " + ToolUtils::getPrettyAabbString(meshAabb));
        }
        else
        {
            print("Mesh bounding box: " + ToolUtils::getPrettyAabbString(meshAabb));
            print("Actual bounding box: " + ToolUtils::getPrettyAabbString(actualAabb));
        }
        // Build metadata for bone assignments
		if (mesh->hasSkeleton())
		{
			// Cause mesh to sort out the number of bone assignments per vertex and
			// the bone map to individual submeshes
			mesh->_updateCompiledBoneAssignments();
		}

        if (mesh->sharedVertexData != NULL)
        {
            print("Shared vertex data with " +
                StringConverter::toString(mesh->sharedVertexData->vertexCount) + " vertices");
			reportBoneAssignmentData(mesh->sharedVertexData, 
				mesh->sharedBlendIndexToBoneIndexMap, Ogre::StringUtil::BLANK);
            reportVertexDeclaration(mesh->sharedVertexData->vertexDeclaration,
                Ogre::StringUtil::BLANK);
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
		print("    Default material: " + submesh->getMaterialName());
        // vertices
        if (submesh->useSharedVertices)
        {
            print("    shared vertex data used.");
        }
        else if (submesh->vertexData != NULL)
        {
            print("    " +
                StringConverter::toString(submesh->vertexData->vertexCount) + " vertices.");
			reportBoneAssignmentData(submesh->vertexData, submesh->blendIndexToBoneIndexMap, 
				"    ");
            reportVertexDeclaration(submesh->vertexData->vertexDeclaration, "    ");
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
            warn("file skipped.");
            return;
        }

        print("Analysing skeletonfile " + skeletonFileName + "...");

        print("Skeleton has " + StringConverter::toString(skeleton->getNumBones()) + " bones.");
        if (mVerbosity == V_HIGH)
        {
            print("Bone names:", V_HIGH);
            for (unsigned short i = 0, end = skeleton->getNumBones(); i < end; ++i)
            {
                Bone* bone = skeleton->getBone(i);
                print("    " + bone->getName(), V_HIGH);
            }
            print(" ", V_HIGH);
        }

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
	void InfoTool::reportBoneAssignmentData(const Ogre::VertexData* vd, 
		const Ogre::Mesh::IndexMap& blendIndexToBoneIndexMap, const Ogre::String& indent) const
	{
		// Report number of bones per vertex
		const Ogre::VertexElement* elem = 
			vd->vertexDeclaration->findElementBySemantic(VES_BLEND_WEIGHTS);
		if (elem)
		{
			unsigned short numWeights = VertexElement::getTypeCount(elem->getType());
			print(indent + "Number of bone assignments per vertex: "
                + StringConverter::toString(numWeights));

			// Report number of bones referenced by this vertex data
			print(indent + "Total number of bones referenced: " + 
				StringConverter::toString(blendIndexToBoneIndexMap.size()));
		}
	}

	/// @todo externalise this function, when reorganise-tool is integrated,
    /// because both use the same format
    void InfoTool::reportVertexDeclaration(const VertexDeclaration* vd, const String& indent) const
    {
        // First: source-ID, second: offset
        typedef std::pair<unsigned short, size_t> ElementPosition;
        typedef std::pair<VertexElementSemantic, VertexElementType> Element;
        typedef std::map<ElementPosition, Element> ElementMap;

        // This map holds the results from iterating all elements in the declaration.
        // Having them stored in the map makes it easy to create the layout string.
        ElementMap elements;

        // Iterate over declaration elements and put them into the map.
        // We do this, because we don't know in what order the elements are stored, but
        // in order to create the layout string we need them in order of their source and offset.
        const VertexDeclaration::VertexElementList& elementList = vd->getElements();
        for (VertexDeclaration::VertexElementList::const_iterator it = elementList.begin(),
            end = elementList.end(); it != end; ++it)
        {
            elements[std::make_pair((*it).getSource(), (*it).getOffset())] = 
                std::make_pair((*it).getSemantic(), (*it).getType());
        }

        // Create the layout string
        String layout;
        unsigned short source = 0;
        for (ElementMap::const_iterator it = elements.begin(), end = elements.end();
            it != end; ++it)
        {
            // If source changed, we append a hyphen to indicate a new buffer.
            if (it->first.first != source)
            {
                layout += '-';
                source = it->first.first;
            }

            // Append char indicating the element semantic
            switch (it->second.first)
            {
            case VES_POSITION:
                layout += "p";
                break;
            case VES_BLEND_WEIGHTS:
                layout += "w";
                break;
            case VES_BLEND_INDICES:
                layout += "i";
                break;
            case VES_NORMAL:
                layout += "n";
                break;
            case VES_DIFFUSE:
                layout += "d";
                break;
            case VES_SPECULAR:
                layout += "s";
                break;
            case VES_TEXTURE_COORDINATES:
                layout += "u";
                break;
            case VES_BINORMAL:
                layout += "b";
                break;
            case VES_TANGENT:
                layout += "t";
                break;
            }
            // Append substring indicating the element type
            switch (it->second.second)
            {
            case VET_FLOAT1:
                layout += "(f1)";
                break;
            case VET_FLOAT2:
                layout += "(f2)";
                break;
            case VET_FLOAT3:
                layout += "(f3)";
                break;
            case VET_FLOAT4:
                layout += "(f4)";
                break;
            case VET_SHORT1:
                layout += "(s1)";
                break;
            case VET_SHORT2:
                layout += "(s2)";
                break;
            case VET_SHORT3:
                layout += "(s3)";
                break;
            case VET_SHORT4:
                layout += "(s4)";
                break;
            case VET_UBYTE4:
                layout += "(u4)";
                break;
            case VET_COLOUR_ARGB:
                layout += "(dx)";
                break;
            case VET_COLOUR_ABGR:
                layout += "(gl)";
                break;
            }
        }

        // and print it
        print(indent + "buffer layout: " + layout);
    }
}
