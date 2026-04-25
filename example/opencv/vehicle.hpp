#ifndef OPENCV_DEMO_CAR_VEHICLE_HPP
#define OPENCV_DEMO_CAR_VEHICLE_HPP

#include <cmath>
#include <opencv2/core/types.hpp>
#include <utility>
#pragma once

#include "config.hpp"

struct Vehicle {
  double width = GlobalConfig.CarWidth;
  double length = GlobalConfig.CarLength;

  double v = 0;            // 质心速度【input】
  double a = 0;            // 加速度（暂时认为车是匀速的）
  double delta_f = 0;      // 前轮转向角【input】
  double delta_r = 0;      // 后轮转向角（前轮驱动时恒为零）
  double beta = 0;         // 滑移角（tire slip angle 速度与车身朝向夹角）
  double phi = 0;          // 航向角（heading angle 车头与X轴夹角）
  double lf = length / 2;  // 前悬长度
  double lr = length / 2;  // 后悬长度

  double theta = atan(width / length);                   // 矩形车对角线与length夹角，中间变量
  double linar = sqrt(width * width + length * length);  // 车对角线长，中间变量

  struct {
    cv::Point2d tl;
    cv::Point2d tr;
    cv::Point2d bl;
    cv::Point2d br;
  } fourPoint;

  std::pair<double, double> RunNextPos(double dt);

  void CalFourPointPosition(double x, double y);
};

inline std::pair<double, double> Vehicle::RunNextPos(double dt) {
  phi = (v * cos(phi + beta) * (tan(delta_f) - tan(delta_r)) / (lf + lr)) * dt;
  double delta_x = v * cos(phi + beta);
  double delta_y = v * sin(phi + beta);
  return {delta_x, delta_y};
}

inline void Vehicle::CalFourPointPosition(double x, double y) {
  fourPoint.tl = {x - linar * cos(theta + phi) / 2, y - linar * sin(theta + phi) / 2};
  fourPoint.tr = {x + linar * cos(theta - phi) / 2, y - linar * sin(theta - phi) / 2};
  fourPoint.bl = {x - linar * cos(theta - phi) / 2, y + linar * sin(theta - phi) / 2};
  fourPoint.br = {x + linar * cos(theta + phi) / 2, y + linar * sin(theta + phi) / 2};
}

#endif  // OPENCV_DEMO_CAR_VEHICLE_HPP