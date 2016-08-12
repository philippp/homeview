#include <fstream>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <glob.h>
#include "media_inputs.h"
#include "transition_features.pb.h"

#include "opencv2/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using std::vector;
using std::string;
using features::Transition;

inline void MatchFiles(const std::string& pattern, vector<string>* files){
  glob_t glob_result;
  files->clear();
  glob(pattern.c_str(), GLOB_TILDE, NULL, &glob_result);
  for(int i=0; i < glob_result.gl_pathc; ++i){
    files->push_back(string(glob_result.gl_pathv[i]));
  }
  globfree(&glob_result);
}

StreamInput::StreamInput(const string& source) {
  frame_idx_ = 0;
  if (std::isdigit(source[0], std::locale())) {
    int i = atoi(source.c_str());
    capture_.reset(new cv::VideoCapture(i));
  } else {
    capture_.reset(new cv::VideoCapture(source));
  }
  if (!capture_->isOpened()) {
    std::cout << "Failed to open " << source << "\n";
  }
}

bool StreamInput::GetFrame(cv::Mat* frame_matrix) {
  if (!capture_->read(*frame_matrix)) {
    std::cout << "Failed to read frame " << frame_idx_;
    return false;
  }
  frame_idx_++;
  return true;
}

FileInput::FileInput(const string& filenames) {
  vector<string> image_files;
  if (filenames.find("*") != std::string::npos) {
    // Scenario 1: Wildcard selection
    MatchFiles(filenames, &image_files);
  } // Scenario 2: List of files (below)
  else if (filenames.find(",") != std::string::npos) {
    FileInput::Split(filenames, ',', image_files);
  } else if (filenames.find(" ") != std::string::npos) {
    FileInput::Split(filenames, ' ', image_files);
  }
  else if (filenames.find("\n") != std::string::npos) {
    FileInput::Split(filenames, '\n', image_files);
  } else {
    std::cout << "Failed to split \"" << filenames << "\"\n";
  }	     
  image_files_ = image_files;
  file_idx_ = 0;
}

bool FileInput::GetFrame(cv::Mat* frame_matrix) {
  if (file_idx_ >= image_files_.size()) {
    return false;
  }
  cv::imread(image_files_[file_idx_], cv::IMREAD_COLOR).copyTo(*frame_matrix);
  file_idx_++;
  return true;
}

void FileInput::Split(const string& s, char delim, vector<string>& elems) {
  std::stringstream ss(s);
  string item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

