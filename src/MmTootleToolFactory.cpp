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
		// TODO
		return optionDefs;
	}

	Ogre::String TootleToolFactory::getToolName() const
	{
		return "Tootle";
	}

	Ogre::String TootleToolFactory::getToolDescription() const
	{
		return "Use AMD Tootle to optimise mesh.";
	}

	void TootleToolFactory::printToolHelp(std::ostream& out) const
	{
		out << std::endl;
		out << "Use AMD Tootle to optimise mesh." << std::endl << std::endl;
		// TODO
	}
}
