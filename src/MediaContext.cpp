
#include "MediaContext.h"
#include "phash.h"
#include <iostream>

static void print_error(const char *msg, int err) {
  char buffer[AV_ERROR_MAX_STRING_SIZE]{0};

  if(err != 0) {
    if(av_strerror(err, buffer, AV_ERROR_MAX_STRING_SIZE) < 0) {
#if __APPLE__
      strerror_r(AV_ERROR_MAX_STRING_SIZE, buffer, AVUNERROR(err));
#else
      strerror_s(buffer, AV_ERROR_MAX_STRING_SIZE, AVUNERROR(err));
#endif
    }
  }

  std::cout << msg << ": " << buffer << std::endl;

  //LOG_ERROR(msg, errbuf_ptr);
}

void MediaContext::Initialize() {
  av_log_set_level(AV_LOG_QUIET);
  av_register_all();
}

MediaContextPtr MediaContext::Create(const char* filename) {
  Initialize();
  auto context = std::make_shared<MediaContext>(filename);
  if(context->Open() == true)
    return context;

  return nullptr;
}

bool MediaContext::IsStreamType(StreamType type, AVMediaType n) {
  auto result = false;
  if(type == StreamType::Video) {
    result = n == AVMEDIA_TYPE_VIDEO;
  } else if(type == StreamType::Audio) {
    result = n == AVMEDIA_TYPE_AUDIO;
  } else if(type == StreamType::Data) {
    result = n == AVMEDIA_TYPE_DATA;
  } else if(type == StreamType::Unknown) {
    result = n == AVMEDIA_TYPE_UNKNOWN;
  } else if(type == StreamType::Subtitle) {
    result = n == AVMEDIA_TYPE_SUBTITLE;
  } else if(type == StreamType::NB) {
    result = n == AVMEDIA_TYPE_NB;
  }

  return result;
}

MediaContext::MediaContext(const char* filename)
  : m_pFormatCtx(nullptr)
  , m_pStream(nullptr)
  , m_pCodec(nullptr)
  , m_pFrameSrc(nullptr)
  , m_pFrameDst(nullptr)
  , m_pCodecCtx(nullptr)
  , m_pSwsContext(nullptr)
  , m_pBuffer(nullptr)
  , m_pixelFormat(AVPixelFormat::AV_PIX_FMT_GRAY8)
  , m_pts(0)
  , m_idxFrameCur(0)
  , m_idxFrameNext(0)
  , m_idxFrameStep(0)
  , m_numFrames(0)
  , m_numFrameRetrieve(0)
  , m_idxStream(-1)
  , m_width(0)
  , m_height(0)
  , m_fps(0)
  , m_isOpen(false)
  , m_isFrameFinished(false)
  , m_filename(filename) {}

MediaContext::~MediaContext() {
  Close();
}

int MediaContext::GetNumberVideoFrames() const {
  int64_t result;

  if(m_pStream->nb_frames > 0) {
    result = m_pStream->nb_frames;
  } else {
    result = static_cast<int64_t>(av_index_search_timestamp(m_pStream, m_pStream->duration, AVSEEK_FLAG_ANY | AVSEEK_FLAG_BACKWARD));
    if(result <= 0) {
      if (m_pStream->duration != AV_NOPTS_VALUE) {
        int timebase = m_pStream->time_base.den / m_pStream->time_base.num;
        result = m_pStream->duration / timebase;
      } else {
        auto duration = m_pFormatCtx->duration + 5000;
        auto secs = static_cast<double>(duration) / AV_TIME_BASE;
        result = av_q2d(m_pStream->avg_frame_rate) * secs;
      }
    }
  }
  
  return static_cast<int>(result);
}

bool MediaContext::Open() {
  if (m_isOpen) {
    return true;
  }

  int err;
  if((err = avformat_open_input(&m_pFormatCtx, m_filename, nullptr, nullptr)) < 0) {
    print_error("Failed to open input media: ", err);
  } else if((err = avformat_find_stream_info(m_pFormatCtx, nullptr)) < 0) {
    print_error("Failed to find stream info: ", err);
  }

  m_isOpen = err >= 0;

  return m_isOpen;
}

bool MediaContext::IsOpen() const { return m_isOpen; }

bool MediaContext::OpenStream(StreamType type, int index) {
  if(!IsOpen()) {
    // Open context if not already
    if (!Open())
      return false;
  } else {
    // Close current open stream if there is one
    CloseStream();
  }

  // Find index of stream
  auto count = 0;
  for(unsigned int i = 0; i < m_pFormatCtx->nb_streams; i++) {
    auto codec = m_pFormatCtx->streams[i]->codec;
    if(IsStreamType(type, codec->codec_type)) {
      if(count != index) {
        count++;
      } else {
        m_idxStream = i;
        break;
      }
    }
  }

  if(m_idxStream == -1) {
    return false;
  }

  m_pStream = m_pFormatCtx->streams[m_idxStream];
  m_pCodec = avcodec_find_decoder(m_pStream->codec->codec_id);
  if(m_pCodec == nullptr) {
    //LOG_ERROR("Unsupported codec: ", avcodec_get_name(m_pStream->codec->codec_id));
  } else {
    m_pCodecCtx = avcodec_alloc_context3(m_pCodec);

    // Copy the codec context and open the codec
    if(m_pCodecCtx == nullptr) {
      //LOG_ERROR("Failed to allocate codec context");
      m_pCodec = nullptr;
    } else {
      int err;
      if((err = avcodec_copy_context(m_pCodecCtx, m_pStream->codec)) < 0) {
        print_error("Failed to copy codec context: ", err);
        m_pCodec = nullptr;
      } else if(avcodec_open2(m_pCodecCtx, m_pCodec, nullptr) < 0) {
        //LOG_ERROR("Failed to open codec");
        m_pCodec = nullptr;
      }

      if(m_pCodec != nullptr) {
        auto num = (m_pStream->r_frame_rate).num;
        auto den = (m_pStream->r_frame_rate).den;
        m_fps = static_cast<float>(num / den);
        m_width = m_pCodecCtx->width;
        m_height = m_pCodecCtx->height;

        auto fps = static_cast<float>(0.5f * m_fps);
        m_idxFrameStep = static_cast<int>(fps + ROUNDING_FACTOR(fps));

        auto frames = GetNumberVideoFrames();
        m_numFrames = (frames / m_idxFrameStep);

        if (!AllocateBuffers()) {
          m_pCodec = nullptr;
          m_fps = 0;
          m_numFrames = 0;
          m_idxFrameStep = 0;
          m_width = 0;
          m_height = 0;
        }
      }

      // If a failure occurred, free the allocated context
      if(m_pCodec == nullptr) {
        avcodec_free_context(&m_pCodecCtx);
        m_pCodecCtx = nullptr;
      }
    }
  }

  if(m_pCodec == nullptr) {
    m_pStream = nullptr;
    m_idxStream = -1;
    return false;
  }

  return true;
}

bool MediaContext::IsStreamOpen() const {
  return (m_pStream != nullptr && m_idxStream != -1);
}

bool MediaContext::IsStreamIndex(int index) const {
  return index == m_idxStream;
}

bool MediaContext::CloseStream() {
  FreeBuffers();

  if(m_pCodecCtx != nullptr) {
    avcodec_close(m_pCodecCtx);
    avcodec_free_context(&m_pCodecCtx);
  }

  m_pStream = nullptr;
  m_pCodecCtx = nullptr;
  m_pCodec = nullptr;
  m_idxStream = -1;
  m_numFrames = 0;
  m_fps = 0;
  m_width = 0;
  m_height = 0;

  return true;
}

bool MediaContext::Close() {
  if (!m_isOpen) {
    return true;
  }

  CloseStream();

  if(m_pFormatCtx != nullptr) {
    avformat_close_input(&m_pFormatCtx);
    m_pFormatCtx = nullptr;
  }

  m_isOpen = false;

  return true;
}

bool MediaContext::SetDimensions(int width, int height) {
  FreeBuffers();

  m_width = width;
  m_height = height;

  return AllocateBuffers();
}

bool MediaContext::AllocateBuffers() {
  m_pFrameDst = nullptr;
  m_pBuffer = nullptr;
  m_pSwsContext = nullptr;

  m_pFrameSrc = av_frame_alloc();
  if(m_pFrameSrc == nullptr) {
    return false;
  }

  bool success;
  m_pFrameDst = av_frame_alloc();
  if(m_pFrameDst == nullptr) {
    success = false;
  } else {
    // Determine required buffer size and allocate buffer
    auto numBytes = avpicture_get_size(m_pixelFormat, m_width, m_height);
    m_pBuffer = static_cast<uint8_t *>(av_malloc(numBytes * sizeof(uint8_t)));
    if(m_pBuffer == nullptr) {
      success = false;
    } else {
      avpicture_fill(reinterpret_cast<AVPicture *>(m_pFrameDst), m_pBuffer, m_pixelFormat, m_width, m_height);
      m_pSwsContext = sws_getContext(m_pCodecCtx->width, m_pCodecCtx->height, m_pCodecCtx->pix_fmt, m_width, m_height, m_pixelFormat, SWS_BICUBIC, nullptr, nullptr, nullptr);
      success = m_pSwsContext != nullptr;
    }
  }

  if(!success) {
    FreeBuffers();
  }

  return success;
}

bool MediaContext::DecodeVideoFrame(const AVPacket& packet) {
  AVPacket avpkt;

  // HACK for CorePNG to decode as normal PNG by default same method used by ffmpeg
  av_init_packet(&avpkt);
  avpkt.flags = AV_PKT_FLAG_KEY;
  avpkt.data = packet.data;
  avpkt.size = packet.size;

  int finished;
  if(avcodec_decode_video2(m_pCodecCtx, m_pFrameSrc, &finished, &avpkt) < 0)
    return false;

  m_isFrameFinished = finished;
  if (m_isFrameFinished)
    m_idxFrameCur++;

  return true;
}

bool MediaContext::IsDecodeVideoFinished() const {
  return m_isFrameFinished;
}

bool MediaContext::IsNextFrameIndex() const {
  return (m_idxFrameCur - 1) == m_idxFrameNext;
}

void MediaContext::StepNextFrameIndex() {
  m_idxFrameNext += m_idxFrameStep;
}

bool MediaContext::Reset() {
  if (!m_isOpen) {
    return false;
  }

  int64_t target = av_rescale_q(0, AV_TIME_BASE_Q, m_pStream->time_base);
  if(av_seek_frame(m_pFormatCtx, m_idxStream, target, AVSEEK_FLAG_ANY) < 0) {
    return false;
  }

  avcodec_flush_buffers(m_pCodecCtx);
  m_idxFrameCur = 0;
  m_idxFrameNext = 0;
  m_isFrameFinished = false;

  return true;
}

bool MediaContext::ReadCurrentFrame(CImageI& image) const {
  if(!m_isFrameFinished)
    return false;

  sws_scale(m_pSwsContext, m_pFrameSrc->data, m_pFrameSrc->linesize, 0, m_pCodecCtx->height, m_pFrameDst->data, m_pFrameDst->linesize);

  auto channels = (m_pixelFormat == AV_PIX_FMT_GRAY8) ? 1 : 3;
  image.assign(*m_pFrameDst->data, channels, m_width, m_height, 1, true);

  return true;
}

bool MediaContext::FreeBuffers() {
  if(m_pFrameSrc != nullptr)
    av_frame_free(&m_pFrameSrc);
  if(m_pFrameDst != nullptr)
    av_frame_free(&m_pFrameDst);
  if(m_pBuffer != nullptr)
    av_free(m_pBuffer);
  if(m_pSwsContext != nullptr)
    sws_freeContext(m_pSwsContext);

  m_pFrameSrc = nullptr;
  m_pFrameDst = nullptr;
  m_pBuffer = nullptr;
  m_pSwsContext = nullptr;

  return true;
}
