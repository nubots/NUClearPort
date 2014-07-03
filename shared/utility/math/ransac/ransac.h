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

#ifndef UTILITY_MATH_RANSAC_RANSAC_H
#define UTILITY_MATH_RANSAC_RANSAC_H

#include <nuclear>
#include <utility>
#include <vector>

namespace utility {
namespace math {

    template <typename Model>
    struct Ransac {

        using DataPoint = typename Model::DataPoint;

        static uint64_t xorShift() {
            static thread_local uint64_t s[2] = { rand(), rand() };

            uint64_t s1 = s[0];
            const uint64_t s0 = s[1];
            s[0] = s0;
            s1  ^= s1 << 23;
            return (s[1] = (s1 ^ s0 ^ (s1 >> 17) ^ (s0 >> 26))) + s0;
        }

        template <typename Iterator>
        static void regenerateRandomModel(Model& model, const Iterator first, const Iterator last) {

            // Get random points between first and last
            uint range = std::distance(first, last) + 1;

            std::set<uint64_t> indices;
            std::vector<DataPoint> points;

            while(indices.size() < Model::REQUIRED_POINTS) {
                indices.insert(xorShift() % range);
            }

            for(auto& i : indices) {
                auto it = first;
                std::advance(it, i);
                points.push_back(*it);
            }

            // If this returns false then it was an invalid model
            model.regenerate(points);
        }

        /**
         * Finds an individual ransac model that fits the data
         *
         * @return A pair containing an iterator to the start of the remaining set, and the best fitting model
         */
        template <typename Iterator>
        static std::pair<Iterator, Model> findModel(Iterator first
                                                  , Iterator last
                                                  , uint minimumPointsForConsensus
                                                  , uint maximumIterationsPerFitting
                                                  , double consensusErrorThreshold) {

            // Check we have enough points
            if(std::distance(first, last) < minimumPointsForConsensus) {
                return std::make_pair(last, Model());
            }

            uint largestConsensus = 0;
            Model bestModel;
            Model model;

            for(uint i = 0; i < numIterations; ++i) {

                uint consensusSize = 0;

                regenerateRandomModel(Model, first, last);

                for(auto it = first; it < last; ++it) {

                    if(model.calculateError(*it) < errorThreshold) {
                        ++consensusSize;
                    }
                }

                // If largest consensus
                if(consensusSize > largestConsensus) {
                    bestModel = std::move(model);
                }
            }

            if(largestConsensus > minConsensus) {

                auto newFirst = std::partition(first, last, [bestModel] (const DataPoint& point) {
                    return errorThreshold > bestModel.calculateError(std::forward<const DataPoint&>(point));
                });

                model.setEnds(first, newFirst);

                return std::make_pair(newFirst, std::move(model));
            }
            else {
                return std::make_pair(last, Model());
            }
        }

        template <typename Iterator>
        static std::vector<Model> fitModels(Iterator first
                                          , Iterator last
                                          , uint minimumPointsForConsensus
                                          , uint maximumIterationsPerFitting
                                          , uint maximumFittedModels
                                          , double consensusErrorThreshold) {

            std::vector<Model> results;
            results.reserve(numFittings);

            while(results.size() < maximumFittedModels) {
                Model m;
                std::tie(first, m) = findModel(first, last);

                // If we have more datapoints left then add this one and continue
                if(!m.empty()) {
                    results.push_back(std::move(m));
                }
                else {
                    return results;
                }
            }

            return results;
        }
    };
}
}

#endif