#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/logger.hpp>
#include "PushOpencv.h"

using namespace std;
using namespace cv;

int main()
{
  utils::logging::setLogLevel(utils::logging::LOG_LEVEL_SILENT);
  std::string url = "rtsp://192.168.5.165:8554/stream";
  VideoCapture cap(url);
  if (!cap.isOpened())
  {
    cout << "无法打开串流设备！" << endl;
    return -1;
  }

  PushOpencv *pushUtils = new PushOpencv("rtsp://192.168.5.165:8554/LTstream");
  pushUtils->open_codec(1280, 720, 30);
  pushUtils->start();

  namedWindow(url, WINDOW_AUTOSIZE);

  while (true)
  {
    Mat frame;
    bool bSuccess = cap.read(frame);

    pushUtils->push_frame(frame);

    if (!bSuccess)
    {
      cout << "" << endl;
      break;
    }

    imshow(url, frame);

    if (waitKey(30) == 'q')
    {
      break;
    }
  }

  cap.release();
  destroyAllWindows();
  return 0;
}
