
#include "VideoProcessor.h"
#include <iostream>

int ReadFrames(MediaContextPtr pContext, CImageIListPtr pFrameList, uint64_t idxLow, uint64_t idxHigh) {
  pContext->SetIndexNextFrame(idxLow);

  AVPacket packet;
  CImageI image;
  auto frameNo = 0;
  auto success = true;

  while((frameNo < pContext->GetNumRetrieveFrames()) && (idxHigh > pContext->GetIndexCurFrame())) {
    if(av_read_frame(pContext->GetFormatContext(), &packet) < 0)
      break;

    if(pContext->IsStreamIndex(packet.stream_index)) {
      success = pContext->DecodeVideoFrame(packet);

      if(success && pContext->IsDecodeVideoFinished() && pContext->IsNextFrameIndex()) {
        pContext->StepNextFrameIndex();
        pContext->ReadCurrentFrame(image);
        image.permute_axes("yzcx");
        pFrameList->push_back(image);

        frameNo++;
      }
    }

    av_free_packet(&packet);
  }

  return success ? frameNo : -1;
}

int VideoProcessor::ReadFrames(CImageIListPtr pFrameList) const {
  AVPacket packet;
  CImageI image;
  auto frameNo = 0;
  auto success = true;

  while(success && frameNo < m_pContext->GetNumRetrieveFrames()) {
    if(av_read_frame(m_pContext->GetFormatContext(), &packet) < 0)
      break;

    if(m_pContext->IsStreamIndex(packet.stream_index)) {
      success = m_pContext->DecodeVideoFrame(packet);
      if(success && m_pContext->IsDecodeVideoFinished() && m_pContext->IsNextFrameIndex()) {
        m_pContext->StepNextFrameIndex();
        m_pContext->ReadCurrentFrame(image);
        image.permute_axes("yzcx");
        pFrameList->push_back(image);
        frameNo++;
      }
    }

    av_free_packet(&packet);
  }

  return success ? frameNo : -1;
}

bool VideoProcessor::CalculateFrameDifferential(float* pFrameDist) const {
  auto numTotalFrames = m_pContext->GetNumFrames();
  auto idxCurFrame = 0;
  auto pFrameList = std::make_shared<CImageIList>();

  CImageF prev(64, 1, 1, 1, 0);
  int numFrames;

  m_pContext->SetNumRetrieveFrames(100);

  // Calculate threshold difference between each successive frame
  do {
    numFrames = ReadFrames(pFrameList);
    if(numFrames < 0) {
      return false;
    }

    unsigned int i = 0;
    while((i < pFrameList->size()) && (idxCurFrame < numTotalFrames)) {
      auto current = pFrameList->at(i++);
      auto hist = current.get_histogram(64, 0, 255);

      pFrameDist[idxCurFrame] = 0.0;
      cimg_forX(hist, X) {
        auto d = hist(X) - prev(X);
        d = (d >= 0) ? d : -d;
        pFrameDist[idxCurFrame] += d;
        prev(X) = static_cast<float>(hist(X));
      }

      idxCurFrame++;
    }

    pFrameList->clear();
  } while((numFrames >= m_pContext->GetNumRetrieveFrames()) && (idxCurFrame < numTotalFrames));

  return true;
}

std::vector<uint64_t> VideoProcessor::CalculateBoundryFrames(float* pFrameDiff) const {
  auto numFrames = m_pContext->GetNumFrames();
  auto pBounds = new uint8_t[numFrames];
  const auto S = 10;
  const auto L = 50;
  auto alpha1 = 3;
  auto alpha2 = 2;
  int s_begin, s_end;
  int l_begin, l_end;
  auto idxCurFrame = 1;

  pBounds[0] = 1;

  do {
    l_begin = (idxCurFrame - L >= 0) ? idxCurFrame - L : 0;
    l_end = (idxCurFrame + L < numFrames) ? idxCurFrame + L : numFrames - 1;

    /* get global average */
    float ave_global;
    float sum_global = 0.0;
    float dev_global = 0.0;
    for(auto i = l_begin; i <= l_end; i++) {
      sum_global += pFrameDiff[i];
    }

    ave_global = sum_global / static_cast<float>(l_end - l_begin + 1);

    /*get global deviation */
    for(auto i = l_begin; i <= l_end; i++) {
      auto dev = ave_global - pFrameDiff[i];
      dev = (dev >= 0) ? dev : -1 * dev;
      dev_global += dev;
    }

    dev_global = dev_global / static_cast<float>(l_end - l_begin + 1);

    s_begin = (idxCurFrame - S >= 0) ? idxCurFrame - S : 0;
    s_end = (idxCurFrame + S < numFrames) ? idxCurFrame + S : numFrames - 1;

    /* get local maximum */
    auto localmaxpos = s_begin;
    for(auto i = s_begin; i <= s_end; i++) {
      if(pFrameDiff[i] > pFrameDiff[localmaxpos])
        localmaxpos = i;
    }

    /* get 2nd local maximum */
    auto localmaxpos2 = s_begin;
    auto localmax2 = 0.0f;
    for(auto i = s_begin; i <= s_end; i++) {
      if(i == localmaxpos)
        continue;
      if(pFrameDiff[i] > localmax2) {
        localmaxpos2 = i;
        localmax2 = pFrameDiff[i];
      }
    }

    auto t_global = ave_global + alpha1*dev_global;
    auto t_local = alpha2*pBounds[localmaxpos2];
    auto thresh = (t_global >= t_local) ? t_global : t_local;
    if((pBounds[idxCurFrame] == pBounds[localmaxpos]) && (pBounds[idxCurFrame] > thresh)) {
      pBounds[idxCurFrame] = 1;
    } else {
      pBounds[idxCurFrame] = 0;
    }

    idxCurFrame++;
  } while(idxCurFrame < numFrames - 1);

  pBounds[numFrames - 1] = 1;

  auto start = 0;
  auto end = 0;
  auto step = m_pContext->GetFrameStep();
  std::vector<uint64_t> results;

  do {
    /* find next boundary */
    do {
      end++;
    } while((pBounds[end] != 1) && (end < numFrames));

    /* find min disparity within bounds */
    auto minpos = start + 1;
    for(auto i = start + 1; i < end; i++) {
      if(pFrameDiff[i] < pFrameDiff[minpos])
        minpos = i;
    }

    results.push_back(minpos*step);

    start = end;
  } while(start < numFrames - 1);

  delete[] pBounds;

  return results;
}

VideoProcessor::VideoProcessor(MediaContextPtr pContext)
 : m_pContext { pContext } {
}

CImageIListPtr VideoProcessor::GetSceneChangeFrames(const char* filename) {
  auto pContext = MediaContext::Create(filename);
  if(pContext == nullptr)
    return nullptr;
  if(!pContext->Open()) 
    return nullptr;
  if(!pContext->OpenStream(StreamType::Video, 0))
    return nullptr;

  auto numFrames = pContext->GetNumFrames();
  auto pFrameDist = new float[numFrames];
  auto pFrameList = std::make_shared<CImageIList>();
  auto success = true;

  VideoProcessor processor(pContext);
  if (!processor.CalculateFrameDifferential(pFrameDist)) {
    success = false;
  } else {
    auto boundries = processor.CalculateBoundryFrames(pFrameDist);

    pContext->SetNumRetrieveFrames(1);
    if (!pContext->SetDimensions(32, 32)) {
      success = false;
    } else {
      pContext->Reset();
      for(auto& index : boundries) {
        pContext->SetIndexNextFrame(index);
        if(processor.ReadFrames(pFrameList) < 0) {
          success = false;
          break;
        }
      }
    }
  }

  delete[] pFrameDist;
    
  return success ? pFrameList : nullptr;
}

