/*
This file is part of MeshMagick - An Ogre mesh file manipulation tool.
Copyright (C) 2010 Steve Streeting

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "MmTootleTool.h"

#include <tootlelib.h>

#include <OgreMesh.h>
#include <OgreSubMesh.h>

#include <sstream>

#include "MmOgreEnvironment.h"
#include "MmOptimiseTool.h"
#include "MmStatefulMeshSerializer.h"

using namespace Ogre;

namespace meshmagick
{

	// Code adapted from OgreTootle, by Ahmed Ismaiel Zakaria
	struct TootleSettings
	{
		const char *pMeshName ;
		const char *pViewpointName ;
		unsigned int nClustering ;
		unsigned int nCacheSize;
		TootleFaceWinding eWinding;
		const char* mOutFile;
	};
	struct TootleStats
	{
		unsigned int nClusters;
		float        fVCacheIn;
		float        fVCacheOut;
		float        fOverdrawIn;
		float        fOverdrawOut;
		float        fMaxOverdrawIn;
		float        fMaxOverdrawOut;
	};

	String getTootleError(TootleResult tr, const String& info)
	{
		switch(tr)
		{
		case TOOTLE_INVALID_ARGS:
			return info + ": Illegal arguments were passed.";
		case TOOTLE_OUT_OF_MEMORY:
			return info + ": Tootle ran out of memory while trying to complete the call.";
		case TOOTLE_3D_API_ERROR:
			return info + ": Errors occurred while setting up the 3D API.";
		case TOOTLE_INTERNAL_ERROR:
			return info + ": Internal error!";
		case TOOTLE_NOT_INITIALIZED:
			return info + ": Tootle was not initialized before a function call.";
		default:
			return info + ": Unknown error.";

		}
	}

	void FillMeshData(v1::HardwareIndexBufferSharedPtr indexBuffer,
		v1::VertexDeclaration *vertexDeclaration,
		v1::VertexBufferBinding* vertexBufferBinding,
		std::vector<UniqueVertex> & vertices,
		std::vector<unsigned int> &indices,
		size_t numvertices)
	{
		// Lock all the buffers first
		typedef std::vector<char*> BufferLocks;
		BufferLocks bufferLocks;
		const auto& bindings =
			vertexBufferBinding->getBindings();
		v1::VertexBufferBinding::VertexBufferBindingMap::const_iterator bindi;
		bufferLocks.resize(vertexBufferBinding->getLastBoundIndex()+1);
		for (bindi = bindings.begin(); bindi != bindings.end(); ++bindi)
		{
			char* lock = static_cast<char*>(bindi->second->lock(v1::HardwareBuffer::HBL_READ_ONLY));
			bufferLocks[bindi->first] = lock;
		}

		for(size_t i=0;i<numvertices;i++)
		{
			UniqueVertex uniqueVertex;
			const auto& elemList =
				vertexDeclaration->getElements();
			v1::VertexDeclaration::VertexElementList::const_iterator elemi;
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
					if (v1::VertexElement::getTypeCount(elemi->getType()) == 4)
					{
						uniqueVertex.tangent.w = *pFloat++;
					}
					break;
				case VES_BINORMAL:
					uniqueVertex.binormal.x = *pFloat++;
					uniqueVertex.binormal.y = *pFloat++;
					uniqueVertex.binormal.z = *pFloat++;
					break;
				case VES_TEXTURE_COORDINATES:
					// supports up to 4 dimensions
					for (unsigned short dim = 0;
						dim < v1::VertexElement::getTypeCount(elemi->getType()); ++dim)
					{
						uniqueVertex.uv[elemi->getIndex()][dim] = *pFloat++;
					}
					++uvSets;
					break;
				case VES_BLEND_INDICES:
				case VES_BLEND_WEIGHTS:
				case VES_DIFFUSE:
				case VES_SPECULAR:
					// No action needed for these semantics.
					break;
				};
			}

			vertices.push_back(uniqueVertex);

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

		//fill the index buffer
		//tootle only work with 32Bit buffers 
		//,so we'll had to recompress them later before saving

		// retrieve buffer pointers
		uint16	*pVIndices16 = NULL;    // the face indices buffer, read only
		uint32	*pVIndices32 = NULL;    // the face indices buffer, read only
		bool use32bit = false;

		if (indexBuffer->getType() == v1::HardwareIndexBuffer::IT_32BIT)
		{
			pVIndices32 = static_cast<uint32*>(
				indexBuffer->lock(v1::HardwareBuffer::HBL_READ_ONLY));
			use32bit = true;
			for(size_t i=0;i<indexBuffer->getNumIndexes();i++)
			{
				indices.push_back((unsigned int)(*pVIndices32));
				pVIndices32++;
			}

		}
		else
		{
			pVIndices16 = static_cast<uint16*>(
				indexBuffer->lock(v1::HardwareBuffer::HBL_READ_ONLY));
			size_t nm_indices=indexBuffer->getNumIndexes();
			for(size_t i=0;i<nm_indices;i++)
			{
				indices.push_back((unsigned int)(*pVIndices16));
				pVIndices16++;
			}
		}
		indexBuffer->unlock();
		int a=0;

	}

	void CopyBackMeshData(v1::HardwareIndexBufferSharedPtr indexBuffer,
		v1::VertexDeclaration *vertexDeclaration,
		v1::VertexBufferBinding* vertexBufferBinding,
		std::vector<UniqueVertex> & vertices,
		std::vector<unsigned int> & verticesRemap,
		std::vector<unsigned int> &indices,
		size_t numvertices)
	{
		// Lock all the buffers first
		typedef std::vector<char*> BufferLocks;
		BufferLocks bufferLocks;
		const auto& bindings =
			vertexBufferBinding->getBindings();
		v1::VertexBufferBinding::VertexBufferBindingMap::const_iterator bindi;
		bufferLocks.resize(vertexBufferBinding->getLastBoundIndex()+1);
		for (bindi = bindings.begin(); bindi != bindings.end(); ++bindi)
		{
			char* lock = static_cast<char*>(bindi->second->lock(v1::HardwareBuffer::HBL_NORMAL));
			bufferLocks[bindi->first] = lock;
		}

		for(size_t i=0;i<numvertices;i++)
		{
			unsigned int nVID = verticesRemap[i];
			const auto& elemList =
				vertexDeclaration->getElements();
			v1::VertexDeclaration::VertexElementList::const_iterator elemi;
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
					*pFloat++ = vertices[nVID].position.x;
					*pFloat++ = vertices[nVID].position.y;
					*pFloat++ = vertices[nVID].position.z;
					break;
				case VES_NORMAL:
					*pFloat++ = vertices[nVID].normal.x;
					*pFloat++ = vertices[nVID].normal.y;
					*pFloat++ = vertices[nVID].normal.z;
					break;
				case VES_TANGENT:
					*pFloat++ = vertices[nVID].tangent.x;
					*pFloat++ = vertices[nVID].tangent.y;
					*pFloat++ = vertices[nVID].tangent.z;
					// support w-component on tangent if present
					if (v1::VertexElement::getTypeCount(elemi->getType()) == 4)
					{
						*pFloat++ = vertices[nVID].tangent.w;
					}
					break;
				case VES_BINORMAL:
					*pFloat++ = vertices[nVID].binormal.x;
					*pFloat++ = vertices[nVID].binormal.y;
					*pFloat++ = vertices[nVID].binormal.z;
					break;
				case VES_TEXTURE_COORDINATES:
					// supports up to 4 dimensions
					for (unsigned short dim = 0;
						dim < v1::VertexElement::getTypeCount(elemi->getType()); ++dim)
					{
						*pFloat++ = vertices[nVID].uv[elemi->getIndex()][dim];
					}
					++uvSets;
					break;
				case VES_BLEND_INDICES:
				case VES_BLEND_WEIGHTS:
				case VES_DIFFUSE:
				case VES_SPECULAR:
					// No action needed for these semantics.
					break;
				};
			}

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

		//copy the index buffer back to where it came from
		if (indexBuffer->getType() == v1::HardwareIndexBuffer::IT_32BIT)
		{
			uint32	*pVIndices32 = NULL;    // the face indices buffer
			std::vector<unsigned int>::iterator srci = indices.begin();

			pVIndices32 = static_cast<uint32*>(
				indexBuffer->lock(v1::HardwareBuffer::HBL_NORMAL));
			for(size_t i=0;i<indexBuffer->getNumIndexes();i++)
			{
				*pVIndices32++ = static_cast<uint32>(*srci++);
			}

		}
		else
		{
			uint16	*pVIndices16 = NULL;    // the face indices buffer
			std::vector<unsigned int>::iterator srci = indices.begin();

			pVIndices16 = static_cast<uint16*>(
				indexBuffer->lock(v1::HardwareBuffer::HBL_NORMAL));
			size_t nm_indices=indexBuffer->getNumIndexes();
			for(size_t i=0;i<nm_indices;i++)
			{
				*pVIndices16++ = static_cast<uint16>(*srci++);
			}
		}
		indexBuffer->unlock();
	}

    v1::Mesh::VertexBoneAssignmentList getAdjustedBoneAssignments(
        Ogre::v1::SubMesh::VertexBoneAssignmentList::const_iterator bit,
        Ogre::v1::SubMesh::VertexBoneAssignmentList::const_iterator eit,
		std::vector<unsigned int> & verticesRemap)
	{
		v1::Mesh::VertexBoneAssignmentList newList;
		for (; bit != eit; ++bit)
		{
			auto ass = bit->second;
			ass.vertexIndex = verticesRemap[ass.vertexIndex];

			newList.insert(v1::Mesh::VertexBoneAssignmentList::value_type(
				ass.vertexIndex, ass));

		}

		return newList;

	}


	TootleTool::TootleTool()
		: Tool()
		, mVCacheSize(0)
		, mClockwise(false)
		, mClusters(0)
		, mQualityOptimization(false)
		, mVMemoryOptimization(true)
	{
	}

	TootleTool::~TootleTool()
	{
	}

	Ogre::String TootleTool::getName() const
	{
		return "tootle";
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

		setOptions(toolOptions);

		StringVector outFileNames = outFileNamesArg.empty() ? inFileNames : outFileNamesArg;

		// Process the meshes
		for (size_t i = 0, end = inFileNames.size(); i < end; ++i)
		{
			if (StringUtil::endsWith(inFileNames[i], ".mesh", true))
			{
				processMeshFile(inFileNames[i], outFileNames[i]);
			}
			else
			{
				warn("unrecognised name ending for file " + inFileNames[i]);
				warn("file skipped.");
			}
		}
	}

	void TootleTool::setOptions(const OptionList& options)
	{
		// default
		mVCacheSize = 0;
		mClockwise = false;
		mClusters = 0;
		mQualityOptimization = false;
		mVMemoryOptimization = true;
		mViewpointList.clear();

		for (OptionList::const_iterator i = options.begin(); i != options.end(); ++i)
		{
			if (i->first == "vcachesize")
				mVCacheSize = static_cast<unsigned int>(any_cast<int>(i->second));
			else if (i->first == "clockwise")
				mClockwise = true;
			else if (i->first == "clusters")
				mClusters = static_cast<unsigned int>(any_cast<int>(i->second));
			else if (i->first == "qualityoptimization")
				mQualityOptimization = true;
			else if (i->first == "novmemoryoptimization")
				mVMemoryOptimization = false;
			else if (i->first == "viewpoint")
				mViewpointList.push_back(any_cast<Vector3>(i->second));

		}
	}


	void TootleTool::processMeshFile(Ogre::String inFile, Ogre::String outFile)
	{
		StatefulMeshSerializer* meshSerializer =
			OgreEnvironment::getSingleton().getMeshSerializer();

		print("Loading mesh " + inFile + "...");
		v1::MeshPtr mesh;
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

		processMesh(mesh);

		meshSerializer->saveMesh(outFile, true);
		print("Mesh saved as " + outFile + ".");

	}

	void TootleTool::processMesh(Ogre::v1::MeshPtr mesh)
	{
		processMesh(mesh.get());
	}

	void TootleTool::processMesh(Ogre::v1::Mesh* mesh)
	{
		print("Processing mesh...");

		std::vector<UniqueVertex> vertices;
		std::vector<unsigned int> indices;

		// Init options
		bool gatherStats = OgreEnvironment::getSingleton().isStandalone() && mVerbosity >= V_HIGH;
		unsigned int cacheSize = mVCacheSize ? mVCacheSize : TOOTLE_DEFAULT_VCACHE_SIZE;
		TootleFaceWinding winding = mClockwise ? TOOTLE_CW : TOOTLE_CCW;
		float* pViewpoints = mViewpointList.empty() ? 0 : mViewpointList.begin()->ptr();
		unsigned int numViewpoints = static_cast<unsigned int>(mViewpointList.size());


		int num_lods = mesh->getNumLodLevels();
		int num_submeshes = mesh->getNumSubMeshes();
		for(int i = 0; i < num_submeshes; i++)
		{
			vertices.clear();
			indices.clear();

			// build buffers containing only the vertex positions and indices, since this is what Tootle requires
			v1::SubMesh * smesh=mesh->getSubMesh(i);

			// Skip empty submeshes
			if (!smesh->indexData[VpNormal]->indexCount)
				continue;

			if(smesh->operationType!=OT_TRIANGLE_LIST)
			{
				continue;
			}
			if(smesh->useSharedVertices)
			{
				FillMeshData(smesh->indexData[VpNormal]->indexBuffer,
					mesh->sharedVertexData[VpNormal]->vertexDeclaration,
					mesh->sharedVertexData[VpNormal]->vertexBufferBinding,
					vertices,indices,
					mesh->sharedVertexData[VpNormal]->vertexCount);
			}
			else
			{
				FillMeshData(smesh->indexData[VpNormal]->indexBuffer,
					smesh->vertexData[VpNormal]->vertexDeclaration,
					smesh->vertexData[VpNormal]->vertexBufferBinding,
					vertices,indices,
					smesh->vertexData[VpNormal]->vertexCount);

			}
			if(indices.size()>0)
			{
				//start tootle work


#if 0
				// Dump to .obj for diagnostics for when Tootle asserts
				if (mVerbosity >= V_HIGH)
				{
					std::ofstream ofstream;
					String filename = String("TootleInput_") + mesh->getName() + "_" + StringConverter::toString(i) + ".obj";
					ofstream.open(filename.c_str());

					for (std::vector<Vector3>::iterator v = vertices.begin(); v != vertices.end(); ++v)
					{
						ofstream << "v " << v->x << " " << v->y << " " << v->z << std::endl;
					}
					for (std::vector<unsigned int>::iterator f = indices.begin(); f != indices.end();)
					{
						ofstream << "f " << (*f++ + 1) << " " << (*f++ + 1) << " " << (*f++ + 1) << std::endl;
					}
					ofstream.close();

				}
#endif


				// *****************************************************************
				//   Optimize the mesh
				// *****************************************************************

				unsigned int nTriangles = (unsigned int) indices.size() / 3;
				unsigned int nVertices = (unsigned int) vertices.size();
				const float* pVB = (float*) &vertices[0];
				unsigned int* pIB = (unsigned int*) &indices[0];

				// position: 3 * sizeof(float)
				// normal:   3 * sizeof(float)
				// tangent:  4 * sizeof(float)
				// binormal: 3 * sizeof(float)
				// uv:       3 * OGRE_MAX_TEXTURE_COORD_SETS * sizeof(float)
				unsigned int nStride = (3 + 3 + 4 + 3 + 3 * OGRE_MAX_TEXTURE_COORD_SETS) * sizeof(float);

				TootleStats stats;
				TootleResult result;

				// initialize Tootle
				result = TootleInit();
				if( result != TOOTLE_OK )
					fail(getTootleError(result, "TootleInit"));


				if (gatherStats)
				{
					// measure input VCache efficiency
					result = TootleMeasureCacheEfficiency( pIB, nTriangles, cacheSize, &stats.fVCacheIn );
					if( result != TOOTLE_OK )
						fail(getTootleError(result, "TootleMeasureCacheEfficiency"));


					// measure input overdraw.  Note that we assume counter-clockwise vertex winding. 
					result = TootleMeasureOverdraw( pVB, pIB, nVertices, nTriangles, nStride, 
						pViewpoints, numViewpoints, winding, 
						&stats.fOverdrawIn, &stats.fMaxOverdrawIn );
					if( result != TOOTLE_OK )
						fail(getTootleError(result, "TootleMeasureOverdraw"));
				}

				if (mQualityOptimization)
				{
					result = TootleOptimize( pVB, pIB, nVertices, nTriangles, nStride, cacheSize,
						pViewpoints, numViewpoints, winding,
						pIB, &stats.nClusters );
					if( result != TOOTLE_OK )
						fail(getTootleError(result, "TootleOptimize"));
				}
				else
				{
					result = TootleFastOptimize( pVB, pIB, nVertices, nTriangles, nStride, cacheSize, winding,
						pIB, &stats.nClusters );
					if( result != TOOTLE_OK )
						fail(getTootleError(result, "TootleFastOptimize"));
				}


				if (gatherStats)
				{
					// measure output VCache efficiency
					result = TootleMeasureCacheEfficiency( pIB, nTriangles, cacheSize, &stats.fVCacheOut );
					if( result != TOOTLE_OK )
						fail(getTootleError(result, "TootleMeasureCacheEfficiency"));

					// measure output overdraw
					result = TootleMeasureOverdraw( pVB, pIB, nVertices, nTriangles, nStride,  
						pViewpoints, numViewpoints, winding, 
						&stats.fOverdrawOut, &stats.fMaxOverdrawOut );
					if( result != TOOTLE_OK )
						fail(getTootleError(result, "TootleMeasureOverdraw"));

					// print tootle statistics (only in verbose mode)
					Ogre::StringStream statsStr;
					statsStr 
						<< "Tootle Stats for submesh " << i << ": " << std::endl
						<< "  Clusters: " << stats.nClusters << std::endl
						<< "  Cache In/Out: " << stats.fVCacheIn << " / " << stats.fVCacheOut << " = " << (stats.fVCacheIn/stats.fVCacheOut) << std::endl
						<< "  Overdraw In/Out: " << stats.fOverdrawIn << " / " << stats.fOverdrawOut << " = " << (stats.fOverdrawIn/stats.fOverdrawOut) << std::endl
						<< "  Max Overdraw In/Out: " << stats.fMaxOverdrawIn << " / " << stats.fMaxOverdrawOut << " = " << (stats.fMaxOverdrawIn/stats.fMaxOverdrawOut);
					print(statsStr.str(), V_HIGH);
				}

				std::vector<unsigned int> pnVertexRemapping(nVertices);
				std::vector<unsigned int> pnVertexInverseRemapping(nVertices);

				if (mVMemoryOptimization)
				{
					result = TootleOptimizeVertexMemory( pVB, pIB, nVertices, nTriangles, nStride,
						NULL, pIB, &pnVertexRemapping[0] );
					if( result != TOOTLE_OK )
						fail(getTootleError(result, "TootleOptimize"));

					for (unsigned int i = 0; i < nVertices; i++)
					{
						unsigned int nVID = pnVertexRemapping[i];
						pnVertexInverseRemapping[nVID] = i;
					}
				}
				else
				{
					for (unsigned int i = 0; i < nVertices; i++)
					{
						pnVertexRemapping[i] = i;
						pnVertexInverseRemapping[i] = i;
					}
				}

				// clean up tootle
				TootleCleanup();

				if(smesh->useSharedVertices)
				{
					CopyBackMeshData(smesh->indexData[VpNormal]->indexBuffer,
						mesh->sharedVertexData[VpNormal]->vertexDeclaration,
						mesh->sharedVertexData[VpNormal]->vertexBufferBinding,
						vertices, pnVertexInverseRemapping, indices,
						mesh->sharedVertexData[VpNormal]->vertexCount);

					if (i == 0)
					{
						if (mesh->getSkeletonName() != Ogre::BLANKSTRING)
						{
							const auto& bas = mesh->getBoneAssignments ();
							auto newList = getAdjustedBoneAssignments(bas.begin(), bas.end(), pnVertexRemapping);
							mesh->clearBoneAssignments();
							for (const auto& boneAssignment : newList)
								mesh->addBoneAssignment (boneAssignment.second);
						}
					}
				}
				else
				{
					CopyBackMeshData(smesh->indexData[VpNormal]->indexBuffer,
						smesh->vertexData[VpNormal]->vertexDeclaration,
						smesh->vertexData[VpNormal]->vertexBufferBinding,
						vertices, pnVertexInverseRemapping, indices,
						smesh->vertexData[VpNormal]->vertexCount);

				}

				const auto& bas = smesh->getBoneAssignments ();
				auto newList = getAdjustedBoneAssignments(bas.begin(), bas.end(), pnVertexRemapping);
				smesh->clearBoneAssignments();
				for (const auto& boneAssignment : newList)
					smesh->addBoneAssignment (boneAssignment.second);
			}
		}


	}

}
