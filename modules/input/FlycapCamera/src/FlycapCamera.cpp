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

#include "FlycapCamera.h"

extern "C" {
    #include <jpeglib.h>
}
#include "utility/image/ColorModelConversions.h"
#include "messages/input/Image.h"
#include "messages/input/CameraParameters.h"
#include "messages/support/Configuration.h"

namespace modules {
    namespace input {

        using messages::support::Configuration;
        using messages::input::CameraParameters;
        //using namespace FlyCapture2;

        // We assume that the device will always be video0, if not then change this
        FlycapCamera::FlycapCamera(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

            // When we shutdown, we must tell our camera class to close (stop streaming)
            on<Trigger<Shutdown>>([this](const Shutdown&) {
                camera.closeCamera();
            });

            on<Trigger<Configuration<FlycapCamera>>>([this](const Configuration<FlycapCamera>& config) {

                auto cameraParameters = std::make_unique<CameraParameters>();

                cameraParameters->imageSizePixels << config["imageWidth"].as<uint>() << config["imageHeight"].as<uint>();
                cameraParameters->FOV << config["FOV_X"].as<double>() << config["FOV_Y"].as<double>();
                cameraParameters->distortionFactor = config["DISTORTION_FACTOR"].as<double>();
                arma::vec2 tanHalfFOV;
                tanHalfFOV << std::tan(cameraParameters->FOV[0] * 0.5) << std::tan(cameraParameters->FOV[1] * 0.5);
                arma::vec2 imageCentre;
                imageCentre << cameraParameters->imageSizePixels[0] * 0.5 << cameraParameters->imageSizePixels[1] * 0.5;
                cameraParameters->pixelsToTanThetaFactor << (tanHalfFOV[0] / imageCentre[0]) << (tanHalfFOV[1] / imageCentre[1]);
                cameraParameters->focalLengthPixels = imageCentre[0] / tanHalfFOV[0];
                emit<Scope::DIRECT>(std::move(cameraParameters));
                
                try {
                    camera.resetCamera(0, 1280, 960);
                    //camera.startStreaming();
                    camera.camera.StartCapture(
                                [](FlyCapture2::Image* pImage, const void* pCallbackData) {
                                    FlycapCamera* reactor = reinterpret_cast<FlycapCamera*>(const_cast<void*>(pCallbackData));
                                    
                                    std::vector<messages::input::Image::Pixel> data(960*1280, {0,0,0});
                                    std::unique_ptr<messages::input::Image> image;
                                    FlyCapture2::Image& rawImage = *pImage;
                                    //const size_t total = 1280 * 960;

                                    // do a cache-coherent demosaic step
                                    for (size_t j = 960/2-465; j < 960/2+465; j += 1280) {
                                        
                                        
                                        for (size_t i = reactor->getViewStart(j/1280,1280,960,465); 
                                            i < getViewEnd(j/1280,1280,960,465); i += 2) { // assume we always start on an even pixel (odd ones are nasty)
                                            
                                            const size_t index = i+j;
                                            //do the current row
                                            auto& px = data[index];
                                            auto& pxNext = data[index+1];
                                            auto& pxNextNext = data[index+2];
                                            
                                            //get the required information
                                            const auto& currentGreen = *rawImage[index];
                                            const auto& currentBlue = *rawImage[index+1];
                                            const auto& nextGreen = *rawImage[index+2];
                                            const auto& nextBlue = *rawImage[index+3];
                                            
                                            //demosaic red and green
                                            px.cr = currentBlue;
                                            pxNext.cb = currentGreen;
                                            pxNext.cr = (unsigned char)(((unsigned int)currentBlue + (unsigned int)nextBlue) >> 1);
                                            pxNextNext.cb = (unsigned char)(((unsigned int)currentGreen + (unsigned int)nextGreen) >> 1);
                                            
                                            //do the row below
                                            px = data[index+1280];
                                            pxNext = data[index+1+1280];
                                            pxNextNext = data[index+2+1280];
                                            
                                            px.cr = currentBlue;
                                            pxNext.cb = currentGreen;
                                            pxNext.cr = (unsigned char)(((unsigned int)currentBlue + (unsigned int)nextBlue) >> 1);
                                            pxNextNext.cb = (unsigned char)(((unsigned int)currentGreen + (unsigned int)nextGreen) >> 1);
                                            
                                            //do the row above
                                            //XXX: if we are feeling hacky, this line can be ignored. It just makes things nicer
                                            px = data[index-1280];
                                            pxNext = data[index+1-1280];
                                            pxNextNext = data[index+2-1280];
                                            
                                            px.cr = (unsigned char)(((unsigned int)currentBlue + (unsigned int)px.cr) >> 1);
                                            pxNext.cb = (unsigned char)(((unsigned int)currentGreen + (unsigned int)pxNext.cb) >> 1);
                                            pxNext.cr = (unsigned char)(((((unsigned int)currentBlue + (unsigned int)nextBlue) >> 1) + (unsigned int)pxNext.cr) >> 1);
                                            pxNextNext.cb = (unsigned char)(((((unsigned int)currentGreen + (unsigned int)nextGreen) >> 1) + (unsigned int)pxNextNext.cb) >> 1);
                                            
                                        }
                                        j += 1280;
                                        // do the second line
                                        for (size_t i = reactor->getViewStart(j/1280,1280,960,465); 
                                            i < getViewEnd(j/1280,1280,960,465); i += 2) { // assume we always start on an even pixel (odd ones are nasty)
                                            
                                            const size_t index = i+j;
                                            //do the current row
                                            auto& px = data[index];
                                            auto& pxNext = data[index+1];
                                            auto& pxNextNext = data[index+2];
                                            
                                            //get the required information
                                            const auto& currentRed = *rawImage[index];
                                            const auto& currentGreen = *rawImage[index+1];
                                            const auto& nextRed = *rawImage[index+2];
                                            const auto& nextGreen = *rawImage[index+3];
                                            
                                            //demosaic red and green
                                            px.y = currentRed;
                                            pxNext.cb = currentGreen;
                                            pxNext.y = (unsigned char)(((unsigned int)currentRed + (unsigned int)nextRed) >> 1);
                                            pxNextNext.cb = (unsigned char)(((unsigned int)currentGreen + (unsigned int)nextGreen) >> 1);
                                            
                                            //do the row below
                                            px = data[index+1280];
                                            pxNext = data[index+1+1280];
                                            pxNextNext = data[index+2+1280];
                                            
                                            px.y = currentRed;
                                            pxNext.cb = currentGreen;
                                            pxNext.y = (unsigned char)(((unsigned int)currentRed + (unsigned int)nextRed) >> 1);
                                            pxNextNext.cb = (unsigned char)(((unsigned int)currentGreen + (unsigned int)nextGreen) >> 1);
                                            
                                            //do the row above
                                            //XXX: if we are feeling hacky, this line can be ignored. It just makes things nicer
                                            px = data[index-1280];
                                            pxNext = data[index+1-1280];
                                            pxNextNext = data[index+2-1280];
                                            
                                            px.y = (unsigned char)(((unsigned int)currentRed + (unsigned int)px.y) >> 1);
                                            pxNext.cb = (unsigned char)(((unsigned int)currentGreen + (unsigned int)pxNext.cb) >> 1);
                                            pxNext.y = (unsigned char)(((((unsigned int)currentRed + (unsigned int)nextRed) >> 1) + (unsigned int)pxNext.y) >> 1);
                                            pxNextNext.cb = (unsigned char)(((((unsigned int)currentGreen + (unsigned int)nextGreen) >> 1) + (unsigned int)pxNextNext.cb) >> 1);
                                            
                                            //utility::image::YCbCr res = utility::image::toYCbCr(utility::image::RGB{uint8_t(*(rawImage(j+1,i))),
                                            //                                                    uint8_t(*(rawImage(j,i))),
                                            //                                                    uint8_t(*(rawImage(j,i+1)))
                                            //                                                    });
                                            
                                            //NOTE:
                                            //we think i,j and i+1,j+1 are green
                                            //i+1,j is BLUE
                                            //i,j+1 is RED
                                            //data[(i+640*j)/2].y  = *(rawImage(j+1,i)); //res.Y;
                                            //data[(i+640*j)/2].cb = *(rawImage(j,i)); //res.Cb;
                                            //data[(i+640*j)/2].cr = *(rawImage(j,i+1)); //res.Cr;
                                            
                                        }
                                    }
                                    image = std::unique_ptr<messages::input::Image>(new messages::input::Image(1280, 960, std::move(data)));
                                    reactor->emit(std::move(image));
                                    
                                }
                                ,this);
                    
                    
                } catch(const std::exception& e) {
                    NUClear::log<NUClear::DEBUG>(std::string("Exception while starting camera streaming: ") + e.what());
                    throw e;
                }
                
                //this is how to set properties
                //XXX: brightness is not enabled by default - do we want it? (ditto for temperature)
                auto& s = camera.getSettings()["brightness"];
                //s.onOff = true;
                s.valueA = config["brightness"].as<unsigned int>();
                camera.camera.SetProperty(&s);
                
                s = camera.getSettings()["gain"];
                s.autoManualMode = config["gain_auto"].as<unsigned int>();
                s.valueA = config["gain"].as<unsigned int>();
                camera.camera.SetProperty(&s);
                
                s = camera.getSettings()["gamma"];
                s.onOff = true;
                s.valueA = config["gamma"].as<unsigned int>();
                camera.camera.SetProperty(&s);
                
                s = camera.getSettings()["absolute_exposure"];
                s.valueA = config["absolute_exposure"].as<unsigned int>();
                camera.camera.SetProperty(&s);
                
                s = camera.getSettings()["absolute_pan"];
                s.valueA = config["absolute_pan"].as<unsigned int>();
                camera.camera.SetProperty(&s);
                
                s = camera.getSettings()["absolute_tilt"];
                s.valueA = config["absolute_tilt"].as<unsigned int>();
                camera.camera.SetProperty(&s);
                
                s = camera.getSettings()["auto_exposure"];
                s.onOff = config["auto_exposure"].as<int>();
                s.absValue = config["auto_exposure_val"].as<float>();
                camera.camera.SetProperty(&s);
                
                s = camera.getSettings()["white_balance_temperature"];
                s.valueA = config["white_balance_temperature_red"].as<unsigned int>();
                s.valueB = config["white_balance_temperature_blue"].as<unsigned int>();
                s.onOff = config["auto_white_balance"].as<unsigned int>();
                camera.camera.SetProperty(&s);
                
                //XXX: auto white balance
                //auto& s = camera.getSettings()["absolute_pan"];
                //s.valueA = config["absolute_pan"].as<unsigned int>();
                //camera.camera.SetProperty(&s);
                
                /*
                settings.insert( std::make_pair("brightness",                 p) );
                settings.insert( std::make_pair("auto_exposure",              p) );
                settings.insert( std::make_pair("sharpness",                  p) );
                settings.insert( std::make_pair("white_balance_temperature",  p) );
                settings.insert( std::make_pair("hue",                        p) );
                settings.insert( std::make_pair("saturation",                 p) );
                settings.insert( std::make_pair("gamma",                      p) );
                settings.insert( std::make_pair("iris",                       p) );
                settings.insert( std::make_pair("absolute_focus",             p) );
                settings.insert( std::make_pair("absolute_zoom",              p) );
                settings.insert( std::make_pair("absolute_pan",               p) );
                settings.insert( std::make_pair("absolute_tilt",              p) );
                settings.insert( std::make_pair("absolute_exposure",          p) );
                settings.insert( std::make_pair("gain",                       p) );
                settings.insert( std::make_pair("temperature",                p) );
                */
                
            });

            /*on<Trigger<Every<1, std::chrono::seconds>>, With<Configuration<LinuxCamera>>>("Camera Setting Applicator", [this] (const time_t&, const Configuration<LinuxCamera>& config) {
                if(camera.isStreaming()) {
                    // Set all other camera settings
                    for(auto& setting : camera.getSettings()) {
                        int value = config[setting.first].as<int>();
                        if(setting.second.set(value) == false) {
                            NUClear::log<NUClear::DEBUG>("Failed to set " + setting.first + " on camera");
                        }
                    }
                }
            });*/
        }

    }  // input
}  // modules