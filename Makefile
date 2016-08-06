CC = g++
LINK = g++
INSTALL = install
CFLAGS = `pkg-config opencv --cflags` -I /usr/include/boost-1_46 -I. -std=c++11 
LFLAGS = -L/usr/local/lib -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_aruco -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_dnn -lopencv_dpm -lopencv_fuzzy -lopencv_line_descriptor -lopencv_optflow -lopencv_plot -lopencv_reg -lopencv_saliency -lopencv_stereo -lopencv_structured_light -lopencv_rgbd -lopencv_surface_matching -lopencv_tracking -lopencv_datasets -lopencv_text -lopencv_face -lopencv_xfeatures2d -lopencv_shape -lopencv_video -lopencv_ximgproc -lopencv_calib3d -lopencv_features2d -lopencv_flann -lopencv_xobjdetect -lopencv_objdetect -lopencv_ml -lopencv_xphoto -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_photo -lopencv_imgproc -lopencv_core -L /usr/lib -lopencv_features2d -lopencv_nonfree `pkg-config --cflags --libs protobuf`

all: find_most_distinct

debug: CFLAGS += -DDEBUG -g
debug: find_most_distinct

opencv/transition_features.pb.cc: opencv/transition_features.proto
	protoc --cpp_out=. --python_out=. opencv/transition_features.proto

opencv/transition_features.pb.o: opencv/transition_features.pb.cc
	$(CC) $(CFLAGS) -o $@ -c $^

opencv/find_most_distinct.o: opencv/find_most_distinct.cpp opencv/transition_features.pb.cc
	$(CC) $(CFLAGS) -o $@ -c opencv/find_most_distinct.cpp

find_most_distinct: opencv/find_most_distinct.o opencv/transition_features.pb.o
	$(LINK) -o $@ $^ $(LFLAGS)

clean:
	rm -f find_most_distinct *.o
	rm -f opencv/*.pb.*
	rm -f opencv/*_pb2.py
	rm -f opencv/*.o
