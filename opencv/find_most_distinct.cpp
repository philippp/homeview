#include <set>
#include <iostream>
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

using namespace cv;
using namespace cv::xfeatures2d;

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

static void findUnmatchedKPs(const std::vector<bool> kp_matched,
			     const std::vector<KeyPoint>& keypoints,
			     std::vector<KeyPoint>& unmatched_keypoints) {
}

static void ImageDifferences(const Mat& img1,
			     const Mat& img2,
			     const std::vector<KeyPoint>& keypoints1,
			     const std::vector<KeyPoint>& keypoints2,
			     std::vector<DMatch>& matches,
			     std::vector<KeyPoint>& unmatched_keypoints)
{
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
      std::cout << keypoints2[i].octave << "\n"; 
    }
  }
}

////////////////////////////////////////////////////
// This program demonstrates the usage of SURF_OCL.
// use cpu findHomography interface to calculate the transformation matrix
int main(int argc, char* argv[])
{
    const char* keys =
      "{ h help     | false            | print help message  }"
      "{ l left     | box.png          | specify left image  }"
      "{ r right    | box_in_scene.png | specify right image }"
      "{ o output   | SURF_output.jpg  | specify output save path }"
      "{ m cpu_mode | false            | run without OpenCL }";
   

    CommandLineParser cmd(argc, argv, keys);
    if (cmd.get<bool>("help"))
    {
        std::cout << "Usage: surf_matcher [options]" << std::endl;
        std::cout << "Available options:" << std::endl;
        cmd.printMessage();
        return EXIT_SUCCESS;
    }
    if (cmd.has("cpu_mode"))
    {
        ocl::setUseOpenCL(false);
        std::cout << "OpenCL was disabled" << std::endl;
    }

    UMat img1, img2;

    std::string outpath = cmd.get<std::string>("o");
    std::string leftName = cmd.get<std::string>("l");
    imread(leftName, IMREAD_GRAYSCALE).copyTo(img1);
    if(img1.empty())
    {
        std::cout << "Couldn't load " << leftName << std::endl;
        cmd.printMessage();
        return EXIT_FAILURE;
    }

    std::string rightName = cmd.get<std::string>("r");
    imread(rightName, IMREAD_GRAYSCALE).copyTo(img2);
    if(img2.empty())
    {
        std::cout << "Couldn't load " << rightName << std::endl;
        cmd.printMessage();
        return EXIT_FAILURE;
    }

    //declare input/output
    std::vector<KeyPoint> keypoints1, keypoints2;
    std::vector<DMatch> matches;

    UMat _descriptors1, _descriptors2;
    Mat descriptors1 = _descriptors1.getMat(ACCESS_RW),
        descriptors2 = _descriptors2.getMat(ACCESS_RW);

    //instantiate detectors/matchers
    SURFDetector surf;

    SURFMatcher<BFMatcher> matcher;

    surf(img1.getMat(ACCESS_READ), Mat(), keypoints1, descriptors1);
    surf(img2.getMat(ACCESS_READ), Mat(), keypoints2, descriptors2);
    matcher.match(descriptors1, descriptors2, matches);
    std::vector<KeyPoint> unmatched_keypoints;
    ImageDifferences(img1.getMat(ACCESS_READ),
		     img2.getMat(ACCESS_READ),
		     keypoints1,
		     keypoints2,
		     matches,
		     unmatched_keypoints);
    Mat output_image1;
    drawKeypoints(img2, unmatched_keypoints, output_image1, Scalar::all(-1), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

    unmatched_keypoints.clear();
    matches.clear();
    matcher.match(descriptors2, descriptors1, matches);
    ImageDifferences(img2.getMat(ACCESS_READ),
		     img1.getMat(ACCESS_READ),
		     keypoints2,
		     keypoints1,
		     matches,
		     unmatched_keypoints);
    Mat output_image2;
    drawKeypoints(img1, unmatched_keypoints, output_image2, Scalar::all(-1), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

    Size sz1 = output_image1.size();
    Size sz2 = output_image2.size();
    Mat im3(sz1.height, sz1.width+sz2.width, CV_8UC3);
    Mat left(im3, Rect(0, 0, sz1.width, sz1.height));
    output_image1.copyTo(left);
    Mat right(im3, Rect(sz1.width, 0, sz2.width, sz2.height));
    output_image2.copyTo(right);
    imwrite(outpath, im3);
    
    waitKey(0);
    return EXIT_SUCCESS;
}
