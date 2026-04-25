
#ifndef OPENCV_DEMO_CAR_CONFIG_HPP
#define OPENCV_DEMO_CAR_CONFIG_HPP

#pragma once

#define CV_KEY_ESCAPE 0x1B
#define CV_KEY_LEFT 'a'
#define CV_KEY_UP 'w'
#define CV_KEY_RIGHT 'd'
#define CV_KEY_DOWN 's'

#define WHITE_SCALAR cv::Scalar{255, 255, 255}
#define BLACK_SCALAR cv::Scalar{0, 0, 0}
#define CAR_COLOR cv::Scalar{255, 0, 0}

const struct {
  int MainRows = 800;
  int MainCols = 851;

  double CarWidth = 100;
  double CarLength = 200;

  double Space = 20;
  double CarRoomWidth = 150;
  double CarRoomLength = 250;

  double fps = 0;
} GlobalConfig;

#endif  // OPENCV_DEMO_CAR_CONFIG_HPP