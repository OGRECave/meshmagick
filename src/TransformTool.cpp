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

#include "TransformTool.h"

#include "OgreEnvironment.h"
#include "StatefulSkeletonSerializer.h"
#include "StatefulMeshSerializer.h"

#include <OgreStringConverter.h>
#include <OgreSubMesh.h>
#include <OgreAnimation.h>

using namespace Ogre;

namespace meshmagick
{
    TransformTool::TransformTool()
        : mTransform(Matrix4::IDENTITY),
          mNormaliseNormals(false),
          mUpdateBoundingBox(true),
          mOptions()
    {
    }

    void TransformTool::doInvoke(const OptionList& toolOptions,
        const StringVector& inFileNames, const StringVector& outFileNamesArg)
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

    void TransformTool::processSkeletonFile(Ogre::String inFile, Ogre::String outFile)
    {
        StatefulSkeletonSerializer* skeletonSerializer =
            OgreEnvironment::getSingleton().getSkeletonSerializer();

        print("Loading skeleton " + inFile + "...");
        SkeletonPtr skeleton;
        try
        {
            skeleton = skeletonSerializer->loadSkeleton(inFile);
        }
        catch(std::exception& e)
        {
            warn(e.what());
            warn("Unable to open skeleton file " + inFile);
            warn("file skipped.");
            return;
        }
        print("Processing skeleton...");
        calculateTransform(false);
        processSkeleton(skeleton);
        skeletonSerializer->saveSkeleton(outFile, true);
        print("Skeleton saved as " + outFile + ".");
    }

    void TransformTool::processMeshFile(Ogre::String inFile, Ogre::String outFile)
    {
        StatefulMeshSerializer* meshSerializer =
            OgreEnvironment::getSingleton().getMeshSerializer();

        print("Loading mesh " + inFile + "...");
        MeshPtr mesh;
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
        print("Processing mesh...");
        calculateTransform(true, mesh);
        processMesh(mesh);
        meshSerializer->saveMesh(outFile, true);
        print("Mesh saved as " + outFile + ".");

        if (mFollowSkeletonLink && mesh->hasSkeleton())
        {
            // In this case keep file name.
            processSkeletonFile(mesh->getSkeletonName(), mesh->getSkeletonName());
        }
    }

    void TransformTool::processSkeleton(Ogre::SkeletonPtr skeleton)
    {
        Skeleton::BoneIterator it = skeleton->getBoneIterator();
        while (it.hasMoreElements())
        {
            processBone(it.peekNext());
            it.moveNext();
        }

        // We only need to apply scaling on the transforms, no rotation and no translation.
        Matrix3 m;
        mTransform.extract3x3Matrix(m);
        Vector3 scale = Vector3(
            m.GetColumn(0).length(), m.GetColumn(1).length(), m.GetColumn(2).length());
        for (unsigned short aniIdx = 0; aniIdx < skeleton->getNumAnimations(); ++aniIdx)
        {
            Animation* ani = skeleton->getAnimation(aniIdx);

            print("Processing animation " + ani->getName() + "...", V_HIGH);

            Animation::NodeTrackIterator trackIt = ani->getNodeTrackIterator();
            while (trackIt.hasMoreElements())
            {
                NodeAnimationTrack* track = trackIt.getNext();
                // An animation track for a skeleton is only supposed to have
                // TransformKeyFrames, so just use these here.
                for (unsigned short frameIdx = 0; frameIdx < track->getNumKeyFrames(); ++frameIdx)
                {
                    TransformKeyFrame* keyframe = track->getNodeKeyFrame(frameIdx);
                    keyframe->setTranslate(m * keyframe->getTranslate());
                }
            }
        }
    }

    void TransformTool::processBone(Ogre::Bone* bone)
    {
        print("Processing bone " + bone->getName() + "...", V_HIGH);
        if (bone->getParent() == NULL)
        {
            // Is root bone, we need to apply full transform
            bone->setPosition(mTransform * bone->getPosition());
            Quaternion rot = mTransform.extractQuaternion();
            rot.normalise();
            bone->setOrientation(rot * bone->getOrientation());
        }
        else
        {
            // Non-root-bone, we apply only scale
            Matrix3 m3x3;
            mTransform.extract3x3Matrix(m3x3);
            Vector3 scale(
                m3x3.GetColumn(0).length(),
                m3x3.GetColumn(1).length(),
                m3x3.GetColumn(2).length());
            bone->setPosition(scale * bone->getPosition());
        }
    }

	void TransformTool::transformMesh(Ogre::MeshPtr mesh, Ogre::Matrix4 transformation)
	{
		mTransform = transformation;
		processMesh(mesh);
	}

    void TransformTool::processMesh(Ogre::MeshPtr mesh)
    {
        mBoundingBox.setNull();

        if (mesh->sharedVertexData != NULL)
        {
            processVertexData(mesh->sharedVertexData);
        }

        for(int i = 0;i < mesh->getNumSubMeshes();i++)
        {
            SubMesh* submesh = mesh->getSubMesh(i);
            if (submesh->vertexData != NULL)
            {
                processVertexData(submesh->vertexData);
            }
        }

        // If there are vertex animations, process these too.
        if (mesh->hasVertexAnimation())
        {
            // First process poses, if there are any
            for (unsigned short i = 0; i < mesh->getPoseCount(); ++i)
            {
                processPose(mesh->getPose(i));
            }

            // Then process morph targets
            unsigned short count = mesh->getNumAnimations();
            for (unsigned short i = 0; i < count; ++i)
            {
                Animation* anim = mesh->getAnimation(i);
                Animation::VertexTrackIterator it = anim->getVertexTrackIterator();
                while (it.hasMoreElements())
                {
                    VertexAnimationTrack* track = it.getNext();
                    if (track->getAnimationType() == VAT_MORPH)
                    {
                        for (unsigned short i = 0; i < track->getNumKeyFrames(); ++i)
                        {
                            processVertexMorphKeyFrame(track->getVertexMorphKeyFrame(i),
                                track->getAssociatedVertexData()->vertexCount);
                        }
                    }
                }
            }
        }

        if (mUpdateBoundingBox)
        {
            mesh->_setBounds(mBoundingBox, false);
        }
    }

    void TransformTool::processVertexData(VertexData* vertexData)
    {
        const VertexElement* position =
            vertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
        if (position != NULL)
        {
            processPositionElement(vertexData, position);
        }

        const VertexElement* normal =
            vertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_NORMAL);
        if (normal != NULL)
        {
            processDirectionElement(vertexData, normal);
        }

        const VertexElement* binormal =
            vertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_BINORMAL);
        if (binormal != NULL)
        {
            processDirectionElement(vertexData, binormal);
        }

        const VertexElement* tangent =
            vertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_TANGENT);
        if (tangent != NULL)
        {
            processDirectionElement(vertexData, tangent);
        }
    }

    void TransformTool::processPositionElement(VertexData* vertexData,
        const VertexElement* vertexElem)
    {
        Ogre::HardwareVertexBufferSharedPtr buffer =
            vertexData->vertexBufferBinding->getBuffer(vertexElem->getSource());

        unsigned char* data =
            static_cast<unsigned char*>(buffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
        for (size_t i = 0; i < vertexData->vertexCount; ++i)
        {
            Real* ptr;
            vertexElem->baseVertexPointerToElement(data, &ptr);

            Vector3 vertex(ptr);
            vertex = mTransform * vertex;
            ptr[0] = vertex.x;
            ptr[1] = vertex.y;
            ptr[2] = vertex.z;
            mBoundingBox.merge(vertex);

            data += buffer->getVertexSize();
        }
        buffer->unlock();
    }

    void TransformTool::processDirectionElement(VertexData* vertexData,
        const VertexElement* vertexElem)
    {
        // We only want to apply rotation to normal, binormal and tangent, so extract it.
        Quaternion rotation = mTransform.extractQuaternion();
        rotation.normalise();

        Ogre::HardwareVertexBufferSharedPtr buffer =
            vertexData->vertexBufferBinding->getBuffer(vertexElem->getSource());

        unsigned char* data =
            static_cast<unsigned char*>(buffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
        for (size_t i = 0; i < vertexData->vertexCount; ++i)
        {
            Real* ptr;
            vertexElem->baseVertexPointerToElement(data, &ptr);

            Vector3 vertex(ptr);
            vertex = rotation * vertex;
            if (mNormaliseNormals)
            {
                vertex.normalise();
            }
            ptr[0] = vertex.x;
            ptr[1] = vertex.y;
            ptr[2] = vertex.z;

            data += buffer->getVertexSize();
        }
        buffer->unlock();
    }

    void TransformTool::processPose(Pose* pose)
    {
        Matrix3 m3x3;
        mTransform.extract3x3Matrix(m3x3);

        Pose::VertexOffsetIterator it = pose->getVertexOffsetIterator();
        while (it.hasMoreElements())
        {
            Vector3 offset = it.peekNextValue();
            *it.peekNextValuePtr() = m3x3 * offset;
            it.moveNext();
        }
    }

    void TransformTool::processVertexMorphKeyFrame(VertexMorphKeyFrame* keyframe,
        size_t vertexCount)
    {
        Vector3* positions = static_cast<Vector3*>(
            keyframe->getVertexBuffer()->lock(HardwareBuffer::HBL_READ_ONLY));
        for (size_t i = 0; i < vertexCount; ++i)
        {
            positions[i] = mTransform * positions[i];
        }
        keyframe->getVertexBuffer()->unlock();
    }

    void TransformTool::setOptions(const OptionList& options)
    {
        mOptions = options;

        mNormaliseNormals = !OptionsUtil::isOptionSet(options, "no-normalise-normals");
        if (!mNormaliseNormals)
        {
            print("Don't normalise normals", V_HIGH);
        }
        mUpdateBoundingBox = !OptionsUtil::isOptionSet(options, "no-update-boundingbox");
        if (!mUpdateBoundingBox)
        {
            print("Don't update bounding box", V_HIGH);
        }
    }

    void TransformTool::calculateTransform(bool useMesh, MeshPtr mesh)
    {
        // Calculate transform
        Matrix4 transform = Matrix4::IDENTITY;

        print("Calculating transformation...", V_HIGH);

        for (OptionList::const_iterator it = mOptions.begin(); it != mOptions.end(); ++it)
        {
            if (it->first == "scale")
            {
                Vector3 scale = any_cast<Vector3>(it->second);
                transform = Matrix4::getScale(scale) * transform;
                print("Apply scaling " + StringConverter::toString(scale), V_HIGH);
            }
            else if (it->first == "translate")
            {
                Vector3 translate = any_cast<Vector3>(it->second);
                transform = Matrix4::getTrans(translate) * transform;
                print("Apply translation " + StringConverter::toString(translate), V_HIGH);
            }
            else if (it->first == "rotate")
            {
                Quaternion rotation = any_cast<Quaternion>(it->second);
                transform = Matrix4(rotation) * transform;
                print("Apply rotation (quat.) " + StringConverter::toString(rotation), V_HIGH);
            }
            else if (it->first == "xalign")
            {
                //ignore, if no mesh given. Without we can't do this op.
                if (!useMesh)
                {
                    print("Skipped alignment, operation can't be applied to skeletons", V_HIGH);
                    continue;
                }

                String alignment = any_cast<String>(it->second);
                Vector3 translate = Vector3::ZERO;
                // Apply current transform to the mesh, to get the bounding box to
                // base te translation on.
                AxisAlignedBox aabb = getTransformedMeshAabb(mesh, transform);
                if (alignment == "left")
                {
                    translate = Vector3(-aabb.getMinimum().x, 0, 0);
                }
                else if (alignment == "center")
                {
                    translate = Vector3(-aabb.getCenter().x, 0, 0);
                }
                else if (alignment == "right")
                {
                    translate = Vector3(-aabb.getMaximum().x, 0, 0);
                }

                transform = Matrix4::getTrans(translate) * transform;
            }
            else if (it->first == "yalign")
            {
                //ignore, if no mesh given. Without we can't do this op.
                if (!useMesh)
                {
                    print("Skipped alignment, operation can't be applied to skeletons", V_HIGH);
                    continue;
                }

                String alignment = any_cast<String>(it->second);
                Vector3 translate = Vector3::ZERO;
                // Apply current transform to the mesh, to get the bounding box to
                // base te translation on.
                AxisAlignedBox aabb = getTransformedMeshAabb(mesh, transform);
                if (alignment == "bottom")
                {
                    translate = Vector3(0, -aabb.getMinimum().y, 0);
                }
                else if (alignment == "center")
                {
                    translate = Vector3(0, -aabb.getCenter().y, 0);
                }
                else if (alignment == "top")
                {
                    translate = Vector3(0, -aabb.getMaximum().y, 0);
                }

                transform = Matrix4::getTrans(translate) * transform;
            }
            else if (it->first == "zalign")
            {
                //ignore, if no mesh given. Without we can't do this op.
                if (!useMesh)
                {
                    print("Skipped alignment, operation can't be applied to skeletons", V_HIGH);
                    continue;
                }

                String alignment = any_cast<String>(it->second);
                Vector3 translate = Vector3::ZERO;
                // Apply current transform to the mesh, to get the bounding box to
                // base te translation on.
                AxisAlignedBox aabb = getTransformedMeshAabb(mesh, transform);
                if (alignment == "front")
                {
                    translate = Vector3(0, 0, -aabb.getMinimum().z);
                }
                else if (alignment == "center")
                {
                    translate = Vector3(0, 0, -aabb.getCenter().z);
                }
                else if (alignment == "back")
                {
                    translate = Vector3(0, 0, -aabb.getMaximum().z);
                }

                transform = Matrix4::getTrans(translate) * transform;
                print("Z-Alignment " + alignment + " - "
                    + StringConverter::toString(translate), V_HIGH);
            }
        }

        mTransform = transform;
        print("final transform " + StringConverter::toString(mTransform), V_HIGH);
    }

    AxisAlignedBox TransformTool::getTransformedMeshAabb(MeshPtr mesh,
        const Matrix4& transform)
    {
        AxisAlignedBox aabb;
        if (mesh->sharedVertexData != 0)
        {
            aabb.merge(getTransformedVertexDataAabb(mesh->sharedVertexData, transform));
        }
        for (unsigned int i = 0; i < mesh->getNumSubMeshes(); ++i)
        {
            SubMesh* sm = mesh->getSubMesh(i);
            if (sm->vertexData != 0)
            {
                aabb.merge(getTransformedVertexDataAabb(sm->vertexData, transform));
            }
        }

        return aabb;
    }

    AxisAlignedBox TransformTool::getTransformedVertexDataAabb(Ogre::VertexData* vd,
        const Matrix4& transform)
    {
        AxisAlignedBox aabb;

        const VertexElement* ve = vd->vertexDeclaration->findElementBySemantic(VES_POSITION);
        HardwareVertexBufferSharedPtr vb = vd->vertexBufferBinding->getBuffer(ve->getSource());

        unsigned char* data = static_cast<unsigned char*>(
            vb->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));

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
