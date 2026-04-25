#ifndef OPENCV_DEMO_CAR_DREIVER_HPP
#define OPENCV_DEMO_CAR_DREIVER_HPP

#include <cmath>
#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>
#include <string>

#include "config.hpp"

#pragma once

#include "road.hpp"
#include "vehicle.hpp"

class Driver {
  public:
  Driver(Vehicle v, Road r, cv::Point2d pos) : vehicle_(v), road_(std::move(r)), vehicle_pos_(pos) {
    vehicle_.CalFourPointPosition(pos.x, pos.y);
  };

  Driver(Vehicle v, Road r) : vehicle_(v), road_(std::move(r)) {
    vehicle_pos_ = cv::Point2d{vehicle_.length / 2, static_cast<double>(road_.Width() / 2)};
    vehicle_.CalFourPointPosition(vehicle_pos_.x, vehicle_pos_.y);
  }

  void StartDrive(const std::string& trave_name);

  private:
  void refreshFlame();

  Vehicle vehicle_;
  Road road_;
  cv::Point2d vehicle_pos_;
};

inline void Driver::refreshFlame() {
  road_.GetCanvas() = cv::Scalar(255, 255, 255);
  road_.CreateDaoku();
  cv::line(road_.GetCanvas(), vehicle_.fourPoint.tl, vehicle_.fourPoint.tr, CAR_COLOR, 2);
  cv::line(road_.GetCanvas(), vehicle_.fourPoint.tr, vehicle_.fourPoint.br, CAR_COLOR, 2);
  cv::line(road_.GetCanvas(), vehicle_.fourPoint.br, vehicle_.fourPoint.bl, CAR_COLOR, 2);
  cv::line(road_.GetCanvas(), vehicle_.fourPoint.bl, vehicle_.fourPoint.tl, CAR_COLOR, 2);
}

inline void Driver::StartDrive(const std::string& trave_name) {
  double gap = 1000.0 / GlobalConfig.fps;
  while (true) {
    refreshFlame();
    cv::imshow(trave_name, road_.GetCanvas());
    auto pressed_key = cv::waitKey(static_cast<int>(gap));
    switch (pressed_key) {
      case CV_KEY_ESCAPE: [[fallthrough]];
      case 'q': cv::destroyAllWindows(); return;
      case CV_KEY_UP: TODO("Car Accelerate Func") vehicle_.v += 0.3; break;
      case CV_KEY_DOWN: TODO("Car Slow Down Func") vehicle_.v -= 0.3; break;
      case CV_KEY_LEFT:
        TODO("Car Move Left Func");
        if (vehicle_.delta_f <= acos(0.5) && vehicle_.delta_f >= -acos(0.5)) {
          vehicle_.delta_f += 0.05;
        }
        break;
      case CV_KEY_RIGHT:
        TODO("Car Move Right Func");
        if (vehicle_.delta_f <= acos(0.5) && vehicle_.delta_f >= -acos(0.5)) {
          vehicle_.delta_f -= 0.05;
        }
        break;
      default: break;
    }
    auto delta_pos = vehicle_.RunNextPos(gap);
    vehicle_pos_.x += delta_pos.first;
    vehicle_pos_.y += delta_pos.second;
    vehicle_.CalFourPointPosition(vehicle_pos_.x, vehicle_pos_.y);
  }
}

#endif  // OPENCV_DEMO_CAR_DREIVER_HPP
