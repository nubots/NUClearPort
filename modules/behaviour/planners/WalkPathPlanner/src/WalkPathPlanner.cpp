/*
 * This file is part of Getup.
 *
 * ScriptRunner is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ScriptRunner is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ScriptRunner.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2013 NUBots <nubots@nubots.net>
 */

#include "WalkPathPlanner.h"

#include <cmath>
#include "messages/support/Configuration.h"
#include "messages/input/Sensors.h"
#include "messages/localisation/FieldObject.h"
#include "messages/vision/VisionObjects.h"

namespace modules {
    namespace behaviour {
        namespace planners {

            using messages::support::Configuration;
            using messages::input::Sensors;
            //using namespace messages;
            
            //using messages::input::ServoID;
            //using messages::motion::ExecuteScriptByName;
            //using messages::behaviour::RegisterAction;
            //using messages::behaviour::ActionPriorites;
            //using messages::behaviour::LimbID;

            WalkPathPlanner::WalkPathPlanner(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
                
                //we will initially stand still
                planType = messages::behaviour::WalkApproach::StandStill;
                
                //do a little configurating
                on<Trigger<Configuration<WalkPathPlanner>>>([this] (const Configuration<WalkPathPlanner>& file){
                    
                    //XXX: load these values from walk config
                    turnSpeed = file.config["turnSpeed"];
                    forwardSpeed = file.config["forwardSpeed"];
                    footSeparation = file.config["footSeparation"];
                    
                    //XXX: load from robot model
                    footSize = file.config["footSize"];
                    
                    //timers for starting turning and walking
                    walkStartTime = file.config["walkStartTime"];
                    walkTurnTime = file.config["walkTurnTime"];
                    
                    //walk accel/deccel controls
                    accelerationTime = file.config["accelerationTime"];
                    accelerationFraction = file.config["accelerationFraction"];
                    
                    //approach speeds
                    closeApproachSpeed = file.config["closeApproachSpeed"];
                    closeApproachDistance = file.config["closeApproachDistance"];
                    midApproachSpeed = file.config["midApproachSpeed"];
                    midApproachDistance = file.config["midApproachDistance"];
                    
                    //turning values
                    turnDeviation = file.config["turnDeviation"];
                    
                    //hystereses
                    distanceHysteresis = file.config["distanceHysteresis"];
                    turningHysteresis = file.config["turningHysteresis"];
                    positionHysteresis = file.config["positionHysteresis"];
                    
                    //ball lineup
                    //XXX: add these once we know what they do
                    //vector<float> ballApproachAngle = file.config["ballApproachAngle"];
                    //svector<int> ballKickFoot = file.config["ballKickFoot"];
                    ballLineupDistance = file.config["ballLineupDistance"];
                    ballLineupMinDistance = file.config["ballLineupMinDistance"];
                    
                    //extra config options
                    useAvoidance = int(file.config["useAvoidance"]) != 0;
                    assumedObstacleWidth = file.config["assumedObstacleWidth"];
                    avoidDistance = file.config["avoidDistance"];
                    
                    bearingSensitivity = file.config["bearingSensitivity"];
                    ApproachCurveFactor = file.config["ApproachCurveFactor"];
                });

                on<Trigger<Every<20, Per<std::chrono::seconds>>>, With<messages::localisation::Ball,
                                     messages::localisation::Self,
                                     std::vector<messages::vision::Obstacle>>, 
                                     Options<Sync<WalkPathPlanner>>>([this] 
                                     (const time_t& now,
                                      const messages::localisation::Ball& ball,
                                      const messages::localisation::Self& self,
                                      const std::vector<messages::vision::Obstacle>& robots
                                      ) {
                    arma::vec2 targetPos, targetHead;
                    
                    
                    //work out where we're going
                    if (targetPosition == messages::behaviour::WalkTarget::Robot) {
                        //XXX: check if robot is visible
                    } else if (targetPosition ==messages::behaviour::WalkTarget::Ball) {
                        targetPos = ball.position;
                    } else { //other types default to position/waypoint location
                        targetPos = currentTargetPosition;
                    }
                    
                    //work out where to face when we get there
                    if (targetHeading == messages::behaviour::WalkTarget::Robot) {
                        //XXX: check if robot is visible
                    } else if (targetHeading == messages::behaviour::WalkTarget::Ball) {
                        targetHead = arma::normalise(ball.position-targetPos);
                    } else { //other types default to position/waypoint bearings
                        targetHead = arma::normalise(currentTargetHeading-targetPos);
                    }
                    
                    //calculate the basic movement plan
                    arma::vec3 movePlan;
                    switch (planType) {
                        case messages::behaviour::WalkApproach::ApproachFromDirection:
                            movePlan = approachFromDirection(self,targetPos,targetHead);
                            break;
                        case messages::behaviour::WalkApproach::WalkToPoint:
                            movePlan = goToPoint(self,targetPos,targetHead);
                            break;
                        case messages::behaviour::WalkApproach::OmnidirectionalReposition:
                            movePlan = goToPoint(self,targetPos,targetHead);
                            break;
                        case messages::behaviour::WalkApproach::StandStill:
                            //emit(); //XXX: fix
                            return;
                    }
                    
                    //work out if we have to avoid something
                    if (useAvoidance) {
                        //this is a vision-based temporary for avoidance
                        movePlan = avoidObstacles(robots,movePlan);
                    }
                    
                    //this applies acceleration/deceleration and hysteresis to movement
                    movePlan = generateWalk(movePlan,
                               planType == messages::behaviour::WalkApproach::OmnidirectionalReposition);
                    
                    
                    //XXX: emit here
                });

                on<Trigger<messages::behaviour::WalkStrategy>, Options<Sync<WalkPathPlanner>>>([this] (const messages::behaviour::WalkStrategy& cmd) {
                    //reset hysteresis variables when a command changes
                    turning = 0;
                    distanceIncrement = 3;
                    
                    //save the plan
                    planType = cmd.walkMovementType;
                    targetHeading = cmd.targetHeadingType;
                    targetPosition = cmd.targetPositionType;
                    currentTargetPosition = cmd.target;
                    currentTargetHeading = cmd.heading;
                });
            }
            
            arma::vec3 WalkPathPlanner::generateWalk(const arma::vec3& move, bool omniPositioning) {
                //this uses hystereses to provide a stable, consistent positioning and movement
                float walk_speed;
                float walk_bearing;
                
                //check what distance increment we're in:
                if (move[0] > midApproachDistance+distanceHysteresis) {
                    distanceIncrement = 3;
                    walk_speed = forwardSpeed;
                } else if (move[0] > closeApproachDistance + distanceHysteresis and
                           move[0] < midApproachDistance) {
                    distanceIncrement = 2;
                    walk_speed = midApproachSpeed;
                } else if (move[0] > ballLineupDistance + distanceHysteresis and
                           move[0] < closeApproachDistance) {
                    distanceIncrement = 1;
                    walk_speed = closeApproachSpeed;
                } else {
                    distanceIncrement = 0;
                    walk_speed = 0.f;
                }
                
                //decide between heading and bearing
                if (distanceIncrement > 1) {
                    walk_bearing = move[1];
                } else {
                    walk_bearing = move[2];
                }
                
                //check turning hysteresis
                /*if (turning < 0 and walk_bearing < -turnDeviation) {
                    //walk_speed = std::min(walk_bearing,turnSpeed);
                } else if (m_turning > 0 and walk_bearing > turnDeviation) {
                    //walk_speed = std::min(walk_bearing,turnSpeed);
                } else {
                    walk_bearing = 0;
                }*/
                
                //This replaces turn hysteresis with a unicorn equation
                float g = 1./(1.+std::exp(-4.*walk_bearing*walk_bearing));
                
                arma::vec3 result;
                result[0] = walk_speed*(1.-g);
                result[1] = 0;
                result[2] = walk_bearing*g;
                return result;
            }

            arma::vec3 WalkPathPlanner::approachFromDirection(const messages::localisation::Self& self,
                                                             const arma::vec2& target,
                                                             const arma::vec2& direction) {
                       
                                                             
                //this method calculates the possible ball approach commands for the robot
                //and then chooses the lowest cost action
                std::vector<arma::vec2> waypoints(3);
                
                //calculate the values we need to set waypoints
                const double ballDistance = arma::norm(target-self.position);
                const double selfHeading = atan2(self.heading[1],self.heading[0]);
                
                //create our waypoints
                waypoints[0] = -direction*ballDistance*ApproachCurveFactor;
                waypoints[1][0] = waypoints[0][1];
                waypoints[1][1] = -waypoints[0][0];
                waypoints[2][0] = -waypoints[0][1];
                waypoints[2][1] = waypoints[0][0];
                
                //fill target headings and distances, and movement bearings and costs
                std::vector<double> headings(3);
                std::vector<double> distances(3);
                std::vector<double> bearings(3);
                std::vector<double> costs(3);
                for (size_t i = 0; i < 3; ++i) {
                    //calculate the heading the robot wants to achieve at its destination
                    const double waypointHeading = atan2(-waypoints[i][1],-waypoints[i][0])-selfHeading;
                    headings[i] = atan2(cos(waypointHeading),sin(waypointHeading));
                    
                    //calculate the distance to destination
                    const arma::vec2 waypointPos = waypoints[i]+target-self.position;
                    distances[i] = arma::norm(waypointPos);
                    
                    //calculate the angle between the current direction and the destination
                    const double waypointBearing = atan2(waypointPos[1],waypointPos[0])-selfHeading;
                    bearings[i] = atan2(cos(waypointBearing),sin(waypointBearing));
                    
                    //costs defines which move plan is the most appropriate
                    costs[i] = bearings[i]*bearings[i]*bearingSensitivity+distances[i]*distances[i];
                }
                
                //decide which approach to the ball is cheapest
                size_t selected;
                if (costs[0] < costs[1] and costs[0] < costs[2]) {
                    selected = 0;
                } else if (costs[1] < costs[2]) {
                    selected = 1;
                } else {
                    selected = 2;
                }
                
                //return the values for the selected approach
                arma::vec3 result;
                result[0] = distances[selected];
                result[1] = bearings[selected];
                result[2] = headings[selected];
                return result;
            }
            
            arma::vec3 WalkPathPlanner::goToPoint(const messages::localisation::Self& self,
                                                  const arma::vec2& target,
                                                  const arma::vec2& direction) {
                //quick and dirty go to point calculator
                //gets position and heading difference and returns walk params for it
                const arma::vec2 posdiff = target-self.position;
                const double targetDistance = arma::norm(posdiff);
                const double selfHeading = atan2(self.heading[1],self.heading[0]);
                const double targetHeading = atan2(posdiff[1],posdiff[0])-selfHeading;
                const double targetBearing = atan2(direction[1],direction[0])-selfHeading;
                
                arma::vec3 result;
                result[0] = targetDistance;
                result[1] = atan2(cos(targetBearing),sin(targetBearing));
                result[2] = atan2(cos(targetHeading),sin(targetHeading));
                return result;
            }
            
            arma::vec3 WalkPathPlanner::avoidObstacles(const std::vector<messages::vision::Obstacle>& robotPosition,
                                                  const arma::vec3& movePlan) {
                return movePlan; //XXX:unimplemented
                //double heading = 0.0;
                /*float new_bearing = relative_bearing;
                float avoid_distance = min(m_avoid_distance,distance);
                vector<Object> obstacles;
                
                
                
                //use either localised or visual avoidance
                if (m_use_localisation_avoidance) {
                    //XXX: localisation based avoidance not implemented
                    
                    
                } else {
                    obstacles = NavigationLogic::getVisibleObstacles();
                    for(unsigned int i=0; i < obstacles.size(); i++) { //for each object
                        if (obstacles[i].measuredDistance() < avoid_distance) { //if we are an obstacle
                            if (obstacles[i].measuredBearing() > relative_bearing and obstacles[i].measuredBearing()-obstacles[i].arc_width < relative_bearing) { 
                                //if we are on the right and occluding
                                new_bearing = mathGeneral::normaliseAngle(obstacles[i].measuredBearing()-obstacles[i].arc_width);
                            } else if (obstacles[i].measuredBearing() < relative_bearing and obstacles[i].measuredBearing()+obstacles[i].arc_width > relative_bearing) { 
                                //if we are on the left and occluding
                                new_bearing = mathGeneral::normaliseAngle(obstacles[i].measuredBearing()+obstacles[i].arc_width);
                            }
                        }
                    }
                }*/
            }



        }  // tools
    }  // behaviours
}  // modules