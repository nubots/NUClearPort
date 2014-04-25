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

#ifndef MODULES_RESEARCH_HAPTICS_H
#define MODULES_RESEARCH_HAPTICS_H

#include <nuclear>

namespace modules {
    namespace vision {

        /**
         * TODO document
         *
         * @author Jake Fountain
         */
        class SLAME : public NUClear::Reactor {
        private:
            std::vector<float> featureStrengths;
            std::vector<ExtractedFeature> features;
            std::vector<SLAMEFeatureUKF> featureFilters;

            utility::vision::FeatureExtractor featureExtractor;

            NUClear::clock::time_point lastTime;
        public:
            static constexpr const char* CONFIGURATION_PATH = "SLAME.json";
            explicit SLAME(std::unique_ptr<NUClear::Environment> environment);
        };
    }
}
#endif

