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

void Driver::SetInputCopy(MediaOutput* output) {
  input_replica_.reset(output);
}

void Driver::Run(int cycles) {
  bool is_frame_returned = true;
  features::TransitionSequence sequence;
  for (int i = 0; (i < cycles || cycles < 0) && is_frame_returned; ++i) {
    cv::Mat frame;
    is_frame_returned = input_->GetFrame(&frame);
    if (!is_frame_returned) {
      break;
    }
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

/**
 * Expected use cases:
 * 1. Training
 * 1.a. Capture training frames from a streaming device
 * 1.b. Iterate filters on sampled training frames
 * 1.c. review annotations and scenes
 * 2. Production
 * 2.a. Capture scenes from a streaming device
 */
int main(int argc, char* argv[]) {
  const char* keys =
    "{ h help       | false | print help message  }"
    "{ v video_in   |       | capture images from a video or device  }"
    "{ i images_in  |       | load from wildcard or list of images }"
    "{ c copy_input | false | Copy all input media to output directory }"
    "{ d destination| ./    | destination directory }"
    "{ o outputs    |       | scene outputs: (i)mage, (v)ideo, (r)endered}"
    "{ a annotation |       | annotation outputs: (i)mage, (v)ideo, (r)endered }"
    "{ l run_length | -1    | how many frames to request (default is unlimited) }";
  
  cv::CommandLineParser cmd(argc, argv, keys);
  if (cmd.get<bool>("help")){
    std::cout << "Usage: find_most_distinct [options]" << std::endl;
    std::cout << "Available options:" << std::endl;
    cmd.printMessage();
    return EXIT_SUCCESS;
  }
  string outpath = cmd.get<string>("d");
  if (!system(("mkdir -p " + outpath).c_str())) {
    std::cout << "Failed to create " + outpath;
  }
  string video_in = cmd.get<string>("v");
  string images_in = cmd.get<string>("i");
  bool copy_input = cmd.get<bool>("c");
  string annotations = cmd.get<string>("a");
  int run_length = cmd.get<int>("l");
  
  Driver driver;
  if (!images_in.empty() && !video_in.empty()) {
    std::cout << "Please choose either video or image inputs, not both.\n";
  }
  if (!images_in.empty()) {
    driver.SetInput(new FileInput(images_in));
  } else if (!video_in.empty()) {
    driver.SetInput(new StreamInput(video_in));
  } else {
    std::cout << "No input specified, trying device #1.";
    driver.SetInput(new StreamInput("1"));
  }
  if (copy_input) {
    driver.SetInputCopy(new ImageOutput(outpath, "input_"));
  }
  if (annotations.empty()) {
    std::cout << "No annotation output specified.";
  } 
  for (const char& c : annotations) {
    switch(c) {
    case 'r':
      driver.AddMediaOutput(new RenderedOutput("annotated"));
      break;
    case 'i':
      driver.AddMediaOutput(new ImageOutput(outpath, "annotated_"));
      break;
    case 'v':
      driver.AddMediaOutput(new VideoOutput(outpath, "annotated", 20));
      break;
    }
  }
  driver.SetSequenceOutput(new SequenceOutput(outpath));
  driver.Run(run_length);
}
