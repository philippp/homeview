#include <string>

namespace cv {
  class Mat;
  class VideoWriter;
}
namespace features {
  class Transition;
}

class TransitionOutput {
 public:
  TransitionOutput(const std::string& filename);
  void WriteTransition(const features::Transition& transition);
};

class MediaOutput {
 public:
  virtual void WriteFrame(const cv::Mat& frame_matrix) = 0;
};

class VideoOutput : public MediaOutput {
 public:
  VideoOutput(const std::string& filename, int fps);
  virtual ~VideoOutput();
  void WriteFrame(const cv::Mat& frame_matrix) override;
 private:
  std::string filename_;
  int fps_;
  // Todo: use boost::scoped_ptr
  cv::VideoWriter* video_writer_;
};

class ImageOutput : public MediaOutput {
 public:
  ImageOutput(const std::string& directory, const std::string& file_prefix);
  void WriteFrame(const cv::Mat& frame_matrix) override;
  void WriteFrame(const cv::Mat& frame_matrix, std::string* filename);
 private:
  std::string directory_;
  std::string file_prefix_;
  int idx_;
};
