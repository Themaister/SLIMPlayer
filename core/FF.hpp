#ifndef __FF_HPP
#define __FF_HPP

#include <stdint.h>

extern "C" {

// This stuff doesn't seem to be pulled in properly...

#ifndef __CONCAT
#define __CONCAT(value, suffix) value##suffix
#endif

#define INT8_C(value) ((int8_t) value) 
#define UINT8_C(value) ((uint8_t) __CONCAT(value, U)) 
#define INT16_C(value) value 
#define UINT16_C(value) __CONCAT(value, U) 
#define INT32_C(value) __CONCAT(value, L) 
#define UINT32_C(value) __CONCAT(value, UL) 
#define INT64_C(value) __CONCAT(value, LL) 
#define UINT64_C(value) __CONCAT(value, ULL) 
#define INTMAX_C(value) __CONCAT(value, LL) 
#define UINTMAX_C(value) __CONCAT(value, ULL)

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

}

#include "General.hpp"

namespace FF
{
   void set_global_pts(uint64_t);

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
         void seek(double pts, double relative);

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
