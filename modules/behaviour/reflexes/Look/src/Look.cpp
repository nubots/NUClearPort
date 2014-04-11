/*
 * This file is part of the NUbots Codebase.
 *
 * The NUbots Codebase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The NUbots Codebase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the NUbots Codebase.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2013 NUBots <nubots@nubots.net>
 */

#include "Look.h"

#include "messages/input/ServoID.h"
#include "messages/behaviour/LookStrategy.h"
#include "messages/behaviour/Action.h"
#include "messages/input/Sensors.h"
#include "messages/support/Configuration.h"

namespace modules {
    namespace behaviour {
        namespace reflexes {

            using messages::input::ServoID;
            using messages::input::Sensors;
            using messages::behaviour::LookAtAngle;
            using messages::behaviour::RegisterAction;
            using messages::behaviour::LimbID;
            using messages::support::Configuration;
            using messages::behaviour::ServoCommand;

            //internal only callback messages to start and stop our action
            struct ExecuteLook {};

            Look::Look(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)), id(size_t(this) * size_t(this) - size_t(this)) {
                
                //do a little configurating
                on<Trigger<Configuration<Look>>>([this] (const Configuration<Look>& file){

                    //load fast and slow panspeed settings
                    
                    //pan speeds
                    fastSpeed = file.config["FastSpeed"];
                    slowSpeed = file.config["SlowSpeed"];
                    
                    //head limits
                    minYaw = file.config["minYaw"];
                    maxYaw = file.config["maxYaw"];
                    minPitch = file.config["minPitch"];
                    maxPitch = file.config["maxPitch"];
                });
                
                on<Trigger<ExecuteLook>>([this] (const ExecuteLook& e) {
                    //we are active!
                    
                });
                
                on<Trigger<LookAtAngle>, With<Sensors>>([this] (const LookAtAngle& look, const Sensors& sensors) {
                    //speeds should take into account the angle delta
                    double distance = look.pitch*look.pitch+look.yaw*look.yaw;
                    panTime = distance/fastSpeed;
                    headYaw = std::fmin(std::fmax(look.yaw+sensors.servos[size_t(ServoID::HEAD_YAW)].presentPosition,minYaw),maxYaw);
                    headPitch = std::fmin(std::fmax(look.pitch+sensors.servos[size_t(ServoID::HEAD_PITCH)].presentPosition,minPitch),maxPitch);
                    
                    
                    //this might find a better location eventually - it is the generic "gotopoint" code
                    time_t time = NUClear::clock::now() + std::chrono::nanoseconds(size_t(std::nano::den*panTime));
                    auto waypoints = std::make_unique<std::vector<ServoCommand>>();
                    waypoints->reserve(4);
                    waypoints->push_back({id, NUClear::clock::now(), ServoID::HEAD_YAW,     float(sensors.servos[size_t(ServoID::HEAD_YAW)].presentPosition),  30.f});
                    waypoints->push_back({id, NUClear::clock::now(), ServoID::HEAD_PITCH,    float(sensors.servos[size_t(ServoID::HEAD_PITCH)].presentPosition), 30.f});
                    waypoints->push_back({id, time, ServoID::HEAD_YAW,     float(std::fmin(std::fmax(headYaw,minYaw),maxYaw)),  30.f});
                    waypoints->push_back({id, time, ServoID::HEAD_PITCH,    float(std::fmin(std::fmax(headPitch,minYaw),maxYaw)), 30.f});
                    emit(std::move(waypoints));
                });

                emit<Scope::INITIALIZE>(std::make_unique<RegisterAction>(RegisterAction {
                    id,
                    { std::pair<float, std::set<LimbID>>(3.0, { LimbID::HEAD }) },
                    [this] (const std::set<LimbID>&) {
                        emit(std::make_unique<ExecuteLook>());
                    },
                    [this] (const std::set<LimbID>&) { },
                    [this] (const std::set<ServoID>&) { }
                }));
            }
        }  // reflexes
    }  // behaviours
}  // modules
