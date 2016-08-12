#include "driver.h"
#include "opencv2/core.hpp"
#include "transition_features.pb.h"

// Set the input media for this driver.
Driver::Driver() {
  motion_sensor_.reset(new MotionSensor());
}
void Driver::SetInput(MediaInput* input) {
  input_.reset(input);
}

void Driver::AddOutput(MediaOutput* output) {
  outputs_.push_back(std::unique_ptr<MediaOutput>(output));
}

void Driver::SaveReplicatedInput(MediaOutput* output) {
  input_replica_.reset(output);
}

void Driver::Run(int cycles) {
  bool is_frame_returned = true;
  for (int i = 0; i < cycles && is_frame_returned; ++i) {
    cv::Mat frame;
    is_frame_returned = input_->GetFrame(&frame);
    if (input_replica_.get() != nullptr) {
      input_replica_->WriteFrame(frame);
    }
    // Do frame processing
    cv::Mat processed_frame;
    features::Transition transition;
    motion_sensor_->ProcessFrame(frame, &processed_frame, &transition);
    for (int output_idx = 0; output_idx < outputs_.size(); ++output_idx) {
      outputs_[output_idx]->WriteFrame(processed_frame);
    }
  }
}

int main(int argc, char* argv[]) {
  Driver driver;
  driver.SetInput(new StreamInput("1"));
  driver.SaveReplicatedInput(new ImageOutput("/tmp", "input_"));
  driver.AddOutput(new VideoOutput("video.avi", 20));
  driver.Run(50);
}
