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

#ifndef __MM__RENAMETOOL_H__
#define __MM__RENAMETOOL_H__

#include "Tool.h"

namespace meshmagick
{

	class _MeshMagickExport RenameTool : public Tool
	{
	public:
		RenameTool();
		~RenameTool();

	protected:
		virtual void doInvoke(const OptionList& toolOptions,
				const Ogre::StringVector& inFileNames,
				const Ogre::StringVector& outFileNames);

	private:
		typedef std::pair<Ogre::String, Ogre::String> StringPair;

		void processMeshFile(
			const OptionList &toolOptions, Ogre::String inFile, Ogre::String outFile);
		void processSkeletonFile(
			const OptionList &toolOptions, Ogre::String inFile, Ogre::String outFile);
		StringPair split(const Ogre::String& value) const;
	};

}

#endif // __MM__RENAMETOOL_H__
