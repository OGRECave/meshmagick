/*
This file is part of MeshMagick - An Ogre mesh file manipulation tool.
Copyright (C) 2007-2010 Daniel Wickert

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

#include "MmMeshUtils.h"

#include <OgreSubMesh.h>

using namespace Ogre;

namespace meshmagick
{
	AxisAlignedBox MeshUtils::getMeshAabb(v1::MeshPtr mesh, const Matrix4& transform)
	{
		return getMeshAabb(mesh.get(), transform);
	}
    AxisAlignedBox MeshUtils::getMeshAabb(v1::Mesh* mesh, const Matrix4& transform)
    {
        AxisAlignedBox aabb;
        if (mesh->sharedVertexData[VpNormal] != 0)
        {
            aabb.merge(getVertexDataAabb(mesh->sharedVertexData[VpNormal], transform));
        }
        for (unsigned int i = 0; i < mesh->getNumSubMeshes(); ++i)
        {
            v1::SubMesh* sm = mesh->getSubMesh(i);
            if (sm->vertexData[VpNormal] != 0)
            {
                aabb.merge(getVertexDataAabb(sm->vertexData[VpNormal], transform));
            }
        }

        return aabb;
    }

    AxisAlignedBox MeshUtils::getVertexDataAabb(v1::VertexData* vd, const Matrix4& transform)
    {
        AxisAlignedBox aabb;

        const v1::VertexElement* ve = vd->vertexDeclaration->findElementBySemantic(VES_POSITION);
        v1::HardwareVertexBufferSharedPtr vb = vd->vertexBufferBinding->getBuffer(ve->getSource());

        unsigned char* data = static_cast<unsigned char*>(
            vb->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));

        for (size_t i = 0; i < vd->vertexCount; ++i)
        {
            float* v;
            ve->baseVertexPointerToElement(data, &v);
            aabb.merge(transform * Vector3(v[0], v[1], v[2]));

            data += vb->getVertexSize();
        }
        vb->unlock();

        return aabb;
    }
}
