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
        : std::binary_function<Mesh::SubMeshNameMap::value_type, unsigned short, bool>
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
				MeshInfo meshInfo = processMesh(inFileNames[i]);
				printMeshInfo(toolOptions, meshInfo);
            }
            else if (StringUtil::endsWith(inFileNames[i], ".skeleton", true))
            {
				SkeletonInfo skeletonInfo = processSkeleton(inFileNames[i]);
				printSkeletonInfo(toolOptions, skeletonInfo);
            }
            else
            {
                warn("unrecognised name ending for file " + inFileNames[i]);
                warn("file skipped.");
            }
        }
    }
    //------------------------------------------------------------------------

    MeshInfo InfoTool::processMesh(const String& meshFileName) const
    {
        StatefulMeshSerializer* meshSerializer =
            OgreEnvironment::getSingleton().getMeshSerializer();

        MeshPtr mesh = meshSerializer->loadMesh(meshFileName);

		MeshInfo rval;
		rval.name = meshFileName;
		rval.version = meshSerializer->getMeshFileVersion();
		rval.endian = getEndianModeAsString(meshSerializer->getEndianMode());

        rval.storedBoundingBox = mesh->getBounds();
		rval.actualBoundingBox = MeshUtils::getMeshAabb(mesh);

        // Build metadata for bone assignments
		if (mesh->hasSkeleton())
		{
			// Cause mesh to sort out the number of bone assignments per vertex and
			// the bone map to individual submeshes
			mesh->_updateCompiledBoneAssignments();
		}

        if (mesh->sharedVertexData != NULL)
        {
			rval.hasSharedVertices = true;
			rval.sharedVertices.numVertices = mesh->sharedVertexData->vertexCount;
			processBoneAssignmentData(rval.sharedVertices, mesh->sharedVertexData, 
				mesh->sharedBlendIndexToBoneIndexMap);
            processVertexDeclaration(rval.sharedVertices, mesh->sharedVertexData->vertexDeclaration);
        }
        else
        {
			rval.hasSharedVertices = false;
        }

        const Mesh::SubMeshNameMap& subMeshNames = mesh->getSubMeshNameMap();
        for(int i = 0;i < mesh->getNumSubMeshes();i++)
        {
			SubMeshInfo subMeshInfo;
            // Has the submesh got a name?
            Mesh::SubMeshNameMap::const_iterator it = std::find_if(subMeshNames.begin(),
                subMeshNames.end(), std::bind2nd(FindSubMeshNameByIndex(), i));

            subMeshInfo.name = it == subMeshNames.end() ? String() : it->first;
            processSubMesh(subMeshInfo, mesh->getSubMesh(i));
			rval.submeshes.push_back(subMeshInfo);
        }

        // Animation detection

        // Morph animations ?
        if (mesh->getNumAnimations() > 0)
        {
            // Yes, list them
            for (unsigned short i = 0, end = mesh->getNumAnimations(); i < end; ++i)
            {
                Animation* ani = mesh->getAnimation(i);
				rval.morphAnimations.push_back(std::make_pair(ani->getName(), ani->getLength()));
            }
        }

        // Poses?
        PoseList poses = mesh->getPoseList();
		for (size_t i = 0; i < poses.size(); ++i)
		{
			rval.poseNames.push_back(poses[i]->getName());
		}

        // Is there a skeleton linked and are we supposed to follow it?
        if (mFollowSkeletonLink && mesh->hasSkeleton())
        {
			rval.hasSkeleton = true;
            rval.skeleton = processSkeleton(mesh->getSkeletonName());
        }
		else
		{
			rval.hasSkeleton = false;
		}

		return rval;
    }
    //------------------------------------------------------------------------

    void InfoTool::processSubMesh(SubMeshInfo& info, Ogre::SubMesh* submesh) const
    {
		info.materialName = submesh->getMaterialName();
		info.usesSharedVertices = submesh->useSharedVertices;
        if (!info.usesSharedVertices)
        {
			info.vertices.numVertices = submesh->vertexData->vertexCount;
			processBoneAssignmentData(info.vertices, submesh->vertexData, submesh->blendIndexToBoneIndexMap);
            processVertexDeclaration(info.vertices, submesh->vertexData->vertexDeclaration);
        }

        // indices
        if (submesh->indexData != NULL)
        {
            HardwareIndexBufferSharedPtr indexBuffer = submesh->indexData->indexBuffer;
            if (indexBuffer->getType() == HardwareIndexBuffer::IT_16BIT)
            {
				info.indexBitWidth = 16;
            }
            else
            {
				info.indexBitWidth = 32;
            }

			size_t numIndices = indexBuffer->getNumIndexes();
			switch(submesh->operationType)
			{
			case RenderOperation::OT_LINE_LIST:
				info.operationType = "OT_LINE_LIST";
				info.numElements = numIndices / 2;
				info.elementType = "lines";
				break;
			case RenderOperation::OT_LINE_STRIP:
				info.operationType = "OT_LINE_STRIP";
				info.numElements = numIndices / 2;
				info.elementType = "lines";
				break;
			case RenderOperation::OT_POINT_LIST:
				info.operationType = "OT_POINT_LIST";
				info.numElements = numIndices;
				info.elementType = "points";
				break;
			case RenderOperation::OT_TRIANGLE_FAN:
				info.operationType = "OT_TRIANGLE_FAN";
				info.numElements = numIndices - 2;
				info.elementType = "triangles";
				break;
			case RenderOperation::OT_TRIANGLE_LIST:
				info.operationType = "OT_TRIANGLE_LIST";
				info.numElements = numIndices / 3;
				info.elementType = "triangles";
				break;
			case RenderOperation::OT_TRIANGLE_STRIP:
				info.operationType = "OT_TRIANGLE_STRIP";
				info.numElements = numIndices - 2;
				info.elementType = "triangles";
				break;
			}
        }
    }
    //------------------------------------------------------------------------

    SkeletonInfo InfoTool::processSkeleton(const String& skeletonFileName) const
    {
		SkeletonInfo rval;

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
			throw;
        }

		rval.name = skeletonFileName;

        for (unsigned short i = 0, end = skeleton->getNumBones(); i < end; ++i)
        {
            Bone* bone = skeleton->getBone(i);
			rval.boneNames.push_back(bone->getName());
        }

        for (unsigned short i = 0, end = skeleton->getNumAnimations(); i < end; ++i)
        {
            Animation* ani = skeleton->getAnimation(i);
			rval.animations.push_back(std::make_pair(ani->getName(), ani->getLength()));
        }

		return rval;
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

	void InfoTool::processBoneAssignmentData(VertexInfo& info, const Ogre::VertexData* vd,
		const Ogre::Mesh::IndexMap& blendIndexToBoneIndexMap) const
	{
		// Report number of bones per vertex
		const Ogre::VertexElement* elem = 
			vd->vertexDeclaration->findElementBySemantic(VES_BLEND_WEIGHTS);
		if (elem)
		{
			info.numBoneAssignments = VertexElement::getTypeCount(elem->getType());
			info.numBonesReferenced = blendIndexToBoneIndexMap.size();
		}
	}
    //------------------------------------------------------------------------

	/// @todo externalise this function, when reorganise-tool is integrated,
    /// because both use the same format
    void InfoTool::processVertexDeclaration(VertexInfo& info, const VertexDeclaration* vd) const
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

		info.layout = layout;
    }
    //------------------------------------------------------------------------

	void InfoTool::printMeshInfo(const OptionList& toolOptions, const MeshInfo& info) const
	{
		if (OptionsUtil::getStringOption(toolOptions, "list") == StringUtil::BLANK)
		{
			reportMeshInfo(info);
		}
	}
    //------------------------------------------------------------------------

	void InfoTool::printSkeletonInfo(const OptionList& toolOptions, const SkeletonInfo& info) const
	{
		if (OptionsUtil::getStringOption(toolOptions, "list") == StringUtil::BLANK)
		{
			reportSkeletonInfo(info);
		}
	}
    //------------------------------------------------------------------------

	void InfoTool::reportMeshInfo(const MeshInfo& meshInfo) const
	{
		// total amount counters
		size_t numVertices = 0;
		size_t numTriangles = 0;
		size_t numLines = 0;
		size_t numPoints = 0;
		
		// formatting helpers
		const String& indent = "    ";

		// header info
		print("Mesh file name: " + meshInfo.name);
		print("Mesh file version: " + meshInfo.version);
		print("Endian mode: " + meshInfo.endian);
		print("");

		// bounding box(es)
		if (meshInfo.actualBoundingBox == meshInfo.storedBoundingBox)
		{
			print("Bounding box: "
				+ ToolUtils::getPrettyAabbString(meshInfo.actualBoundingBox));
		}
		else
		{
			print("Stored bounding box: "
				+ ToolUtils::getPrettyAabbString(meshInfo.storedBoundingBox));
			print("Actual bounding box: "
				+ ToolUtils::getPrettyAabbString(meshInfo.actualBoundingBox));
		}
		print("");

		// shared vertices
		if (meshInfo.hasSharedVertices)
		{
			print("Shared vertices:");
			print(indent + StringConverter::toString(meshInfo.sharedVertices.numVertices)
				+ " vertices");
			print(indent
				+ StringConverter::toString(meshInfo.sharedVertices.numBonesReferenced)
				+ " bones referenced.");
			print(indent
				+ StringConverter::toString(meshInfo.sharedVertices.numBoneAssignments)
				+ " bone assignments per vertex.");
			print(indent + "Buffer layout: " + meshInfo.sharedVertices.layout);

			numVertices += meshInfo.sharedVertices.numVertices;
		}
		else
		{
			print("No shared vertices.");
		}
		print("");

		// submesh info
		const size_t numSubmeshes = meshInfo.submeshes.size();
		print(StringConverter::toString(numSubmeshes)
			+ (numSubmeshes > 1 ? " submeshes." : "submesh."));
		for (size_t i = 0; i < numSubmeshes; ++i)
		{
			const SubMeshInfo& info = meshInfo.submeshes[i];

			print("submesh " + StringConverter::toString(i) + "(" + info.name + ")");
			print(indent + "material " + info.materialName);
			if (info.usesSharedVertices)
			{
				print(indent + "submesh uses shared vertices.");
			}
			else
			{
				print(indent + StringConverter::toString(info.vertices.numVertices)
					+ " vertices");
				print(indent + StringConverter::toString(info.vertices.numBonesReferenced)
					+ " bones referenced.");
				print(indent + StringConverter::toString(info.vertices.numBoneAssignments)
					+ " bone assignments per vertex.");
				print(indent + "Buffer layout: " + info.vertices.layout);

				numVertices += info.vertices.numVertices;
			}

			print(indent + "OperationType: " + info.operationType);
			print(indent + StringConverter::toString(info.numElements)
				+ " " + info.elementType);
			print(indent + StringConverter::toString(info.indexBitWidth) + " bit index width");

			// Discriminate element type for total element counts
			if (info.elementType == "triangles")
			{
				numTriangles += info.numElements;
			}
			else if (info.elementType == "lines")
			{
				numLines += info.numElements;
			}
			else if (info.elementType == "points")
			{
				numPoints += info.numElements;
			}
			print("");
		}

		// Print total counts
		print(StringConverter::toString(numVertices) + " vertices in total.");
		if (numTriangles > 0)
		{
			print(StringConverter::toString(numTriangles) + " triangles in total.");
		}
		if (numLines > 0)
		{
			print(StringConverter::toString(numLines) + " lines in total.");
		}
		if (numPoints > 0)
		{
			print(StringConverter::toString(numPoints) + " points in total.");
		}
		print("");

		// Other mesh properties

		if (meshInfo.hasEdgeList)
		{
			print("Edge list stored in file.");
		}
		else
		{
			print("No edge list stored in file.");
		}

		if (meshInfo.numLodLevels > 0)
		{
			print(StringConverter::toString(meshInfo.numLodLevels)
				+ " LOD levels stored in file.");
		}
		else
		{
			print("No LOD info stored in file.");
		}
		print("");

		if (meshInfo.morphAnimations.empty())
		{
			print("No morph animations");
		}
		else
		{
			print(StringConverter::toString(meshInfo.morphAnimations.size())
				+ " morph animations");
			for (size_t i = 0; i < meshInfo.morphAnimations.size(); ++i)
			{
				std::pair<Ogre::String, Ogre::Real> ani = meshInfo.morphAnimations[i];
				print(indent + "name: " + ani.first
					+ " / length: " + StringConverter::toString(ani.second));
			}
			print("");
		}

		if (meshInfo.poseNames.empty())
		{
			print("No poses.");
		}
		else
		{
			print(StringConverter::toString(meshInfo.poseNames.size())
				+ " poses.");
			for (size_t i = 0; i < meshInfo.poseNames.size(); ++i)
			{
				print(indent + meshInfo.poseNames[i]);
			}
		}
		print("");
		print("");

		if (meshInfo.hasSkeleton)
		{
			reportSkeletonInfo(meshInfo.skeleton);
		}
	}
    //------------------------------------------------------------------------

	void InfoTool::reportSkeletonInfo(const SkeletonInfo& info) const
	{
		const String& indent = "    ";
		print("Skeleton file name: " + info.name);
		print("");

		print(StringConverter::toString(info.boneNames.size()) + " bones");
		for (size_t i = 0; i < info.animations.size(); ++i)
		{
			print(indent + info.boneNames[i]);
		}
		print("");

		print(StringConverter::toString(info.animations.size()) + " animations");
		for (size_t i = 0; i < info.animations.size(); ++i)
		{
			print(indent + "name: " + info.animations[i].first + " / length: "
				+ StringConverter::toString(info.animations[i].second));
		}
	}
    //------------------------------------------------------------------------
}
