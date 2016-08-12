#include <memory>
#include "opencv2/core.hpp"

namespace features {
  class Transition;
}


class MotionSensor {
 public:
  void ProcessFrame(const cv::Mat& frame,
		    cv::Mat* frame_diff,
		    features::Transition* transition);

  void CalcRSSDistance(const cv::Mat& frame_a,
		       const cv::Mat& frame_b,
		       cv::Mat* rss_distance_matrix,
		       features::Transition* transition) const;
 private:
  void FindHotRectangle(const cv::Mat& rss_distance_matrix,
			float min_significant_distance,
			uint8_t scaling_factor,
			features::Transition* transition) const;

  cv::Mat CreateHeatmap(const cv::Mat& source, uint16_t max) const;


  cv::Mat last_frame_;
};
