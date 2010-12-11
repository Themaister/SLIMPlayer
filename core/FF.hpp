#ifndef __FF_HPP
#define __FF_HPP

#define __STDC_CONSTANT_MACROS
#include <stdint.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "General.hpp"

namespace FF
{
   class Packet
   {
      public:
         Packet();
         AVPacket& get();
         ~Packet();

         void operator=(const Packet&) = delete;
         Packet(const Packet&) = delete;

         Packet& operator=(Packet&&);
         Packet(Packet&&);

         enum class Type : unsigned
         {
            None = 0x0,
            Error = 0x1,
            Audio = 0x2,
            Video = 0x4,
         };

      private:
         AVPacket *pkt;
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
            bool active;
            AVCodecContext *ctx;
         };

         struct video_info
         {
            unsigned width;
            unsigned height;
            float aspect_ratio;
            bool active;
            AVCodecContext *ctx;
         };

         const audio_info& audio() const;
         const video_info& video() const;

         Packet::Type packet(Packet&);

      private:
         AVCodec *vcodec;
         AVCodec *acodec;
         AVCodecContext *actx;
         AVCodecContext *vctx;
         AVFormatContext *fctx;
         audio_info aud_info;
         video_info vid_info;
         int vid_stream;
         int aud_stream;

         void resolve_codecs();
         void set_media_info();
   };
}

#endif
