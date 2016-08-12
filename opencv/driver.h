#ifndef driver_h
#define driver_h

#include "media_outputs.h"
#include "media_inputs.h"
#include "event_detector.h"
#include <vector>
#include <memory>

class Driver {
 public:
  Driver();
  void SetInput(MediaInput* input);
  void SetInputCopy(MediaOutput* output);
  void SetSequenceOutput(SequenceOutput* output);
  void AddMediaOutput(MediaOutput* output);
  void Run(int cycles);

 private:
  std::vector<std::unique_ptr<MediaOutput>> outputs_;
  std::unique_ptr<MediaInput> input_;
  std::unique_ptr<MediaOutput> input_replica_;
  std::unique_ptr<MotionSensor> motion_sensor_;
  std::unique_ptr<SequenceOutput> sequence_output_;
};

#endif
