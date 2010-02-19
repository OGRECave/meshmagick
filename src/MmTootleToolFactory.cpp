/*
This file is part of MeshMagick - An Ogre mesh file manipulation tool.
Copyright (C) 2007-2009 Daniel Wickert

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

#include "MmTootleToolFactory.h"

#include "MmOptionsParser.h"
#include "MmTootleTool.h"

namespace meshmagick
{

	TootleToolFactory::TootleToolFactory()
	{
	}

	TootleToolFactory::~TootleToolFactory()
	{
	}

	Tool* TootleToolFactory::createTool()
	{
		return new TootleTool();
	}

	void TootleToolFactory::destroyTool(Tool* tool)
	{
		delete tool;
	}

	OptionDefinitionSet TootleToolFactory::getOptionDefinitions() const
	{
		OptionDefinitionSet optionDefs;
		optionDefs.insert(OptionDefinition("vcachesize", OT_INT, false, false));
		optionDefs.insert(OptionDefinition("clockwise", OT_BOOL, false, false, Ogre::Any(false)));
		optionDefs.insert(OptionDefinition("clusters", OT_INT, false, false));
		optionDefs.insert(OptionDefinition("viewpoint", OT_VECTOR3, false, true));
		
		return optionDefs;
	}

	Ogre::String TootleToolFactory::getToolName() const
	{
		return "tootle";
	}

	Ogre::String TootleToolFactory::getToolDescription() const
	{
		return "Use AMD Tootle to optimise mesh.";
	}

	void TootleToolFactory::printToolHelp(std::ostream& out) const
	{
		out << std::endl;
		out << "Use AMD Tootle to optimise mesh." << std::endl << std::endl;
		out << "Optimisation parameters:" << std::endl;
		out << " -vcachesize=N    - specify the vertex cache size (omit to use default)"
			<< std::endl;
		out << " -clockwise       - treat clockwise faces as front-facing (default is CCW)"
			<< std::endl;
		out << " -clusters=N      - manually specify the number of clusters (default auto)"
			<< std::endl;
		out << " -viewpoint=x/y/z - specify one or more viewpoints to judge overdraw"
			<< std::endl;
		out << "                    Default is to generate viewpoints automatically"
			<< std::endl;
	}
}
