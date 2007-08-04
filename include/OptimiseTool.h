/*
This file is part of MeshMagick - An Ogre mesh file manipulation tool.
Copyright (C) 2007 - Daniel Wickert, Steve Streeting

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

#ifndef __MM_OPTIMISE_TOOL_H__
#define __MM_OPTIMISE_TOOL_H__

#include "MeshMagickPrerequisites.h"
#include "Tool.h"
#include "OptionsParser.h"

#include <OgreMeshSerializer.h>
#include <OgreMesh.h>
#include <OgreSubMesh.h>
#include <OgreVector3.h>
#include <OgreVector4.h>

namespace meshmagick
{
	class OptimiseTool : public Tool
	{
	public:
		OptimiseTool();

	protected:
		float mTolerance;
		bool mKeepIdentityTracks;

		void processMeshFile(Ogre::String file, Ogre::String outFile);
		void processSkeletonFile(Ogre::String file, Ogre::String outFile);

		void processMesh(Ogre::MeshPtr mesh);
		void processSkeleton(Ogre::SkeletonPtr skeleton);

		/** Mapping from original vertex index to new (potentially shared) vertex index */
		typedef std::vector<Ogre::uint32> IndexRemap;
		IndexRemap mIndexRemap;

		struct UniqueVertex
		{
			Ogre::Vector3 position;
			Ogre::Vector3 normal;
			Ogre::Vector4 tangent;
			Ogre::Vector3 uv[OGRE_MAX_TEXTURE_COORD_SETS];

			UniqueVertex()
				: position(Ogre::Vector3::ZERO), normal(Ogre::Vector3::ZERO), tangent(Ogre::Vector4::ZERO)
			{
				memset(uv, 0, sizeof(Ogre::Vector3) * OGRE_MAX_TEXTURE_COORD_SETS);
			}

		};
		struct UniqueVertexLess
		{
			float tolerance;
			unsigned short uvSets;
			bool operator()(const UniqueVertex& a, const UniqueVertex& b) const;

			bool equals(const Ogre::Vector3& a, const Ogre::Vector3& b) const;
			bool equals(const Ogre::Vector4& a, const Ogre::Vector4& b) const;
			bool less(const Ogre::Vector3& a, const Ogre::Vector3& b) const;
			bool less(const Ogre::Vector4& a, const Ogre::Vector4& b) const;
		};

		/** Map used to efficiently look up vertices that have the same components.
		The second element is the source vertex index.
		*/
		typedef std::map<UniqueVertex, Ogre::uint32, UniqueVertexLess> UniqueVertexMap;
		UniqueVertexMap mUniqueVertexMap;

		Ogre::VertexData* mTargetVertexData;
		typedef std::list<Ogre::IndexData*> IndexDataList;
		IndexDataList mIndexDataList;

		void setTargetVertexData(Ogre::VertexData* vd);
		void addIndexData(Ogre::IndexData* id);
		bool optimiseGeometry();
		bool calculateDuplicateVertices();
		void rebuildVertexBuffers();
		void remapIndexDataList();
		void remapIndexes(Ogre::IndexData* idata);
		Ogre::Mesh::VertexBoneAssignmentList getAdjustedBoneAssignments(
			Ogre::Mesh::BoneAssignmentIterator& it);
		void fixLOD(Ogre::ProgressiveMesh::LODFaceList lodFaces);

		void doInvoke(const OptionList& toolOptions,
			const Ogre::StringVector& inFileNames, const Ogre::StringVector& outFileNames);
	};
}
#endif
