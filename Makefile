CC = g++
LINK = g++
INSTALL = install
CFLAGS = `pkg-config opencv --cflags` -I /usr/include/boost-1_46 -I. -std=c++14 
LFLAGS = -L/usr/local/lib -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_aruco -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_dnn -lopencv_dpm -lopencv_fuzzy -lopencv_line_descriptor -lopencv_optflow -lopencv_plot -lopencv_reg -lopencv_saliency -lopencv_stereo -lopencv_structured_light -lopencv_rgbd -lopencv_surface_matching -lopencv_tracking -lopencv_datasets -lopencv_text -lopencv_face -lopencv_xfeatures2d -lopencv_shape -lopencv_video -lopencv_ximgproc -lopencv_calib3d -lopencv_features2d -lopencv_flann -lopencv_xobjdetect -lopencv_objdetect -lopencv_ml -lopencv_xphoto -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_photo -lopencv_imgproc -lopencv_core -L /usr/lib -lopencv_features2d -lopencv_nonfree `pkg-config --cflags --libs protobuf`

all: opencv/driver

debug: CFLAGS += -DDEBUG -g
debug: opencv/driver

opencv/transition_features.pb.cc: opencv/transition_features.proto
	protoc --cpp_out=. --python_out=. opencv/transition_features.proto

opencv/transition_features.pb.o: opencv/transition_features.pb.cc
	$(CC) $(CFLAGS) -o $@ -c $^

opencv/event_detector.o: opencv/event_detector.cc
	$(CC) $(CFLAGS) -o $@ -c $^

opencv/media_inputs.o: opencv/media_inputs.cc
	$(CC) $(CFLAGS) -o $@ -c $^

opencv/media_outputs.o: opencv/media_outputs.cc
	$(CC) $(CFLAGS) -o $@ -c $^

opencv/driver.o: opencv/driver.cc
	$(CC) $(CFLAGS) -o $@ -c $^

opencv/driver: opencv/driver.o opencv/transition_features.pb.o opencv/media_inputs.o opencv/media_outputs.o opencv/event_detector.o
	$(LINK) -o $@ $^ $(LFLAGS)

clean:
	rm -f opencv/driver *.o
	rm -f opencv/*.pb.*
	rm -f opencv/*_pb2.py
	rm -f opencv/*.o
