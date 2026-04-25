#ifndef OPENCV_DEMO_CAR_ROAD_HPP
#define OPENCV_DEMO_CAR_ROAD_HPP

#pragma once

#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>

#include "config.hpp"
#include "skutils/logger.h"

class Road {
  public:
  explicit Road(int width = GlobalConfig.MainCols, int length = GlobalConfig.MainRows,
                double car_room_width = GlobalConfig.CarRoomWidth, double car_room_length = GlobalConfig.CarRoomLength,
                double gap = GlobalConfig.Space)
    : width_(width),
      length_(length),
      car_room_width_(car_room_width),
      car_room_length_(car_room_length),
      gap_(gap),
      main_canvas_(cv::Mat(width, length, CV_8UC3, WHITE_SCALAR)) {}

  int Width() const { return width_; }

  int Length() const { return length_; }

  cv::Mat GetCanvas() { return main_canvas_; }

  void CreateDaoku();

  void CreateCeFang() { TODO("Create Cefang roadmap"); }

  private:
  cv::Point2d createDaokuAbove(const cv::Point2d& bl);
  cv::Point2d createDaokuBelow(const cv::Point2d& tl);

  int width_;
  int length_;
  double car_room_width_;
  double car_room_length_;
  double gap_{};
  cv::Mat main_canvas_;
};

inline cv::Point2d Road::createDaokuAbove(const cv::Point2d& bl) {
  if ((bl.x + car_room_width_) > width_) {
    return {-1, -1};
  }
  // 1. 创建点位
  cv::Point2d A{bl.x, bl.y};
  cv::Point2d B{A.x + gap_, A.y};
  cv::Point2d C{B.x, B.y - car_room_length_};
  cv::Point2d D{C.x + car_room_width_, C.y};
  cv::Point2d E{D.x, D.y + car_room_length_};
  // 1.2 创建连线
  cv::line(main_canvas_, A, B, BLACK_SCALAR, 3);
  cv::line(main_canvas_, B, C, BLACK_SCALAR, 3);
  cv::line(main_canvas_, C, D, BLACK_SCALAR, 3);
  cv::line(main_canvas_, D, E, BLACK_SCALAR, 3);
  // 1.3 返回末位点位
  return E;
}

inline cv::Point2d Road::createDaokuBelow(const cv::Point2d& tl) {
  if ((tl.x + car_room_width_) > width_) {
    return {-1, -1};
  }
  // 1. 创建点位
  cv::Point2d A{tl.x, tl.y};
  cv::Point2d B{A.x + gap_, A.y};
  cv::Point2d C{B.x, B.y + car_room_length_};
  cv::Point2d D{C.x + car_room_width_, C.y};
  cv::Point2d E{D.x, D.y - car_room_length_};
  // 1.2 创建连线
  cv::line(main_canvas_, A, B, BLACK_SCALAR, 3);
  cv::line(main_canvas_, B, C, BLACK_SCALAR, 3);
  cv::line(main_canvas_, C, D, BLACK_SCALAR, 3);
  cv::line(main_canvas_, D, E, BLACK_SCALAR, 3);
  // 1.3 返回末位点位
  return E;
}

inline void Road::CreateDaoku() {
  cv::Point2d point{0, length_ - car_room_length_ - gap_};
  while (point.x != -1) {
    point = createDaokuBelow(point);
  }
  point = {0, gap_ + car_room_length_};
  while (point.x != -1) {
    point = createDaokuAbove(point);
  }
}

#endif  // OPENCV_DEMO_CAR_ROAD_HPP