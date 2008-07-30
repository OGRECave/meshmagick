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

#include "InfoToolFactory.h"
#include "InfoTool.h"

#include <stdexcept>

using namespace Ogre;

namespace meshmagick
{
    Tool* InfoToolFactory::createTool()
    {
        Tool* tool = new InfoTool();
        return tool;
    }

    void InfoToolFactory::destroyTool(Tool* tool)
    {
        delete tool;
    }

    OptionDefinitionSet InfoToolFactory::getOptionDefinitions() const
    {
        OptionDefinitionSet optionDefs;
        optionDefs.insert(OptionDefinition("list", OT_STRING, false, true));
        return optionDefs;
    }

    void InfoToolFactory::printToolHelp(std::ostream& out) const
    {
        out << "Print information about the mesh" << std::endl
            << "info has no options, use the global option -verbose for a more detailed output" <<std::endl;
    }

    Ogre::String InfoToolFactory::getToolName() const
    {
        return "info";
    }

    Ogre::String InfoToolFactory::getToolDescription() const
    {
        return "print information about the mesh.";
    }
}
