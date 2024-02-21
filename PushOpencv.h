//
// Created by LT on 2024/2/21.
//

#ifndef RTSPTEST_PUSHOPENCV_H
#define RTSPTEST_PUSHOPENCV_H

#include <iostream>
#include <string>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <thread>
#include <opencv2/core/opengl.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/time.h>
}

class PushOpencv {
public:
    PushOpencv(std::string url);
    void start();
    void push_frame(cv::Mat &frame);
    int open_codec(int width, int height, int den);

private:
    int push();
    AVFrame *CVMatToAVFrame(cv::Mat &inMatV, int YUV_TYPE);
    cv::Mat pop_one_frame();

private:
    std::mutex queue_mutex;
    std::string url;
    std::queue<cv::Mat> pic_buffer;
    std::thread push_thread;
    std::condition_variable conditionVariable;

    AVCodecContext *outputVc;
    AVFormatContext *output;
    AVStream *vs;
};


#endif //RTSPTEST_PUSHOPENCV_H
