#include <string>
#include <memory>

namespace cv {
  class Mat;
  class VideoWriter;
}
namespace features {
  class Transition;
  class TransitionSequence;
}

/**
 * Saves the sequence protobuf metadata.
 */
class SequenceOutput {
 public:
  SequenceOutput(const std::string& destination_path);
  void WriteSequence(const features::TransitionSequence& transition);
 private:
  std::string destination_path_;
  uint64_t sequence_id_;
};

/**
 * Abstract superclass for media outputs, all of which can take a 
 * matrix and do something with it.
 */
class MediaOutput {
 public:
  virtual void WriteFrame(const cv::Mat& frame_matrix) = 0;
};

/**
 * Saves a sequence of frames as a video file.
 */
class VideoOutput : public MediaOutput {
 public:
  VideoOutput(const std::string& filename, int fps);
  void WriteFrame(const cv::Mat& frame_matrix) override;
 private:
  std::string filename_;
  int fps_;
  std::unique_ptr<cv::VideoWriter> video_writer_;
};

/**
 * Saves a sequence of frames as a set of JPEGs.
 */
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

/**
 * Renders a sequence of frames to an XWindow.
 */
class RenderedOutput : public MediaOutput {
 public:
  RenderedOutput(const std::string& window_name);
  void WriteFrame(const cv::Mat& frame_matrix) override;
 private:
  std::string window_name_;
};
