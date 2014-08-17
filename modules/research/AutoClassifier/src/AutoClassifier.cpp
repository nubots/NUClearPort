/*
 * This file is part of NUbots Codebase.
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

#include "AutoClassifier.h"
#include <armadillo>

#include "messages/vision/VisionObjects.h"

namespace modules {
namespace research {

    using messages::vision::Ball;
    using messages::vision::Goal;
    using messages::vision::Colour;
    using messages::vision::LookUpTable;

    AutoClassifier::AutoClassifier(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

        on<Trigger<std::vector<Ball>>, With<LookUpTable>, Options<Single, Priority<NUClear::LOW>>>("Auto Classifier Balls", [this](
            const std::vector<Ball>& balls, const LookUpTable& lut) {

            // create a new lookup table
            auto newLut = lut;
            if (!reference) {
                reference = std::make_unique<LookUpTable>(lut);
            }


            for (auto& ball : balls) {
                auto& image = *ball.classifiedImage->image;

                uint radius = ball.circle.radius;
                arma::vec2 centre = ball.circle.centre;

                // find the min and max y points on the circle
                // capped at the bounds of the image
                uint minY = std::max(std::round(centre[1] - radius), 0.0);
                uint maxY = std::min(std::round(centre[1] + radius), double(image.height() - 1));

                uint rangeSqr = std::pow(30, 2); // TODO: config

                // loop through pixels on the image in bounding box
                for (uint y = minY; y <= maxY; y++) {
                    // find the min and max x points on the circle for each given y
                    // uses the general equation of a circle and solves for x
                    // capped at the bounds of the image
                    double a = y - centre[1];
                    double b = std::sqrt(radius * radius - a * a);
                    uint minX = std::max(std::round(centre[0] - b), 0.0);
                    uint maxX = std::min(std::round(centre[0] + b), double(image.width() - 1));

                    for (uint x = minX; x <= maxX; x++) {
                        // get the pixel
                        auto& pixel = image(x, y);
                        uint i = 0;
                        // if pixel is unclassfied and close to 'ball' coloured, classify it
                        if (newLut(pixel) == Colour::UNCLASSIFIED) {
                            for (auto& colour : reference->getRawData()) {
                                if (colour == Colour::ORANGE) {
                                    auto matchedPixel = reference->getPixelFromIndex(i);
                                    // find euclidean distance between the two pixels
                                    uint distSqr = std::pow(pixel.y - matchedPixel.y, 2)
                                              + std::pow(pixel.cb - matchedPixel.cb, 2)
                                              + std::pow(pixel.cr - matchedPixel.cr, 2);
                                    // check its within the given range
                                    if (distSqr <= rangeSqr) {
                                        // classify!
                                        newLut(pixel) = colour;
                                        break;
                                    }
                                }
                                i++;
                            }
                        }
                    }
                }

            }

            emit(std::move(std::make_unique<LookUpTable>(newLut)));

        });

        /*on<Trigger<std::vector<Goal>>>("Auto Classifier Goals", [this](const std::vector<Goal>& goals) {

            for (auto& goal : goals) {
                // TODO:
            }

        });*/

    }

}
}

