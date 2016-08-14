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

#include <iostream>
#include <string>
#include <cstdio>

//http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
template<typename ... Args>
string StringFormat( const std::string& format, Args ... args )
{
  size_t size = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
  std::unique_ptr<char[]> buf( new char[ size ] ); 
  std::snprintf( buf.get(), size, format.c_str(), args ... );
  return string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

VideoOutput::VideoOutput(const string& directory,
			 const string& file_prefix,
			 int fps) {
  directory_ = directory;
  file_prefix_ = file_prefix;
  fps_ = fps;
}


void VideoOutput::WriteFrame(const cv::Mat& frame_matrix) {
  if (video_writer_.get() == nullptr) {
    video_writer_.reset(new cv::VideoWriter(
        directory_ + '/' + file_prefix_ + ".avi",
	CV_FOURCC('D','I','V','3'),
	fps_,
	cv::Size((int)frame_matrix.cols,
		 (int)frame_matrix.rows)));
  }
  video_writer_->write(frame_matrix);
}

ImageOutput::ImageOutput(const string& directory,
			 const string& file_prefix) {
  directory_ = directory;
  file_prefix_ = file_prefix;
  idx_ = 0;
}

void ImageOutput::WriteFrame(const cv::Mat& frame_matrix) {
  string filename;
  WriteFrame(frame_matrix, &filename);
}

void ImageOutput::WriteFrame(const cv::Mat& frame_matrix,
			     string* filename) {
  *filename = directory_ + "/" + file_prefix_+ StringFormat("%07d",idx_) + ".jpeg";
  cv::imwrite(*filename, frame_matrix);
  idx_ += 1;
}

SequenceOutput::SequenceOutput(const std::string& destination_path) {
  destination_path_ = destination_path;
  sequence_id_ = 0;
}

void SequenceOutput::WriteSequence(const features::TransitionSequence& sequence) {
  std::ofstream outstream = std::ofstream(
     destination_path_ + "/transition_sequence_" + StringFormat("%07d", sequence_id_) +
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
