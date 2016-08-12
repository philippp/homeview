#include <stdio.h>
#include <fstream>

#include "opencv2/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "media_outputs.h"
#include "transition_features.pb.h"

using std::string;

VideoOutput::VideoOutput(const string& filename, int fps) {
  filename_ = filename;
  fps_ = fps;
}


void VideoOutput::WriteFrame(const cv::Mat& frame_matrix) {
  if (video_writer_.get() == nullptr) {
    video_writer_.reset(new cv::VideoWriter(
        filename_,
	CV_FOURCC('D','I','V','3'),
	fps_,
	cv::Size((int)frame_matrix.cols,
		 (int)frame_matrix.rows)));
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

SequenceOutput::SequenceOutput(const std::string& destination_path) {
  destination_path_ = destination_path;
  sequence_id_ = 0;
}

void SequenceOutput::WriteSequence(const features::TransitionSequence& sequence) {
  std::ofstream outstream = std::ofstream(
     destination_path_ + "/transition_sequence_" + std::to_string(sequence_id_) +
     ".rio", std::ofstream::out);
  outstream << sequence.SerializeAsString();
};

RenderedOutput::RenderedOutput(const string& window_name) {
  window_name_ = window_name;
}

void RenderedOutput::WriteFrame(const cv::Mat& frame_matrix) {
  cv::imshow(window_name_, frame_matrix);
  cv::waitKey(30);
}
