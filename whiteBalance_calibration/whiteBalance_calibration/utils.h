#pragma once
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
using namespace std;

bool is_file_exist(string fileName);
cv::Mat convertColor(cv::Mat src, const int* avgcolor);
int* PerfectReflectionAlgorithm(cv::Mat src, string path);
int calibration(string path, cv::Size size, float cube_size);