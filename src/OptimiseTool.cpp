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

#include "OptimiseTool.h"

#include "OgreEnvironment.h"
#include "StatefulMeshSerializer.h"
#include "StatefulSkeletonSerializer.h"

#include <OgreStringConverter.h>

#include <algorithm>
#include <functional>

using namespace Ogre;

namespace meshmagick
{
	//------------------------------------------------------------------------
	OptimiseTool::OptimiseTool()
	{
	}
	//------------------------------------------------------------------------
	void OptimiseTool::doInvoke(const OptionList& toolOptions,
		const Ogre::StringVector& inFileNames, const Ogre::StringVector& outFileNamesArg)
	{
		// Name count has to match, else we have no way to figure out how to apply output
		// names to input files.
		if (!(outFileNamesArg.empty() || inFileNames.size() == outFileNamesArg.size()))
		{
			fail("number of output files must match number of input files.");
		}

		mTolerance = 1e-06f;
		mKeepIdentityTracks = OptionsUtil::isOptionSet(toolOptions, "keep-identity-tracks");
		for (OptionList::const_iterator it = toolOptions.begin(); it != toolOptions.end(); ++it)
		{
			if (it->first == "tolerance")
			{
				mTolerance = static_cast<float>(any_cast<Real>(it->second));
			}
		}


		StringVector outFileNames = outFileNamesArg.empty() ? inFileNames : outFileNamesArg;

		// Process the meshes
		for (size_t i = 0, end = inFileNames.size(); i < end; ++i)
		{
			if (StringUtil::endsWith(inFileNames[i], ".mesh", true))
			{
				processMeshFile(inFileNames[i], outFileNames[i]);
			}
			else if (StringUtil::endsWith(inFileNames[i], ".skeleton", true))
			{
				processSkeletonFile(inFileNames[i], outFileNames[i]);
			}
			else
			{
				warn("unrecognised name ending for file " + inFileNames[i]);
				warn("file skipped.");
			}
		}
	}
	//---------------------------------------------------------------------
	void OptimiseTool::processMeshFile(Ogre::String file, Ogre::String outFile)
	{
		StatefulMeshSerializer* meshSerializer =
			OgreEnvironment::getSingleton().getMeshSerializer();

		print("Loading mesh " + file + "...");
		MeshPtr mesh;
		try
		{
			mesh = meshSerializer->loadMesh(file);
		}
		catch(std::exception& e)
		{
			warn(e.what());
			warn("Unable to open mesh file " + file);
			warn("file skipped.");
			return;
		}
		print("Optimising mesh...");
		processMesh(mesh);
		meshSerializer->saveMesh(outFile, mKeepVersion, true);
		print("Mesh saved as " + outFile + ".");

		if (mFollowSkeletonLink && mesh->hasSkeleton())
		{
			// In this case keep file name.
			processSkeletonFile(mesh->getSkeletonName(), mesh->getSkeletonName());
		}

	}
	//---------------------------------------------------------------------
	void OptimiseTool::processSkeletonFile(Ogre::String file, Ogre::String outFile)
	{
		StatefulSkeletonSerializer* skeletonSerializer =
			OgreEnvironment::getSingleton().getSkeletonSerializer();

		print("Loading skeleton " + file + "...");
		SkeletonPtr skeleton;
		try
		{
			skeleton = skeletonSerializer->loadSkeleton(file);
		}
		catch(std::exception& e)
		{
			warn(e.what());
			warn("Unable to open skeleton file " + file);
			warn("file skipped.");
			return;
		}
		print("Optimising skeleton...");
		processSkeleton(skeleton);
		skeletonSerializer->saveSkeleton(outFile, true);
		print("Skeleton saved as " + outFile + ".");

	}
	//---------------------------------------------------------------------
	void OptimiseTool::processMesh(Ogre::MeshPtr mesh)
	{
		bool rebuildEdgeList = false;
		// Shared geometry
		if (mesh->sharedVertexData)
		{
			print("Optimising mesh shared vertex data...");
			setTargetVertexData(mesh->sharedVertexData);

			for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
			{
				SubMesh* sm = mesh->getSubMesh(i);
				if (sm->useSharedVertices)
				{
					addIndexData(sm->indexData);
				}
			}
			
			if (optimiseGeometry())
			{
				if (mesh->hasSkeleton())
				{
					print("    fixing bone assignments...");
					Mesh::BoneAssignmentIterator currentIt = mesh->getBoneAssignmentIterator();
					Mesh::VertexBoneAssignmentList newList = 
						getAdjustedBoneAssignments(currentIt);
					mesh->clearBoneAssignments();
					for (Mesh::VertexBoneAssignmentList::iterator bi = newList.begin();
						bi != newList.end(); ++bi)
					{
						mesh->addBoneAssignment(bi->second);
					}

				}

				for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
				{
					SubMesh* sm = mesh->getSubMesh(i);
					if (sm->useSharedVertices)
					{
						fixLOD(sm->mLodFaceList);
					}
				}
				rebuildEdgeList = true;

			}
		}

		// Dedicated geometry
		for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
		{
			SubMesh* sm = mesh->getSubMesh(i);
			if (!sm->useSharedVertices)
			{
				print("Optimising submesh " + StringConverter::toString(i) + " dedicated vertex data ");
				setTargetVertexData(sm->vertexData);
				addIndexData(sm->indexData);
				if (optimiseGeometry())
				{
					if (mesh->hasSkeleton())
					{
						print("    fixing bone assignments...");
						Mesh::BoneAssignmentIterator currentIt = sm->getBoneAssignmentIterator();
						Mesh::VertexBoneAssignmentList newList = 
							getAdjustedBoneAssignments(currentIt);
						sm->clearBoneAssignments();
						for (Mesh::VertexBoneAssignmentList::iterator bi = newList.begin();
							bi != newList.end(); ++bi)
						{
							sm->addBoneAssignment(bi->second);
						}

					}

					fixLOD(sm->mLodFaceList);
					rebuildEdgeList = true;
				}
			}
		}

		if (rebuildEdgeList && mesh->isEdgeListBuilt())
		{
			// force rebuild of edge list
			mesh->freeEdgeList();
			mesh->buildEdgeList();
		}


	}
	//---------------------------------------------------------------------
	void OptimiseTool::fixLOD(ProgressiveMesh::LODFaceList lodFaces)
	{
		for (ProgressiveMesh::LODFaceList::iterator l = lodFaces.begin();
			l != lodFaces.end(); ++l)
		{
			IndexData* idata = *l;
			remapIndexes(idata);
		}

	}
	//---------------------------------------------------------------------
	Mesh::VertexBoneAssignmentList OptimiseTool::getAdjustedBoneAssignments(
		Mesh::BoneAssignmentIterator& it)
	{
		Mesh::VertexBoneAssignmentList newList;
		while (it.hasMoreElements())
		{
			VertexBoneAssignment ass = it.getNext();
			// remap
			ass.vertexIndex = static_cast<unsigned int>(mIndexRemap[ass.vertexIndex]);
			newList.insert(Mesh::VertexBoneAssignmentList::value_type(
				ass.vertexIndex, it.getNext()));
		}

		return newList;

	}
	//---------------------------------------------------------------------
	void OptimiseTool::processSkeleton(Ogre::SkeletonPtr skeleton)
	{
		skeleton->optimiseAllAnimations(mKeepIdentityTracks);
	}
	//---------------------------------------------------------------------
	void OptimiseTool::setTargetVertexData(Ogre::VertexData* vd)
	{
		mTargetVertexData = vd;
		mUniqueVertexMap.clear();
		mIndexDataList.clear();
		mIndexRemap.clear();
	}
	//---------------------------------------------------------------------
	void OptimiseTool::addIndexData(Ogre::IndexData* id)
	{
		mIndexDataList.push_back(id);
	}
	//---------------------------------------------------------------------
	bool OptimiseTool::optimiseGeometry()
	{
		if (calculateDuplicateVertices())
		{
			size_t numDupes = mTargetVertexData->vertexCount - 
				mUniqueVertexMap.size();
			print("    " + StringConverter::toString(numDupes) + " duplicate vertices to be removed.");
			print("    rebuilding vertex buffers...");
			rebuildVertexBuffers();
			print("    re-indexing faces...");
			remapIndexDataList();
			print("    done.");
			return true;
		}
		else
		{
			print("    no optimisation required.");
			return false;
		}
	}
	//---------------------------------------------------------------------
	bool OptimiseTool::calculateDuplicateVertices()
	{
		bool duplicates = false;

		// Lock all the buffers first
		typedef std::vector<char*> BufferLocks;
		BufferLocks bufferLocks;
		const VertexBufferBinding::VertexBufferBindingMap& bindings = 
			mTargetVertexData->vertexBufferBinding->getBindings();
		VertexBufferBinding::VertexBufferBindingMap::const_iterator bindi;
		bufferLocks.resize(mTargetVertexData->vertexBufferBinding->getLastBoundIndex()+1);
		for (bindi = bindings.begin(); bindi != bindings.end(); ++bindi)
		{
			char* lock = static_cast<char*>(bindi->second->lock(HardwareBuffer::HBL_READ_ONLY));
			bufferLocks[bindi->first] = lock;
		}

		for (uint32 v = 0; v < mTargetVertexData->vertexCount; ++v)
		{
			UniqueVertex uniqueVertex;
			const VertexDeclaration::VertexElementList& elemList = 
				mTargetVertexData->vertexDeclaration->getElements();
			VertexDeclaration::VertexElementList::const_iterator elemi;
			unsigned short uvSets = 0;
			for (elemi = elemList.begin(); elemi != elemList.end(); ++elemi)
			{
				// all float pointers for the moment
				float *pFloat;
				elemi->baseVertexPointerToElement(
					bufferLocks[elemi->getSource()], &pFloat);

				switch(elemi->getSemantic())
				{
				case VES_POSITION:
					uniqueVertex.position.x = *pFloat++;
					uniqueVertex.position.y = *pFloat++;
					uniqueVertex.position.z = *pFloat++;
					break;
				case VES_NORMAL:
					uniqueVertex.normal.x = *pFloat++;
					uniqueVertex.normal.y = *pFloat++;
					uniqueVertex.normal.z = *pFloat++;
					break;
				case VES_TANGENT:
					uniqueVertex.tangent.x = *pFloat++;
					uniqueVertex.tangent.y = *pFloat++;
					uniqueVertex.tangent.z = *pFloat++;
					// support w-component on tangent if present
					if (VertexElement::getTypeCount(elemi->getType()) == 4)
					{
						uniqueVertex.tangent.w = *pFloat++;
					}
					break;
				case VES_TEXTURE_COORDINATES:
					// supports up to 4 dimensions
					for (unsigned short dim = 0; dim < VertexElement::getTypeCount(elemi->getType()); ++dim)
					{
						uniqueVertex.uv[elemi->getIndex()][dim] = *pFloat++;
					}
					++uvSets;
					break;

				};
			}

			if (v == 0)
			{
				// set up comparator
				UniqueVertexLess lessObj;
				lessObj.tolerance = mTolerance;
				lessObj.uvSets = uvSets;
				mUniqueVertexMap = UniqueVertexMap(lessObj);
			}

			// try to locate equivalent vertex in the list already
			uint32 indexUsed;
			UniqueVertexMap::iterator ui = mUniqueVertexMap.find(uniqueVertex);
			if (ui != mUniqueVertexMap.end())
			{
				// re-use vertex, remap
				indexUsed = ui->second;
				duplicates = true;
			}
			else
			{
				// new vertex
				indexUsed = static_cast<uint32>(mUniqueVertexMap.size());
				// store the originating vertex index in the unique map
				mUniqueVertexMap[uniqueVertex] = v;
			}
			// Insert remap entry (may map to itself)
			mIndexRemap.push_back(indexUsed);


			// increment buffer lock pointers
			for (bindi = bindings.begin(); bindi != bindings.end(); ++bindi)
			{
				bufferLocks[bindi->first] += bindi->second->getVertexSize();
			}

		}


		// unlock the buffers now
		for (bindi = bindings.begin(); bindi != bindings.end(); ++bindi)
		{
			bindi->second->unlock();
		}

		// Were there duplicates?
		return duplicates;

	}
	//---------------------------------------------------------------------
	void OptimiseTool::rebuildVertexBuffers()
	{
		// We need to build new vertex buffers of the new, reduced size
		VertexBufferBinding* newBind = 
			HardwareBufferManager::getSingleton().createVertexBufferBinding();

		// Lock source buffers
		typedef std::vector<char*> BufferLocks;
		BufferLocks srcbufferLocks;
		BufferLocks destbufferLocks;
		const VertexBufferBinding::VertexBufferBindingMap& srcBindings = 
			mTargetVertexData->vertexBufferBinding->getBindings();
		VertexBufferBinding::VertexBufferBindingMap::const_iterator bindi;
		srcbufferLocks.resize(mTargetVertexData->vertexBufferBinding->getLastBoundIndex()+1);
		destbufferLocks.resize(mTargetVertexData->vertexBufferBinding->getLastBoundIndex()+1);
		for (bindi = srcBindings.begin(); bindi != srcBindings.end(); ++bindi)
		{
			char* lock = static_cast<char*>(bindi->second->lock(HardwareBuffer::HBL_READ_ONLY));
			srcbufferLocks[bindi->first] = lock;

			// Add a new vertex buffer and binding
			HardwareVertexBufferSharedPtr newBuf = 
				HardwareBufferManager::getSingleton().createVertexBuffer(
					bindi->second->getVertexSize(), 
					mUniqueVertexMap.size(), 
					bindi->second->getUsage(),
					bindi->second->hasShadowBuffer());
			newBind->setBinding(bindi->first, newBuf);
			lock = static_cast<char*>(newBuf->lock(HardwareBuffer::HBL_DISCARD));
			destbufferLocks[bindi->first] = lock;
		}
		const VertexBufferBinding::VertexBufferBindingMap& destBindings = 
			newBind->getBindings();


		// Iterate over the new vertices
		for (UniqueVertexMap::iterator ui = mUniqueVertexMap.begin(); 
			ui != mUniqueVertexMap.end(); ++ui)
		{
			uint32 origVertexIndex = ui->second;
			// copy vertex from each buffer in turn
			VertexBufferBinding::VertexBufferBindingMap::const_iterator srci = 
				srcBindings.begin();
			VertexBufferBinding::VertexBufferBindingMap::const_iterator desti = 
				destBindings.begin();
			for (; srci != srcBindings.end(); ++srci, ++desti)
			{
				// determine source pointer
				char* pSrc = srcbufferLocks[srci->first] + 
					(srci->second->getVertexSize() * origVertexIndex);
				char* pDest = destbufferLocks[desti->first];

				// Copy vertex from source index
				memcpy(pDest, pSrc, desti->second->getVertexSize());

				// increment destination lock pointer
				destbufferLocks[desti->first] += desti->second->getVertexSize();
			}


		}

		// unlock the buffers now
		for (bindi = srcBindings.begin(); bindi != srcBindings.end(); ++bindi)
		{
			bindi->second->unlock();
		}
		for (bindi = destBindings.begin(); bindi != destBindings.end(); ++bindi)
		{
			bindi->second->unlock();
		}

		// now switch over the bindings, and thus the buffers
		VertexBufferBinding* oldBind = mTargetVertexData->vertexBufferBinding;
		mTargetVertexData->vertexBufferBinding = newBind;
		HardwareBufferManager::getSingleton().destroyVertexBufferBinding(oldBind);


	}
	//---------------------------------------------------------------------
	void OptimiseTool::remapIndexDataList()
	{
		for (IndexDataList::iterator i = mIndexDataList.begin(); i != mIndexDataList.end(); ++i)
		{
			IndexData* idata = *i;
			remapIndexes(idata);
			
		}

	}
	//---------------------------------------------------------------------
	void OptimiseTool::remapIndexes(IndexData* idata)
	{
		// Time to repoint indexes at the new shared vertices
		uint16* p16 = 0;
		uint32* p32 = 0;

		// Lock for read & write
		if (idata->indexBuffer->getType() == HardwareIndexBuffer::IT_32BIT)
		{
			p32 = static_cast<uint32*>(idata->indexBuffer->lock(HardwareBuffer::HBL_NORMAL));
		}
		else
		{
			p16 = static_cast<uint16*>(idata->indexBuffer->lock(HardwareBuffer::HBL_NORMAL));
		}

		for (size_t j = 0; j < idata->indexCount; ++j)
		{
			uint32 oldIndex = p32? *p32 : *p16;
			uint32 newIndex = static_cast<uint32>(mIndexRemap[oldIndex]);
			if (newIndex != oldIndex)
			{
				if (p32)
					*p32 = newIndex;
				else
					*p16 = static_cast<uint16>(newIndex);
			}
			if (p32)
				++p32;
			else
				++p16;
		}

		idata->indexBuffer->unlock();

	}
	//---------------------------------------------------------------------
	bool OptimiseTool::UniqueVertexLess::equals(const Vector3& a, const Vector3& b) const
	{
		// note during this comparison we treat directions as positions
		// becuase we're interested in numerical equality, not semantics
		// and some of these might be null and thus not be a valid direction
		return a.positionEquals(b, tolerance);
	}
	//---------------------------------------------------------------------
	bool OptimiseTool::UniqueVertexLess::equals(const Vector4& a, const Vector4& b) const
	{
		// no built-in position equals
		for (int i = 0; i < 4; ++i)
		{
			if (Math::RealEqual(a[i], b[i], tolerance))
				return true;
		}
		return false;
	}
	//---------------------------------------------------------------------
	bool OptimiseTool::UniqueVertexLess::less(const Vector3& a, const Vector3& b) const
	{
		// don't use built-in operator, we need sorting
		for (int i = 0; i < 3; ++i)
		{
			if (!Math::RealEqual(a[i], b[i], tolerance))
				return a[i] < b[i];
		}
		// should never get here if equals() has been checked first
		return a.x < b.x;
	}
	//---------------------------------------------------------------------
	bool OptimiseTool::UniqueVertexLess::less(const Vector4& a, const Vector4& b) const
	{
		// don't use built-in operator, we need sorting
		for (int i = 0; i < 4; ++i)
		{
			if (!Math::RealEqual(a[i], b[i], tolerance))
				return a[i] < b[i];
		}
		// should never get here if equals() has been checked first
		return a.x < b.x;
	}
	//---------------------------------------------------------------------
	bool OptimiseTool::UniqueVertexLess::operator ()(
		const OptimiseTool::UniqueVertex &a, 
		const OptimiseTool::UniqueVertex &b) const
	{
		if (!equals(a.position, b.position))
		{
			return less(a.position, b.position);
		}
		else if (!equals(a.normal, b.normal))
		{
			return less(a.normal, b.normal);
		}
		else if (!equals(a.tangent, b.tangent))
		{
			return less(a.tangent, b.tangent);
		}
		else
		{
			// position, normal and tangent are all the same, try UVs
			for (unsigned short i = 0; i < uvSets; ++i)
			{
				if (!equals(a.uv[i], b.uv[i]))
				{
					return less(a.uv[i], b.uv[i]);
				}
			}

			// if we get here, must be equal (with tolerance)
			return false;
		}

	}
}
