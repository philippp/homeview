#include "driver.h"
#include "opencv2/core.hpp"
#include "transition_features.pb.h"
#include "opencv2/core/utility.hpp"

using std::string;

// Set the input media for this driver.
Driver::Driver() {
  motion_sensor_.reset(new MotionSensor());
}
void Driver::SetInput(MediaInput* input) {
  input_.reset(input);
}

void Driver::SetSequenceOutput(SequenceOutput* output) {
  sequence_output_.reset(output);
}

void Driver::AddMediaOutput(MediaOutput* output) {
  outputs_.push_back(std::unique_ptr<MediaOutput>(output));
}

void Driver::SetInputCopyOutput(MediaOutput* output) {
  input_replica_.reset(output);
}

void Driver::Run(int cycles) {
  bool is_frame_returned = true;
  features::TransitionSequence sequence;
  for (int i = 0; i < cycles && is_frame_returned; ++i) {
    cv::Mat frame;
    is_frame_returned = input_->GetFrame(&frame);
    if (input_replica_.get() != nullptr) {
      input_replica_->WriteFrame(frame);
    }
    cv::Mat processed_frame;
    motion_sensor_->ProcessFrame(frame, &processed_frame, sequence.add_transitions());
    for (int output_idx = 0; output_idx < outputs_.size(); ++output_idx) {
      outputs_[output_idx]->WriteFrame(processed_frame);
    }
  }
  if (sequence_output_.get() != nullptr) {
    sequence_output_->WriteSequence(sequence);
  }

}

int main(int argc, char* argv[]) {
  const char* keys =
    "{ h help     | false            | print help message  }"
    "{ c capture  |                  | capture video from a stream  }"
    "{ f files    |                  | load from comma-separated list of files }"
    "{ o output   | ./               | specify comparison output path }";
  
  cv::CommandLineParser cmd(argc, argv, keys);
  if (cmd.get<bool>("help")){
    std::cout << "Usage: find_most_distinct [options]" << std::endl;
    std::cout << "Available options:" << std::endl;
    cmd.printMessage();
    return EXIT_SUCCESS;
  }
  string outpath = cmd.get<string>("o");
  string capture_source = cmd.get<string>("c");
  string files = cmd.get<string>("f");
  
  Driver driver;
  driver.SetInput(new StreamInput("1"));
  //driver.SetInputCopyOutput(new ImageOutput("/tmp", "input_"));
  //driver.SetInputCopyOutput(new VideoOutput("input.avi", 20));
  //driver.AddMediaOutput(new VideoOutput("video.avi", 20));
  driver.AddMediaOutput(new RenderedOutput("processed"));
  driver.SetSequenceOutput(new SequenceOutput("/tmp"));
  driver.Run(5000);
}
