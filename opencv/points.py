import matplotlib
matplotlib.use('pdf')

import glob
from matplotlib import pyplot as plt
import cv2
import numpy as np


def show_single_image(filename):
    img = cv2.imread('../captures/latest/video0.jpeg',0)

    plt.subplot(221)
    plt.imshow(img, cmap = 'gray')
    plt.title('Original Image')
    plt.xticks([])
    plt.yticks([])

    edges = cv2.Canny(img,0,150)
    plt.subplot(222)
    plt.imshow(edges, cmap = 'gray')
    plt.title('Edge Image')
    plt.xticks([])
    plt.yticks([])

    surf = cv2.xfeatures2d.SURF_create(1000)
    kp, des = surf.detectAndCompute(img,None)
    img2 = cv2.drawKeypoints(img,kp,None,(255,0,0),4)
    plt.subplot(223)
    plt.imshow(img2)
    plt.title('SURF Keypoints')
    plt.xticks([])
    plt.yticks([])

    plt.savefig('../captures/foo.png')

def rank_differences(filenames):
    surf = cv2.xfeatures2d.SURF_create(1000)
    matcher = cv2.DescriptorMatcher_create("BruteForce-Hamming")
    description_a = None
    key_points_a = None
    description_b = None
    key_points_b = None

    # FLANN parameters
    FLANN_INDEX_KDTREE = 0
    index_params = dict(algorithm = FLANN_INDEX_KDTREE, trees = 5)
    search_params = dict(checks=50)   # or pass empty dictionary

    flann = cv2.FlannBasedMatcher(index_params,search_params)

    for filename in filenames:
        description_b = description_a
        key_points_b = key_points_a
        img = cv2.imread(filename)
        key_points_a, description_a = surf.detectAndCompute(img,None)
        if description_b is None:
            continue  # First iteration.
        
        matches = flann.knnMatch(description_a, description_b, k=2)
        # visualize the matches
        print '#matches:', len(matches)
        dist = [m.distance for m in matches]
        
        print 'distance: min: %.3f' % min(dist)
        print 'distance: mean: %.3f' % (sum(dist) / len(dist))
        print 'distance: max: %.3f' % max(dist)

if __name__ == "__main__":

    files = glob.glob("../captures/*/*/*/*/video0.jpeg")
    files.sort()
    rank_differences(files)
    
    # http://stackoverflow.com/questions/11114349/how-to-visualize-descriptor-matching-using-opencv-module-in-python
