/*
This file is part of MeshMagick - An Ogre mesh file manipulation tool.
Copyright (C) 2007 - Daniel Wickert, Sascha Kolewa, Henrik Hinrichs

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

#include "MeshMergeTool.h"

#include <stdexcept>
#include <OgreMeshManager.h>
#include <OgreHardwareBufferManager.h>
#include <OgreSubMesh.h>
#include <OgreAxisAlignedBox.h>
#include <OgreSkeletonManager.h>

#include "OgreEnvironment.h"

using namespace Ogre;

namespace meshmagick
{
	MeshMergeTool::MeshMergeTool()
		: m_BaseSkeleton(),
        m_Meshes()
	{
	}

	MeshMergeTool::~MeshMergeTool() 
	{    
        m_Meshes.clear();
        m_BaseSkeleton.setNull();
	}

	void MeshMergeTool::doInvoke(const OptionList& toolOptions,
            const Ogre::StringVector& inFileNames,
            const Ogre::StringVector& outFileNames)
	{
		if (outFileNames.size() != 1)
		{
			print("Exactly one output file must be specified.", V_QUIET);
			return;
		}
		if (inFileNames.size() == 0)
		{
			print("No input files specified.", V_QUIET);
			return;
		}

		StatefulMeshSerializer* meshSer = OgreEnvironment::getSingleton().getMeshSerializer();
		StatefulSkeletonSerializer* skelSer = OgreEnvironment::getSingleton().getSkeletonSerializer();
 		for (Ogre::StringVector::const_iterator it = inFileNames.begin(); 
			it != inFileNames.end(); ++it)
		{
			MeshPtr curMesh = meshSer->loadMesh(*it);
			if (curMesh.isNull())
			{
			}
			else
			{
				if (curMesh->hasSkeleton() 
					&& SkeletonManager::getSingleton().getByName(curMesh->getSkeletonName()).isNull())
				{
					skelSer->loadSkeleton(curMesh->getSkeletonName());
				}
				addMesh(curMesh);
			}
		}
		Ogre::String outputfile = *outFileNames.begin();
		meshSer->exportMesh(bake(outputfile).getPointer(), outputfile);
	}


    void MeshMergeTool::addMesh(Ogre::MeshPtr mesh)
	{    
		SkeletonPtr meshSkel = mesh->getSkeleton();
		if (meshSkel.isNull() && mesh->hasSkeleton())
		{
			meshSkel = SkeletonManager::getSingleton().getByName(mesh->getSkeletonName());
		}

		if (meshSkel.isNull() && !m_BaseSkeleton.isNull())
        {
			print("Skipped: " + mesh->getName() + " has no skeleton", V_NORMAL);
		    return;
        }

		if (!meshSkel.isNull() && m_BaseSkeleton.isNull() && !m_Meshes.empty())
        {
			throw std::logic_error(
				"Some meshes have a skeleton, but others have none, cannot merge.");
        }

		if (!meshSkel.isNull() && m_BaseSkeleton.isNull() && m_Meshes.empty())
        {
            m_BaseSkeleton = meshSkel;                    
			print("Set: base skeleton (" + m_BaseSkeleton->getName()+")", V_HIGH);
        }


        if (meshSkel != m_BaseSkeleton)
        {
	        print("Skipped: " 
				+ mesh->getName() + " has other skeleton ("
				+ mesh->getSkeleton()->getName() +")", V_NORMAL);
            return;
        }

        m_Meshes.push_back(mesh);
	}

    const String MeshMergeTool::findSubmeshName(MeshPtr m, Ogre::ushort sid) const
    {
        Mesh::SubMeshNameMap map = m->getSubMeshNameMap();
        for (Mesh::SubMeshNameMap::const_iterator it = map.begin();
             it != map.end(); ++it)
        {
            if (it->second == sid)
                return it->first;
        }

        return "";
    }   

	MeshPtr MeshMergeTool::bake(const Ogre::String& meshname)
    {    
        print("Baking: New Mesh started", V_HIGH);

        MeshPtr mp = MeshManager::getSingleton().
            createManual(meshname, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		
		if (!m_BaseSkeleton.isNull())
		{
			mp->setSkeletonName(m_BaseSkeleton->getName());
		}

        AxisAlignedBox totalBounds = AxisAlignedBox();
        for (std::vector< Ogre::MeshPtr >::iterator it = m_Meshes.begin();
             it != m_Meshes.end(); ++it)
        {   
            print("Baking: adding submeshes for " + (*it)->getName(), V_HIGH);

            // insert all submeshes
            for (Ogre::ushort sid = 0; sid < (*it)->getNumSubMeshes(); ++sid)
            {
                SubMesh* sub = (*it)->getSubMesh(sid);
                const String name = findSubmeshName((*it), sid);                
                
                // create submesh with correct name                
                SubMesh* newsub;
                if (name.length() == 0)
                    newsub = mp->createSubMesh();
                else 
                /// @todo check if a submesh with this name has been created before
                    newsub = mp->createSubMesh(name);   

                newsub->useSharedVertices = sub->useSharedVertices;

                // add index
                newsub->indexData = sub->indexData->clone();

                // add geometry
                if (!newsub->useSharedVertices)
                {
                    newsub->vertexData = sub->vertexData->clone();
                
					if (!m_BaseSkeleton.isNull())
					{
						// build bone assignments
						SubMesh::BoneAssignmentIterator bit = sub->getBoneAssignmentIterator();
						while (bit.hasMoreElements())
						{
							VertexBoneAssignment vba = bit.getNext();
							newsub->addBoneAssignment(vba);
						}
					}
                }

                newsub->setMaterialName(sub->getMaterialName());

                print("Baking: adding submesh '" + name + "'  with material " + sub->getMaterialName(), V_HIGH);
            } 

            // sharedvertices
            if ((*it)->sharedVertexData)
            {
                /// @todo merge with existing sharedVertexData
                if (!mp->sharedVertexData)
				{
					mp->sharedVertexData = (*it)->sharedVertexData->clone();
				}

				if (!m_BaseSkeleton.isNull())
				{
					Mesh::BoneAssignmentIterator bit = (*it)->getBoneAssignmentIterator();
					while (bit.hasMoreElements())
					{
						VertexBoneAssignment vba = bit.getNext();
						mp->addBoneAssignment(vba);
					}
				}
            }

            print("Baking: adding bounds for " + (*it)->getName(), V_HIGH);

            // add bounds
            totalBounds.merge((*it)->getBounds());
        }           
        mp->_setBounds(totalBounds);

        /// @todo merge submeshes with same material


        /// @todo add parameters
        mp->buildEdgeList();

        print("Baking: Finished", V_HIGH);

        return mp;
	}

                /*
                bool use32BitIndexes = 
                    (sub->indexData->indexBuffer->getType() == HardwareIndexBuffer::IT_32BIT);
                newsub->operationType = sub->operationType;
                newsub->indexData->indexCount = sub->indexData->indexCount;
                
                HardwareIndexBufferSharedPtr newibuf = 
                    HardwareBufferManager::getSingleton().createIndexBuffer(
                        use32BitIndexes ? HardwareIndexBuffer::IT_32BIT : HardwareIndexBuffer::IT_16BIT, 
                        newsub->indexData->indexCount, 
                        HardwareBuffer::HBU_DYNAMIC,
                        false);
                newsub->indexData->indexBuffer = newibuf;
                HardwareIndexBufferSharedPtr oldibuf = sub->indexData->indexBuffer;
                unsigned int *pNewInt, *pOldInt;
                unsigned short *pNewShort, *pOldShort;
                if (use32BitIndexes)
                {
                    pNewInt = static_cast<unsigned int*>(newibuf->lock(HardwareBuffer::HBL_DISCARD));
                    pOldInt = static_cast<unsigned int*>(oldibuf->lock(HardwareBuffer::HBL_READ_ONLY));
                }
                else
                {
                    pNewShort = static_cast<unsigned short*>(newibuf->lock(HardwareBuffer::HBL_DISCARD));
                    pOldShort = static_cast<unsigned short*>(oldibuf->lock(HardwareBuffer::HBL_READ_ONLY));
                }
                for (unsigned int iid = 0; iid < newsub->indexData->indexCount; ++iid)
                {
                    if (use32BitIndexes)
                        *pNewInt++ = *pOldInt++;
                    else
                        *pNewShort++ = *pOldShort++;
                }
                newibuf->unlock();
                oldibuf->unlock();    
                        



                    new VertexData();                                                                  
                    newsub->vertexData->vertexCount = sub->vertexData->vertexCount;

                    if (newsub->vertexData->vertexCount <= 0)
                        break;

                    VertexDeclaration* decl = newsub->vertexData->vertexDeclaration;
                    VertexBufferBinding* bind = newsub->vertexData->vertexBufferBinding;*/
}
