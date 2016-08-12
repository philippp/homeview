#include <stdio.h>

#include "opencv2/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "media_outputs.h"

using std::string;

VideoOutput::VideoOutput(const string& filename, int fps) {
  filename_ = filename;
  fps_ = fps;
  video_writer_ = NULL;
}

VideoOutput::~VideoOutput() {
  delete video_writer_;
  video_writer_ = NULL;
}

void VideoOutput::WriteFrame(const cv::Mat& frame_matrix) {
  if (video_writer_ == NULL) {
    video_writer_ = new cv::VideoWriter(
        filename_,
	CV_FOURCC('X','V','I','D'),
	fps_,
	cv::Size((int)frame_matrix.cols,
		 (int)frame_matrix.rows));
  }
  video_writer_->write(frame_matrix);
}

ImageOutput::ImageOutput(const string& directory, const string& file_prefix) {
  directory_ = directory;
  file_prefix_ = file_prefix;
  idx_ = 0;
}

void ImageOutput::WriteFrame(const cv::Mat& frame_matrix) {
  string filename;
  WriteFrame(frame_matrix, &filename);
}

void ImageOutput::WriteFrame(const cv::Mat& frame_matrix, string* filename) {
  *filename = directory_ + "/" + file_prefix_+ std::to_string(idx_) + ".jpeg";
  cv::imwrite(*filename, frame_matrix);
  idx_ += 1;
}
