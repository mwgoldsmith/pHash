#ifndef MEDIACONTEXT_H
#define MEDIACONTEXT_H

#include <cstdint>
#include <memory>

#include "internal.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/log.h>
#include <libavutil/avutil.h>
}

enum class StreamType {
  Unknown = -1,
  Video,
  Audio,
  Data,
  Subtitle,
  Attachment,
  NB
};

class MediaContext;
using MediaContextPtr = std::shared_ptr<MediaContext>;

class MediaContext {
public:
  explicit MediaContext(const char *filename);
  ~MediaContext();

  int GetNumFrames() const { return m_numFrames; }
  int GetNumRetrieveFrames() const { return m_numFrameRetrieve; }
  uint64_t GetIndexCurFrame() const { return m_idxFrameCur; }
  int GetFrameStep() const { return m_idxFrameStep; }
  AVFormatContext* GetFormatContext() const { return m_pFormatCtx; }
  AVCodecContext* GetCodecContext() const { return m_pCodecCtx; }
  AVFrame* GetSourceFrame() const { return m_pFrameSrc; }
  AVFrame* GetDestFrame() const { return m_pFrameDst; }
  void SetIndexNextFrame(uint64_t value) { m_idxFrameNext = value; }
  void SetNumRetrieveFrames(int value) { m_numFrameRetrieve = value; }

  bool Open();
  bool IsOpen() const;
  bool Close();
  bool SetDimensions(int width, int height);
  bool OpenStream(StreamType type, int index);
  bool Reset();
  bool IsStreamOpen() const;
  bool IsStreamIndex(int index) const;
  bool CloseStream();

  bool AllocateBuffers();
  bool DecodeVideoFrame(const AVPacket& packet);
  bool IsDecodeVideoFinished() const;
  bool IsNextFrameIndex() const;
  void StepNextFrameIndex();
  bool ReadCurrentFrame(CImageI& image) const;
  bool FreeBuffers();

private:
  int GetNumberVideoFrames() const;

public:
  static void Initialize();
  static MediaContextPtr Create(const char* filename);
  static bool IsStreamType(StreamType type, AVMediaType n);

private:
  AVFormatContext* m_pFormatCtx;
  AVStream*        m_pStream;
  AVCodec*         m_pCodec;
  AVFrame*         m_pFrameSrc;
  AVFrame*         m_pFrameDst;
  AVCodecContext*  m_pCodecCtx;
  SwsContext*      m_pSwsContext;
  uint8_t*         m_pBuffer;
  AVPixelFormat    m_pixelFormat;
  int64_t          m_pts;
  uint64_t         m_idxFrameCur;
  uint64_t         m_idxFrameNext;
  int              m_idxFrameStep;
  int              m_numFrames;
  int              m_numFrameRetrieve;
  int              m_idxStream;
  int              m_width;
  int              m_height;
  float            m_fps;
  bool             m_isOpen;
  bool             m_isFrameFinished;
  const char*      m_filename;
};

#endif /* MEDIACONTEXT_H */
