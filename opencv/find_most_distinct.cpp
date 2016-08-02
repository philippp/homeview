#include <set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include "opencv2/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/core/ocl.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/xfeatures2d.hpp"

using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;

void split(const string &s, char delim, vector<string> &elems) {
  stringstream ss(s);
  string item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

struct SURFDetector
{
    Ptr<Feature2D> surf;
    SURFDetector(double hessian = 800.0)
    {
        surf = SURF::create(hessian);
    }
    template<class T>
    void operator()(const T& in, const T& mask, std::vector<cv::KeyPoint>& pts, T& descriptors, bool useProvided = false)
    {
        surf->detectAndCompute(in, mask, pts, descriptors, useProvided);
    }
};

template<class KPMatcher>
struct SURFMatcher
{
    KPMatcher matcher;
    template<class T>
    void match(const T& in1, const T& in2, std::vector<cv::DMatch>& matches)
    {
        matcher.match(in1, in2, matches);
    }
};

static void ImageDifferences(const Mat& img1,
			     const Mat& img2,
			     std::vector<KeyPoint>& unmatched_keypoints)
{
  //declare input/output
  std::vector<KeyPoint> keypoints1, keypoints2;
  std::vector<DMatch> matches;

  UMat _descriptors1, _descriptors2;
  Mat descriptors1 = _descriptors1.getMat(ACCESS_RW);
  Mat descriptors2 = _descriptors2.getMat(ACCESS_RW);
  
  //instantiate detectors/matchers
  SURFDetector surf;
  SURFMatcher<BFMatcher> matcher;
  
  surf(img1, Mat(), keypoints1, descriptors1);
  surf(img2, Mat(), keypoints2, descriptors2);
  matcher.match(descriptors1, descriptors2, matches);

  std::vector<bool> kp1_matched(keypoints1.size(), false);
  std::vector<bool> kp2_matched(keypoints2.size(), false);
  for (const DMatch& match : matches) {
    kp1_matched[match.queryIdx] = true;
    kp2_matched[match.trainIdx] = true;
  }
  for (int i = 0; i < kp2_matched.size(); ++i) {
    if (!kp2_matched[i]) {
      if (keypoints2[i].octave > 2) {
	unmatched_keypoints.push_back(keypoints2[i]);
      }
    }
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

static void ProcessImageDiff(const string& img_file_1,
			     const string& img_file_2,
			     const string& diff_output_dir,
			     std::ofstream& manifest) {

  UMat img1, img2;
  imread(img_file_1, IMREAD_GRAYSCALE).copyTo(img1);
  if(img1.empty()) {
    std::cout << "Couldn't load " << img_file_1 << std::endl;
    return;
  }
  
  imread(img_file_2, IMREAD_GRAYSCALE).copyTo(img2);
  if(img2.empty()) {
    std::cout << "Couldn't load " << img_file_2 << std::endl;
    return;
  }
  const string img_id_1 = GetParentDirName(img_file_1);
  const string img_id_2 = GetParentDirName(img_file_2);
  const string output_file = diff_output_dir + "/" + img_id_1 + "_" + img_id_2 + ".jpeg";

  Mat output_image1;
  Mat output_image2;
  {	
    std::vector<KeyPoint> unmatched_keypoints;
    ImageDifferences(img1.getMat(ACCESS_READ),
		     img2.getMat(ACCESS_READ),
		     unmatched_keypoints);
    
    drawKeypoints(img2, unmatched_keypoints, output_image1,
		  Scalar(0,0,255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
    manifest << "diff," << img_id_1 << "," << img_id_2 <<
      "," << unmatched_keypoints.size() << "," << output_file << "\n";
  }
  {
    std::vector<KeyPoint> unmatched_keypoints;
    ImageDifferences(img2.getMat(ACCESS_READ),
		     img1.getMat(ACCESS_READ),
		     unmatched_keypoints);
    drawKeypoints(img1, unmatched_keypoints, output_image2,
		  Scalar(0,0,255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
    manifest << "diff," << img_id_2 << "," << img_id_1 << ","
	     << unmatched_keypoints.size() << "," << output_file << "\n";
  }
  Size sz1 = output_image1.size();
  Size sz2 = output_image2.size();
  Mat im3(sz1.height, sz1.width+sz2.width, CV_8UC3);
  Mat left(im3, Rect(0, 0, sz1.width, sz1.height));
  output_image1.copyTo(left);
  Mat right(im3, Rect(sz1.width, 0, sz2.width, sz2.height));
  output_image2.copyTo(right);
  imwrite(output_file, im3); //diff_output_dir + "/" + img_id_1 + "_" + img_id_2 + ".jpeg", im3);
  manifest.flush();
}

static void ProcessImageDiff2(const string& img_file_1,
			      const string& img_file_2,
			      const string& diff_output_dir,
			      std::ofstream& manifest) {			      
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
  const string output_file = diff_output_dir + "/" + img_id_1 + "_" + img_id_2 + ".jpeg";  
  cv::Mat diffImage;
  cv::absdiff(img1, img2, diffImage);
  cv::Mat foregroundMask = cv::Mat::zeros(diffImage.rows, diffImage.cols, CV_8UC1);

  float threshold = 80.0f;
  float dist;
  float dist_sum = 0;
  for(int j=0; j<diffImage.rows; ++j) {
    for(int i=0; i<diffImage.cols; ++i) {
      cv::Vec3b pix = diffImage.at<cv::Vec3b>(j,i);

      dist = (pix[0]*pix[0] + pix[1]*pix[1] + pix[2]*pix[2]);
      dist = sqrt(dist);
      
      if(dist>threshold) {
	dist_sum += dist;
	foregroundMask.at<unsigned char>(j,i) = 255;
      }
    }
  }
  manifest << "mask," << img_file_1 << "," << img_file_2 << ","
	   << dist_sum << "," << output_file << "\n";
  imwrite(output_file, foregroundMask);
}

////////////////////////////////////////////////////
// This program demonstrates the usage of SURF_OCL.
// use cpu findHomography interface to calculate the transformation matrix
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
    std::ofstream manifest_file(outpath + "/manifest.csv", std::ofstream::out);

    string image_files_str;
    string line;
    while (std::getline(std::cin, line)) {
	image_files_str += (line + " ");
    }
    vector<string> image_files;
    split(image_files_str, ' ', image_files);
    string last_image_file = "";
    for (const string& image_file : image_files) {
      if (last_image_file.empty()) {
	last_image_file = image_file;
	continue;
      }
      ProcessImageDiff2(last_image_file, image_file, outpath, manifest_file);
      cout << "Diffed " << last_image_file << " " << image_file << "\n";
      last_image_file = image_file;
    }
    return EXIT_SUCCESS;
}
