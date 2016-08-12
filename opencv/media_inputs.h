#include <vector>
#include <iostream>
#include <stdio.h>
#include <memory>

namespace cv {
  class Mat;
  class VideoCapture;
}

class MediaInput {
 public:
  virtual bool GetFrame(cv::Mat* frame_matrix) = 0;
};

class StreamInput : public MediaInput {
 public:
  StreamInput(const std::string& source);
  bool GetFrame(cv::Mat* frame_matrix) override;
 private:
  std::unique_ptr<cv::VideoCapture> capture_;
  int frame_idx_;
};

class FileInput : public MediaInput {
 public:
  FileInput(const std::string& files);
  bool GetFrame(cv::Mat* frame_matrix) override;
 protected:
  static void Split(const std::string& s, char delim, std::vector<std::string>& elems);
 private:
  std::vector<std::string> image_files_;
  int file_idx_;
};
