#ifndef __FF_HPP
#define __FF_HPP

#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "General.hpp"

namespace FF
{
   class Packet
   {
      public:
         AVPacket* get();
         ~Packet();

      private:
         AVPacket pkt;
   };

   // A ref counted class that keeps track of FFmpeg deinit and init. :)
   class FFMPEG : public General::RefCounted<FFMPEG>
   {
      public:
         FFMPEG();
         ~FFMPEG();
   };

   class MediaFile : public FFMPEG, public General::Shared<MediaFile>
   {
      public:
         MediaFile(const char *path);
         MediaFile(MediaFile&&);
         MediaFile& operator=(MediaFile&&);
         void operator=(const MediaFile&) = delete;
         MediaFile(const MediaFile&) = delete;
         ~MediaFile();

         struct audio_info
         {
            unsigned channels;
            unsigned rate;
         };

         struct video_info
         {
            unsigned width;
            unsigned height;
            float aspect_ratio;
         };

         const audio_info& audio() const;
         const video_info& video() const;

         bool packet(Packet&);

      private:
         AVCodec *codec;
         AVCodecContext *ctx;
         AVFormatContext *fctx;
         audio_info aud_info;
         video_info vid_info;
   };
}

#endif
