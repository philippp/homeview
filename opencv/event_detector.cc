#include <iostream>
#include <stdio.h>
#include "opencv2/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/imgproc.hpp"
#include "transition_features.pb.h"
#include "opencv2/highgui/highgui.hpp"
#include "event_detector.h"

void MotionSensor::ProcessFrame(const cv::Mat& frame,
				cv::Mat* frame_diff,
				features::Transition* transition) {
  if (last_frame_.empty()){
    frame.copyTo(*frame_diff);
    frame.copyTo(last_frame_);
    return;
  }

  transition->mutable_img_dimensions()->set_x(frame.cols);
  transition->mutable_img_dimensions()->set_y(frame.rows);
  cv::Mat rss_distance_matrix = cv::Mat(frame.rows, frame.cols, CV_16U);
  {  // Calculate the RSS Distance matrix and associated stats.
    CalcRSSDistance(last_frame_,
		    frame,
		    &rss_distance_matrix,
		    transition);
  }
  float std_dev = sqrt(transition->rss_distance_variance());
  FindHotRectangle(rss_distance_matrix,
		   4 * std_dev,
		   16,
		   transition);
  // Draw the heatmap and rectangle.
  CreateHeatmap(rss_distance_matrix, 5 * std_dev).copyTo(*frame_diff);
  const features::Transition_Rectangle& rect = transition->interest_box();
  cv::rectangle(*frame_diff,
		cv::Point(rect.top_left().x(),
			  rect.top_left().y()),
		cv::Point(rect.bottom_right().x(),
			  rect.bottom_right().y()),
		cv::Scalar(0,0,255),
		4);
  
  frame.copyTo(last_frame_);
}

void MotionSensor::FindHotRectangle(const cv::Mat& rss_distance_matrix,
				    float min_significant_distance,
				    uint8_t scaling_factor,
				    features::Transition* transition) const {

  cv::Point top_left;
  cv::Point bottom_right;
  uint32_t mean_rss_distance;

  cv::Mat matrix_scaled(rss_distance_matrix);
  while (scaling_factor > 1) {
    pyrDown(matrix_scaled,
	    matrix_scaled,
	    cv::Size(matrix_scaled.cols/2,
		 matrix_scaled.rows/2));
    scaling_factor = scaling_factor/2;
  }
  std::cout << "Min sig distance=" << min_significant_distance << "\n";
  float distance_sum = 0;
  for (int j=0; j<matrix_scaled.rows; ++j) {
    for (int i=0; i<matrix_scaled.cols; ++i) {
      int rss_distance = matrix_scaled.at<uint16_t>(j,i);
      if (rss_distance > min_significant_distance) {
	// The far sides of the frame keep moving as long as we're in an interesting area.
	if (bottom_right.y < j) {
	  bottom_right.y = j;
	}
	if (bottom_right.x < i) {
	  bottom_right.x = i;
	}
	if (top_left.y == 0 || j < top_left.y) {
	  top_left.y = j;
	}
	if (top_left.x == 0 || i < top_left.x) {
	  top_left.x = i;
	}
	if (top_left.x <= i && top_left.y <= j && bottom_right.x >= i && bottom_right.y >= j) {
	  distance_sum += rss_distance;
	}
      }
    }
  }
  uint32_t rectangle_size = ((bottom_right.x - top_left.x)
			     *(bottom_right.y - top_left.y));
  std::cout << "Rectangle size: " << rectangle_size << "\n";
  if (rectangle_size < 4) {
    mean_rss_distance = 0;
    return;
  }
  mean_rss_distance = static_cast<uint32_t>(static_cast<float>(distance_sum)/rectangle_size);

  std::cout << "Mean Rectangle RSS Distance: " << mean_rss_distance << "\n";
  uint16_t x_scale = rss_distance_matrix.cols / matrix_scaled.cols;
  uint16_t y_scale = rss_distance_matrix.cols / matrix_scaled.cols;

  top_left.x = std::max<uint16_t>(0, (top_left.x * x_scale) - x_scale);
  top_left.y = std::max<uint16_t>(0, (top_left.y * y_scale) - y_scale);
  bottom_right.x = std::min<uint16_t>(rss_distance_matrix.cols,
				       (bottom_right.x * x_scale) + x_scale);
  bottom_right.y = std::min<uint16_t>(rss_distance_matrix.rows,
				       (bottom_right.y * y_scale) + y_scale);
  rectangle_size = ((bottom_right.x - top_left.x)
		    *(bottom_right.y - top_left.y));
  std::cout << "Scaled rectangle size: " << rectangle_size << "\n";

  features::Transition_Rectangle* rect = transition->mutable_interest_box();
  rect->set_rss_distance_mean(mean_rss_distance);
  rect->mutable_top_left()->set_x(top_left.x);
  rect->mutable_top_left()->set_y(top_left.y);
  rect->mutable_bottom_right()->set_x(bottom_right.x);
  rect->mutable_bottom_right()->set_y(bottom_right.y);
}


void MotionSensor::CalcRSSDistance(const cv::Mat& frame_a,
				   const cv::Mat& frame_b,
				   cv::Mat* rss_distance_matrix,
				   features::Transition* transition) const {
  cv::Mat frame_diff;
  cv::absdiff(frame_a, frame_b, frame_diff);

  uint16_t rss_distance;
  float sum_rss_distance = 0;
  float mean_rss_distance = 0;
  float variance;
  int n = 0;
  float m2 = 0;  
  for(int i=0; i < frame_diff.rows; ++i) {
    for(int j=0; j < frame_diff.cols; ++j) {
      n += 1;
      cv::Vec3b pix = frame_diff.at<cv::Vec3b>(i,j);
      rss_distance = static_cast<uint16_t>(
          sqrt(pix[0]*pix[0] + pix[1]*pix[1] + pix[2]*pix[2]));
      float delta = rss_distance - mean_rss_distance;
      mean_rss_distance += delta / n;
      m2 += delta * (rss_distance - mean_rss_distance);
      rss_distance_matrix->at<uint16_t>(i,j) = rss_distance;
      sum_rss_distance += rss_distance;
    }
  }
  transition->set_rss_distance_variance(m2 / (n - 1));
  transition->set_rss_distance_sum(sum_rss_distance);
  transition->set_rss_distance_mean(mean_rss_distance);
}

cv::Mat MotionSensor::CreateHeatmap(const cv::Mat& source, uint16_t max) const {
  cv::Mat heatmap = cv::Mat::zeros(source.rows, source.cols, CV_8UC1);
  for (int i=0; i < source.rows; ++i) {
    for (int j=0; j < source.cols; ++j) {
      uint16_t sourceval = source.at<uint16_t>(i,j);
      float scaling_factor = static_cast<float>(sourceval)/static_cast<float>(max);
      uint8_t scaled = std::min(255, static_cast<int>(255 * scaling_factor));
      heatmap.at<unsigned char>(i,j) = scaled;
    }
  }
  cv::applyColorMap(heatmap, heatmap, cv::COLORMAP_JET);
  return heatmap;
}
