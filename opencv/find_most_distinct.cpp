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
  float rss_distance;
  int n = 0;
  float m2 = 0;  
  for(int j=0; j<diffImage.rows; ++j) {
    for(int i=0; i<diffImage.cols; ++i) {
      n += 1;
      cv::Vec3b pix = diffImage.at<cv::Vec3b>(j,i);
      rss_distance = sqrt((pix[0]*pix[0] + pix[1]*pix[1] + pix[2]*pix[2]));
      float delta = rss_distance - *mean_rss_distance;
      *mean_rss_distance += delta / n;
      m2 += delta * (rss_distance - *mean_rss_distance);
      rss_distance_matrix->at<float>(j,i) = rss_distance;
      *sum_rss_distance += rss_distance;
    }
  }
  *variance = m2 / (n - 1);
}

static void ProcessImageDiff2(const string& img_file_1,
			      const string& img_file_2,
			      const string& diff_output_dir,
			      features::TransitionSequence* sequence) {
  features::Transition transition;
  transition.set_img_file_1(img_file_1);
  transition.set_img_file_2(img_file_2);

  UMat img1_, img2_, img1, img2;
  imread(img_file_1, IMREAD_COLOR).copyTo(img1_);
  if(img1_.empty()) {
    std::cout << "Couldn't load " << img_file_1 << std::endl;
    return;
  }
  
  imread(img_file_2, IMREAD_COLOR).copyTo(img2_);
  if(img2_.empty()) {
    std::cout << "Couldn't load " << img_file_2 << std::endl;
    return;
  }
  pyrDown( img1_, img1, Size( img1_.cols/2, img1_.rows/2 ));
  pyrDown( img1, img1_, Size( img1.cols/2, img1.rows/2 ));
  pyrDown( img1_, img1, Size( img1_.cols/2, img1_.rows/2 ));

  pyrDown( img2_, img2, Size( img2_.cols/2, img2_.rows/2 ));
  pyrDown( img2, img2_, Size( img2.cols/2, img2.rows/2 ));
  pyrDown( img2_, img2, Size( img2_.cols/2, img2_.rows/2 ));
  transition.mutable_img_dimensions()->set_x(img1.cols);
  transition.mutable_img_dimensions()->set_y(img1.rows);
  
  const string img_id_1 = GetParentDirName(img_file_1);
  const string img_id_2 = GetParentDirName(img_file_2);
  transition.set_id(img_id_1 + "_" + img_id_2);
  const string output_file = diff_output_dir + "/" + img_id_1 + "_" + img_id_2 + ".jpeg";
  transition.set_img_file_diff(output_file);
  cv::Mat rss_distance_matrix = cv::Mat(img1.rows, img1.cols, CV_64F);
  {
    float sum_rss_distance = 0;
    float mean_rss_distance = 0;
    float variance;
    cv::Mat diffImage;
    cv::absdiff(img1, img2, diffImage);
    CalcRSSDistance(diffImage,
		    &rss_distance_matrix,
		    &sum_rss_distance,
		    &variance,
		    &mean_rss_distance);
    transition.set_rss_distance_sum(sum_rss_distance);
    transition.set_rss_distance_variance(variance);
    transition.set_rss_distance_mean(mean_rss_distance);
  }
  float std_dev = sqrt(transition.rss_distance_variance());
  cv::Point top_left;
  cv::Point bottom_right;
  cv::Mat foregroundMask = cv::Mat::zeros(rss_distance_matrix.rows,
					  rss_distance_matrix.cols,
					  CV_8UC1);
  float rss_distance_sum_in_rectangle = 0;
  float rss_distance;
  float max_rss_distance = transition.rss_distance_mean() + 5 * std_dev;
  for (int j=0; j<rss_distance_matrix.rows; ++j) {
    for (int i=0; i<rss_distance_matrix.cols; ++i) {
      rss_distance = rss_distance_matrix.at<float>(j,i);
      if (rss_distance > 3 * std_dev) {
	// The far sides of the frame keep moving as long as we're in an interesting area.
	if (bottom_right.y < j) {
	  bottom_right.y = j;
	}
	if (bottom_right.x < i) {
	  bottom_right.x = i;
	}
	if (top_left.y == 0 || j < top_left.y) {
	  top_left.y = j;
	}
	if (top_left.x == 0 || i < top_left.x) {
	  top_left.x = i;
	}
	if (top_left.x <= i && top_left.y <= j && bottom_right.x >= i && bottom_right.y >= j) {
	  rss_distance_sum_in_rectangle += rss_distance;
	}
      }
      foregroundMask.at<unsigned char>(j,i) = 255 * (rss_distance/max_rss_distance);
    }
  }
  features::Transition_Rectangle* rect = transition.mutable_interest_box();
  rect->set_rss_distance_sum(rss_distance_sum_in_rectangle);
  rect->mutable_top_left()->set_x(top_left.x);
  rect->mutable_top_left()->set_y(top_left.y);
  rect->mutable_bottom_right()->set_x(bottom_right.x);
  rect->mutable_bottom_right()->set_y(bottom_right.y);

  sequence->add_transitions()->CopyFrom(transition);
  Mat im_color;
  applyColorMap(foregroundMask, im_color, COLORMAP_JET);
  rectangle(im_color, top_left, bottom_right, Scalar(0,0,255), 1);
  imwrite(output_file, im_color);
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
