/*
 * This file is part of Darwin Sensor Filter.
 *
 * Darwin Sensor Filter is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Darwin Sensor Filter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Darwin Sensor Filter.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2013 NUBots <nubots@nubots.net>
 */

#ifndef MODULES_PLATFORM_DARWIN_SENSORFILTER_H
#define MODULES_PLATFORM_DARWIN_SENSORFILTER_H

#include <nuclear>

#include "messages/motion/ServoWaypoint.h"
#include "utility/math/kalman/UKF.h"
#include "utility/math/kalman/AdaptiveIMUModel.h"

namespace modules {
    namespace platform {
        namespace darwin {

            /**
             * TODO document
             * 
             * @author YOUR NAME HERE!
             */
            class SensorFilter : public NUClear::Reactor {
            public:
                explicit SensorFilter(std::unique_ptr<NUClear::Environment> environment);

                time_t lastUpdate;
                arma::mat33 lastOrientationMatrix;
                utility::math::kalman::UKF<utility::math::kalman::AdaptiveIMUModel> orientationFilter;
                int frameLimiter;

                double DEFAULT_NOISE_GAIN;
                double HIGH_NOISE_THRESHOLD;
                double HIGH_NOISE_GAIN;
                double LOW_NOISE_THRESHOLD;
                static constexpr const char* CONFIGURATION_PATH = "DarwinSensorFilter.json";

            };
        }
    }
}
#endif  // MODULES_PLATFORM_DARWIN_SENSORFILTER_H

