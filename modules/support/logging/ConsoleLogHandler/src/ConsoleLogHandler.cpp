/*
 * This file is part of ConsoleLogHandler.
 *
 * ConsoleLogHandler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ConsoleLogHandler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ConsoleLogHandler.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2013 NUBots <nubots@nubots.net>
 */

#include "ConsoleLogHandler.h"

namespace modules {
    namespace support {
        namespace logging {

            ConsoleLogHandler::ConsoleLogHandler(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
                on<Trigger<NUClear::ReactionStatistics>>([this](const NUClear::ReactionStatistics & stats) {
                    if (stats.exception) {
                        try {
                            std::rethrow_exception(stats.exception);
                        }
                        catch (std::exception ex) {
                            NUClear::log<NUClear::ERROR>("Unhandled Exception: ", ex.what());
                        }
                        // We don't actually want to crash
                        catch (...) {
                        }
                    }
                });
                
                
                on<Trigger<NUClear::LogMessage>, Options<Sync<ConsoleLogHandler>>>([this](const NUClear::LogMessage& message) {
                    
                    // Output the message
                    std::cout << message.message << std::endl;
                });
            }
            
        }  // logging
    }  // support
}  // modules