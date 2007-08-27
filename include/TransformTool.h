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

#ifndef __MM_TRANSFORM_TOOL_H__
#define __MM_TRANSFORM_TOOL_H__

#include "MeshMagickPrerequisites.h"
#include "Tool.h"
#include "OptionsParser.h"

#include <OgreAnimation.h>
#include <OgreMesh.h>
#include <OgreSkeleton.h>
#include <OgreBone.h>

namespace meshmagick
{
    class _MeshMagickExport TransformTool : public Tool
    {
    public:
        TransformTool();

		void transformMesh(Ogre::MeshPtr mesh, Ogre::Matrix4 transformation);

    private:
        Ogre::Matrix4 mTransform;
        Ogre::AxisAlignedBox mBoundingBox;
        bool mNormaliseNormals;
        bool mUpdateBoundingBox;
        OptionList mOptions;

        void processSkeletonFile(Ogre::String file, Ogre::String outFile);
        void processMeshFile(Ogre::String file, Ogre::String outFile);

        void processSkeleton(Ogre::SkeletonPtr skeleton);
        void processMesh(Ogre::MeshPtr mesh);

        void setOptions(const OptionList& options);

        void processVertexData(Ogre::VertexData* vertexData);
        void processPositionElement(Ogre::VertexData* vertexData,
            const Ogre::VertexElement* vertexElem);
        void processDirectionElement(Ogre::VertexData* vertexData,
            const Ogre::VertexElement* vertexElem);

        void processAnimation(Ogre::Animation* ani);
        void processBone(Ogre::Bone* bone);
        void processPose(Ogre::Pose* pose);
        void processVertexMorphKeyFrame(Ogre::VertexMorphKeyFrame* keyframe, size_t vertexCount);

        /// Calculate transformation matrix from input arguments and, if given, a mesh.
        /// The mesh is used to retrieve the AABB, which is needed for alignment operation.
        /// Alignment operations are ignored, if no mesh is given.
        /// This doesn't matter for skeletons, since translations don't apply there.
        void calculateTransform(bool useMesh, Ogre::MeshPtr mesh = Ogre::MeshPtr());

        Ogre::AxisAlignedBox getTransformedMeshAabb(Ogre::MeshPtr mesh,
            const Ogre::Matrix4& transform);
        Ogre::AxisAlignedBox getTransformedVertexDataAabb(Ogre::VertexData* vd,
            const Ogre::Matrix4& transform);

		void doInvoke(const OptionList& toolOptions,
            const Ogre::StringVector& inFileNames,
            const Ogre::StringVector& outFileNames);
    };
}
#endif
