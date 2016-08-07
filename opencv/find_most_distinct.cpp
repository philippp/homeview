#include <iostream>
#include <fstream>
#include <stdio.h>
#include "opencv2/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "transition_features.pb.h"

using namespace std;
using namespace cv;

void split(const string &s, char delim, vector<string> &elems) {
  stringstream ss(s);
  string item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

static string GetParentDirName(const string& filename) {
  vector<string> file_tokens;
  split(filename, '/', file_tokens);
  if (file_tokens.size() > 1) {
    return file_tokens[file_tokens.size() - 2];
  }
  return "";
}

/**
 * Given a map of RGB difference values, calculate the corresponding map of
 * RSS edit distance (i.e. sqrt(dR^2+dG^2+dR^2)) as well as some statistics
 * about the image difference.
 */
static void CalcRSSDistance(const cv::Mat& diffImage,
			    cv::Mat* rss_distance_matrix,
			    float* mean_rss_distance,
			    float* sum_rss_distance,
			    float* variance) {
  uint16_t rss_distance;
  int n = 0;
  float m2 = 0;  
  for(int j=0; j<diffImage.rows; ++j) {
    for(int i=0; i<diffImage.cols; ++i) {
      n += 1;
      cv::Vec3b pix = diffImage.at<cv::Vec3b>(j,i);
      rss_distance = static_cast<uint16_t>(sqrt((pix[0]*pix[0] + pix[1]*pix[1] + pix[2]*pix[2])));
      float delta = rss_distance - *mean_rss_distance;
      *mean_rss_distance += delta / n;
      m2 += delta * (rss_distance - *mean_rss_distance);
      rss_distance_matrix->at<uint16_t>(j,i) = rss_distance;
      *sum_rss_distance += rss_distance;
    }
  }
  *variance = m2 / (n - 1);
}

static void FindHotRectangle(const cv::Mat& rss_distance_matrix,
			     float min_significant_distance,
			     int64 scaling_factor,
			     cv::Point* top_left,
			     cv::Point* bottom_right,
			     float* mean_rss_distance,
			     float* variance) {
  cv::Mat matrix_scaled(rss_distance_matrix);
  while (scaling_factor > 1) {
    pyrDown(matrix_scaled,
	    matrix_scaled,
	    Size(matrix_scaled.cols/2,
		 matrix_scaled.rows/2));
    scaling_factor = scaling_factor/2;
  }
  float distance_sum = 0;
  for (int j=0; j<matrix_scaled.rows; ++j) {
    for (int i=0; i<matrix_scaled.cols; ++i) {
      int rss_distance = matrix_scaled.at<uint16_t>(j,i);
      if (rss_distance > min_significant_distance) {
	// The far sides of the frame keep moving as long as we're in an interesting area.
	if (bottom_right->y < j) {
	  bottom_right->y = j;
	}
	if (bottom_right->x < i) {
	  bottom_right->x = i;
	}
	if (top_left->y == 0 || j < top_left->y) {
	  top_left->y = j;
	}
	if (top_left->x == 0 || i < top_left->x) {
	  top_left->x = i;
	}
	if (top_left->x <= i && top_left->y <= j && bottom_right->x >= i && bottom_right->y >= j) {
	  distance_sum += rss_distance;
	}
      }
    }
  }
  *mean_rss_distance = (distance_sum/(matrix_scaled.rows * matrix_scaled.cols));
  *variance = 0;

  uint16_t x_scale = rss_distance_matrix.cols / matrix_scaled.cols;
  uint16_t y_scale = rss_distance_matrix.cols / matrix_scaled.cols;

  top_left->x = std::max<uint16_t>(0, (top_left->x * x_scale) - x_scale);
  top_left->y = std::max<uint16_t>(0, (top_left->y * y_scale) - y_scale);
  bottom_right->x = std::min<uint16_t>(rss_distance_matrix.cols,
				       (bottom_right->x * x_scale) + x_scale);
  bottom_right->y = std::min<uint16_t>(rss_distance_matrix.rows,
				       (bottom_right->y * y_scale) + y_scale);
}
			     
static void ProcessImageDiff2(const string& img_file_1,
			      const string& img_file_2,
			      const string& diff_output_dir,
			      features::TransitionSequence* sequence) {
  features::Transition transition;
  transition.set_img_file_1(img_file_1);
  transition.set_img_file_2(img_file_2);

  UMat img1, img2;
  imread(img_file_1, IMREAD_COLOR).copyTo(img1);
  if(img1.empty()) {
    std::cout << "Couldn't load " << img_file_1 << std::endl;
    return;
  }
  
  imread(img_file_2, IMREAD_COLOR).copyTo(img2);
  if(img2.empty()) {
    std::cout << "Couldn't load " << img_file_2 << std::endl;
    return;
  }
  const string img_id_1 = GetParentDirName(img_file_1);
  const string img_id_2 = GetParentDirName(img_file_2);

  transition.mutable_img_dimensions()->set_x(img1.cols);
  transition.mutable_img_dimensions()->set_y(img1.rows);
  
  transition.set_id(img_id_1 + "_" + img_id_2);
  cv::Mat rss_distance_matrix = cv::Mat(img1.rows, img1.cols, CV_16U);
  {  // Calculate the RSS Distance matrix and associated stats.
    float sum_rss_distance = 0;
    float mean_rss_distance = 0;
    float variance;
    cv::Mat diffImage;
    cv::absdiff(img1, img2, diffImage);
    CalcRSSDistance(diffImage,
		    &rss_distance_matrix,
		    &mean_rss_distance,
		    &sum_rss_distance,
		    &variance);
    transition.set_rss_distance_sum(sum_rss_distance);
    transition.set_rss_distance_variance(variance);
    transition.set_rss_distance_mean(mean_rss_distance);
  }
  float std_dev = sqrt(transition.rss_distance_variance());
  cv::Point top_left;
  cv::Point bottom_right;
  float rss_distance_mean_in_rectangle;
  float rss_distance_variance_in_rectangle;
  FindHotRectangle(rss_distance_matrix,
		   4 * std_dev,
		   16,
		   &top_left,
		   &bottom_right,
		   &rss_distance_mean_in_rectangle,
		   &rss_distance_variance_in_rectangle);
  // Let's draw our results!
  cv::Mat foregroundMask = cv::Mat::zeros(rss_distance_matrix.rows,
					  rss_distance_matrix.cols,
					  CV_8UC1);
  uint16_t max_rss_distance = transition.rss_distance_mean() + 5 * std_dev;
  for (int j=0; j<rss_distance_matrix.rows; ++j) {
    for (int i=0; i<rss_distance_matrix.cols; ++i) {
      uint16_t rss_distance = rss_distance_matrix.at<uint16_t>(j,i);
      foregroundMask.at<unsigned char>(j,i) = 255 * (static_cast<float>(rss_distance)/static_cast<float>(max_rss_distance));
    }
  }
  features::Transition_Rectangle* rect = transition.mutable_interest_box();
  rect->set_rss_distance_mean(rss_distance_mean_in_rectangle);
  rect->mutable_top_left()->set_x(top_left.x);
  rect->mutable_top_left()->set_y(top_left.y);
  rect->mutable_bottom_right()->set_x(bottom_right.x);
  rect->mutable_bottom_right()->set_y(bottom_right.y);

  Mat im_color;
  applyColorMap(foregroundMask, im_color, COLORMAP_JET);
  rectangle(im_color, top_left, bottom_right, Scalar(0,0,255), 4);
  const string output_file = diff_output_dir + "/" + img_id_1 + "_" + img_id_2 + ".jpeg";
  transition.set_img_file_diff(output_file);
  imwrite(output_file, im_color);
  sequence->add_transitions()->CopyFrom(transition);
}

int main(int argc, char* argv[])
{
    const char* keys =
      "{ h help     | false            | print help message  }"
      "{ o output   | ./               | specify comparison output path }";

    CommandLineParser cmd(argc, argv, keys);
    if (cmd.get<bool>("help"))
    {
        std::cout << "Usage: surf_matcher [options]" << std::endl;
        std::cout << "Available options:" << std::endl;
        cmd.printMessage();
        return EXIT_SUCCESS;
    }
    string outpath = cmd.get<string>("o");
    std::ofstream manifest_file(outpath + "/sequence_metadata.rio", std::ofstream::out);

    string image_files_str;
    string line;
    while (std::getline(std::cin, line)) {
	image_files_str += (line + " ");
    }
    vector<string> image_files;
    split(image_files_str, ' ', image_files);
    string last_image_file = "";
    features::TransitionSequence sequence;
    for (const string& image_file : image_files) {
      if (last_image_file.empty()) {
	last_image_file = image_file;
	continue;
      }
      ProcessImageDiff2(last_image_file, image_file, outpath, &sequence);
      cout << "Diffed " << last_image_file << " " << image_file << "\n";
      last_image_file = image_file;
    }
    manifest_file << sequence.SerializeAsString();
    return EXIT_SUCCESS;
}
