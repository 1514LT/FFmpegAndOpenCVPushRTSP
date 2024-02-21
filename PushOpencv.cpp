//
// Created by LT on 2024/2/21.
//
#include "PushOpencv.h"

AVFrame *PushOpencv::CVMatToAVFrame(cv::Mat &inMat, int YUV_TYPE)
{
  // 得到Mat信息
  AVPixelFormat dstFormat = AV_PIX_FMT_YUV420P;
  int width = inMat.cols;
  int height = inMat.rows;
  // 创建AVFrame填充参数 注：调用者释放该frame
  AVFrame *frame = av_frame_alloc();
  frame->width = width;
  frame->height = height;
  frame->format = dstFormat;

  // 初始化AVFrame内部空间
  int ret = av_frame_get_buffer(frame, 64);
  if (ret < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "av_frame_get_buffer faild\n");
    return nullptr;
  }
  ret = av_frame_make_writable(frame);
  if (ret < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "av_frame_make_writable faild\n");
    return nullptr;
  }

  // 转换颜色空间为YUV420
  cv::cvtColor(inMat, inMat, cv::COLOR_BGR2YUV_I420);

  // 按YUV420格式，设置数据地址
  int frame_size = width * height;
  unsigned char *data = inMat.data;

  memcpy(frame->data[0], data, frame_size);
  memcpy(frame->data[1], data + frame_size, frame_size / 4);
  memcpy(frame->data[2], data + frame_size * 5 / 4, frame_size / 4);

  return frame;
}

int PushOpencv::push()
{
  int ret = 0;
  cv::Mat frame;
  AVFrame *yuv;
  long pts = 0;
  AVPacket pack = {0};
  ret = avformat_write_header(output, NULL);
  av_log(NULL, AV_LOG_INFO, "avformat_write_header success\n");
  while (true)
  {
    frame = pop_one_frame();
    yuv = CVMatToAVFrame(frame, 0);

    yuv->pts = pts;
    pts += 1;
    av_log(NULL, AV_LOG_INFO, "bit_rate:%ld\n", outputVc->bit_rate);
    ret = avcodec_send_frame(outputVc, yuv);
    if (ret != 0)
    {
      av_log(NULL, AV_LOG_ERROR, "avcodec_send_frame erro\n");
      av_packet_unref(&pack);
      av_frame_free(&yuv);
      continue;
    }
    av_log(NULL, AV_LOG_INFO, "avcodec_send_frame success\n");
    while (avcodec_receive_packet(outputVc, &pack) == 0)
    {
      int firstFrame = 0;
      if (pack.dts < 0 || pack.pts < 0 || pack.dts > pack.pts || firstFrame)
      {
        firstFrame = 0;
        pack.dts = pack.pts = pack.duration = 0;
      }

      pack.pts = av_rescale_q(pack.pts, outputVc->time_base, vs->time_base);           // 显示时间
      pack.dts = av_rescale_q(pack.dts, outputVc->time_base, vs->time_base);           // 解码时间
      pack.duration = av_rescale_q(pack.duration, outputVc->time_base, vs->time_base); // 数据时长

      ret = av_interleaved_write_frame(output, &pack);

      if (ret < 0)
      {
        printf("发送数据包出错\n");
        av_frame_free(&yuv);
        av_packet_unref(&pack);
        continue;
      }
    }
    av_frame_free(&yuv);
    av_packet_unref(&pack);
  }

  return ret;
}

int PushOpencv::open_codec(int width, int height, int den)
{
  int ret = 0;
  avformat_network_init();
  const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!codec)
  {
    throw std::logic_error("Can`t find h264 encoder!"); // 找不到264编码器
  }
  // b 创建编码器上下文
  outputVc = avcodec_alloc_context3(codec);
  if (!outputVc)
  {
    throw std::logic_error("avcodec_alloc_context3 failed!"); // 创建编码器失败
  }
  // c 配置编码器参数
  outputVc->codec_type = AVMEDIA_TYPE_VIDEO;
  outputVc->pix_fmt = AV_PIX_FMT_YUV420P;
  outputVc->width = width;
  outputVc->height = height;
  outputVc->time_base = (AVRational){1, den};
  outputVc->bit_rate = 50 * 1024 * 80; // 压缩后每秒视频的bit位大小为500kb
  outputVc->max_b_frames = 0;
  outputVc->gop_size = 10;
#if 1
    outputVc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; // 全局参数
    outputVc->codec_id = codec->id;
    outputVc->thread_count = 8;
    outputVc->framerate = {den, 1};
    outputVc->qmax = 51;
    outputVc->qmin = 10;
#endif
  // d 打开编码器上下文
  ret = avcodec_open2(outputVc, codec, NULL);
  av_log(NULL, AV_LOG_INFO, "avcodec_open2 success\n");

  ret = avformat_alloc_output_context2(&output, NULL, "rtsp", url.c_str());

  vs = avformat_new_stream(output, outputVc->codec);
  vs->codecpar->codec_tag = 0;
  // 从编码器复制参数
  avcodec_parameters_from_context(vs->codecpar, outputVc);
  av_dump_format(output, 0, url.c_str(), 1);

  return ret;
}

cv::Mat PushOpencv::pop_one_frame()
{
  while (true)
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    conditionVariable.wait(lock, [this]()
                           { return pic_buffer.size() > 0; });
    cv::Mat tmp = pic_buffer.front().clone();
    pic_buffer.pop();
    return tmp;
  }
}

PushOpencv::PushOpencv(std::string url)
{
  this->url = url;
  outputVc = nullptr;
  output = nullptr;
}

void PushOpencv::push_frame(cv::Mat &frame)
{
  if (pic_buffer.size() < 256)
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    pic_buffer.push(frame);
    conditionVariable.notify_all();
  }
}

void PushOpencv::start()
{
  push_thread = std::thread(&PushOpencv::push, this);
  push_thread.detach();
}
