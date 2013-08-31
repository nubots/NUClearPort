/*
 * This file is part of ScriptEngine.
 *
 * ScriptEngine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * ScriptEngine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with ScriptEngine.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Copyright 2013 Trent Houliston <trent@houliston.me>
 */

#ifndef MODULES_SCRIPTENGINE_H
#define MODULES_SCRIPTENGINE_H

#include <map>
#include <NUClear.h>

namespace modules {

    class ScriptEngine : public NUClear::Reactor {
    private:
        std::map<std::string, ScriptPosition> scripts;
    public:
        explicit ScriptEngine(NUClear::PowerPlant* plant);
    };
}
#endif

