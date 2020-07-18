//
// g++ -std=c++1z  -ggdb video_cap.cpp `pkg-config --cflags --libs opencv4`
//

#include "opencv2/opencv.hpp"

//#include <opencv2/core/utility.hpp>
//#include <opencv2/imgproc.hpp>
//#include <opencv2/highgui.hpp>

#include <iostream>

using namespace std;
using namespace cv;
int main(){
  VideoCapture cap(0);
  if(!cap.isOpened()) {
    cout << "Error opening video stream" << endl;
    return -1;
  }
  int frame_width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
  int frame_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

  int mycodec = cv::VideoWriter::fourcc('M','J','P','G');
  VideoWriter video(
            "out.avi",
             mycodec, 10, Size(frame_width,frame_height));
  while(1) {
    Mat frame;
    cap >> frame;
    if (frame.empty())
      break;
    video.write(frame);
    imshow( "Frame", frame );

    char c = (char)waitKey(1);
    if( c == 27 )
      break;
  }
  cap.release();
  video.release();
  destroyAllWindows();
  return 0;
}

