/*
As the base for this snippet, took pieces from the stackoverflow discussion:
https://stackoverflow.com/questions/22881878/how-can-i-draw-half-circle-in-opencv/22920595

Here's the direct link to ellipse():
https://docs.opencv.org/2.4/modules/core/doc/drawing_functions.html?highlight=rectangle#ellipse


The Python versions below, with and without rounded ends of the arc.
*/

#include <iostream>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

int main() {
cv::Mat outImage = cv::Mat(256,256,CV_8UC3, cv::Scalar(255,255,255));

cv::Point center(128,128);
int radius = 100;

//draw upright black halfcircle
cv::Scalar colorBlack = cv::Scalar(0,0,0);
double startAngleUpright = 180;
cv::ellipse(outImage,center,cv::Size(radius,radius),0,startAngleUpright,startAngleUpright+180,colorBlack,8,0);

//draw downright red halfcircle
cv::Scalar colorRed = cv::Scalar(0,0,255);
double startAngleDownright = 0;
cv::ellipse(outImage,center,cv::Size(radius,radius),0,startAngleDownright,startAngleDownright+180,colorRed,8,0);

cv::imshow("outImage", outImage);
cv::imwrite("tmp/DrawHalfCircle.png", outImage);

cv::waitKey(-1);
}

/*
Here are Python versions:

import cv2
import numpy as np

# Colors (B, G, R)
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)


def create_blank(width, height, color=(0, 0, 0)):
    """Create new image(numpy array) filled with certain color in BGR"""
    image = np.zeros((height, width, 3), np.uint8)
    # Fill image with color
    image[:] = color

    return image


def draw_half_circle_rounded(image):
    height, width = image.shape[0:2]
    # Ellipse parameters
    radius = 100
    center = (width / 2, height - 25)
    axes = (radius, radius)
    angle = 0
    startAngle = 180
    endAngle = 360
    thickness = 10

    # http://docs.opencv.org/modules/core/doc/drawing_functions.html#ellipse
    cv2.ellipse(image, center, axes, angle, startAngle, endAngle, BLACK, thickness)


# Create new blank 300x150 white image
width, height = 300, 150
image = create_blank(width, height, color=WHITE)
draw_half_circle_rounded(image)
cv2.imwrite('half_circle_rounded.jpg', image)

============================================
and another:

import cv2
import numpy as np

# Colors (B, G, R)
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)


def create_blank(width, height, color=(0, 0, 0)):
    """Create new image(numpy array) filled with certain color in BGR"""
    image = np.zeros((height, width, 3), np.uint8)
    # Fill image with color
    image[:] = color

    return image


def draw_half_circle_no_round(image):
    height, width = image.shape[0:2]
    # Ellipse parameters
    radius = 100
    center = (width / 2, height - 25)
    axes = (radius, radius)
    angle = 0
    startAngle = 180
    endAngle = 360
    # When thickness == -1 -> Fill shape
    thickness = -1

    # Draw black half circle
    cv2.ellipse(image, center, axes, angle, startAngle, endAngle, BLACK, thickness)

    axes = (radius - 10, radius - 10)
    # Draw a bit smaller white half circle
    cv2.ellipse(image, center, axes, angle, startAngle, endAngle, WHITE, thickness)


# Create new blank 300x150 white image
width, height = 300, 150

image = create_blank(width, height, color=WHITE)
draw_half_circle_no_round(image)
cv2.imwrite('half_circle_no_round.jpg', image)

*/
