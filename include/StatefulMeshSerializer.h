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

#ifndef __MM_STATEFUL_MESH_SERIALIZER_H__
#define __MM_STATEFUL_MESH_SERIALIZER_H__

#include "MeshMagickPrerequisites.h"

#include <OgreMesh.h>
#include <OgreMeshSerializer.h>
#include <OgreString.h>

namespace meshmagick
{
    class StatefulMeshSerializer : private Ogre::MeshSerializer
    {
    public:
        Ogre::MeshPtr loadMesh(const Ogre::String& name);
        void saveMesh(const Ogre::String& name, bool keepVersion, bool keepEndianess);
        void clear();
        Ogre::MeshPtr getMesh() const;
        Ogre::String getMeshFileVersion() const;
        Ogre::Serializer::Endian getEndianMode() const;
    private:
        Ogre::MeshPtr mMesh;
        Ogre::String mMeshFileVersion;
        Endian mMeshFileEndian;

        void determineFileFormat(Ogre::DataStreamPtr stream);
    };
}
#endif
