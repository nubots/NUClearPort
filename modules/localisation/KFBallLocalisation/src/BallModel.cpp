#include "BallModel.h"

#include <armadillo>

#include "utility/math/angle.h"
#include "utility/math/coordinates.h"
#include "messages/localisation/FieldObject.h"

using messages::localisation::FakeOdometry;

namespace modules {
namespace localisation {
namespace ball {

arma::vec::fixed<BallModel::size> ApplyVelocity(
    const arma::vec::fixed<BallModel::size>& state, double deltaT) {
    auto result = state;

    // Apply ball velocity
    result[kX] += state[kVx] * deltaT;
    result[kY] += state[kVy] * deltaT;
    const double kDragCoefficient = 1.0; // TODO: Config system
    result[kVx] *= kDragCoefficient;
    result[kVy] *= kDragCoefficient;

    return result;
}

arma::vec::fixed<BallModel::size> BallModel::timeUpdate(
    const arma::vec::fixed<BallModel::size>& state, double deltaT, std::nullptr_t foo) {

    return ApplyVelocity(state, deltaT);
}

arma::vec::fixed<BallModel::size> BallModel::timeUpdate(
    const arma::vec::fixed<BallModel::size>& state, double deltaT, 
    const FakeOdometry& odom) {

    auto result = ApplyVelocity(state, deltaT);

    // Apply robot odometry / robot position change
    result[kX] -= odom.torso_displacement[0];
    result[kY] -= odom.torso_displacement[1];

    double h = -odom.torso_rotation;
    arma::mat22 rot = {  std::cos(h), std::sin(h),
                        -std::sin(h), std::cos(h) };
    // Rotate ball_pos by -torso_rotation.
    result.rows(kX, kY) = rot * result.rows(kX, kY);


    return result;
}

/// Return the predicted observation of an object at the given position
arma::vec BallModel::predictedObservation(
    const arma::vec::fixed<BallModel::size>& state, std::nullptr_t unused) {

    // // Robot-relative cartesian
    // return { state[kX], state[kY] };

    // Distance and unit vector heading
    arma::vec2 radial = utility::math::coordinates::Cartesian2Radial(state.rows(0, 1));
    auto heading_angle = radial[1];
    auto heading_x = std::cos(heading_angle);
    auto heading_y = std::sin(heading_angle);
    return {radial[0], heading_x, heading_y};
}

arma::vec BallModel::observationDifference(const arma::vec& a, 
                                            const arma::vec& b){
    // Distance and unit vector heading
    return a - b;
}

arma::vec::fixed<BallModel::size> BallModel::limitState(
    const arma::vec::fixed<BallModel::size>& state) {
 
    return { state[kX], state[kY], state[kVx], state[kVy] };
    

    // // Radial coordinates
    // return { state[kX], 
    //     utility::math::angle::normalizeAngle(state[kY]),
    //     state[kVx], state[kVy] };
}

arma::mat::fixed<BallModel::size, BallModel::size> BallModel::processNoise() {
    arma::mat noise = arma::eye(BallModel::size, BallModel::size) * processNoiseFactor;

    // noise(kX, kX) = processNoiseFactor * 100;
    // noise(kY, kY) = processNoiseFactor * 100;
    noise(kVx, kVx) = processNoiseFactor * 10;
    noise(kVy, kVy) = processNoiseFactor * 10;

    return noise;
}

}
}
}
