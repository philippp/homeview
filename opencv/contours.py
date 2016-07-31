import matplotlib
matplotlib.use('pdf')

from matplotlib import pyplot as plt
import numpy as np
import cv2
 
im = cv2.imread('../captures/latest/video0.jpeg')
imgray = cv2.cvtColor(im,cv2.COLOR_BGR2GRAY)
ret,thresh = cv2.threshold(imgray,127,255,0)
im2, contours, hierarchy = cv2.findContours(thresh,cv2.RETR_TREE,cv2.CHAIN_APPROX_SIMPLE)
cv2.drawContours(im, contours, -1, (0,255,0), 3)

plt.imshow(im)
plt.title('Original Image')
plt.xticks([])
plt.yticks([])

plt.savefig('../captures/foo.png')
