
#ifndef __AV_HPP
#define __AV_HPP

#include "General.hpp"
#include "FF.hpp"
#include "video/display.hpp"
#include "audio/stream.hpp"

namespace AV
{
   class Scheduler : public General::Shared<Scheduler>
   {
      public:
         Scheduler(FF::MediaFile::Ptr in_file, AV::Audio::Stream<int16_t>::Ptr in_audio, AV::Video::Display::Ptr in_vid = AV::Video::Display::Ptr());
         void operator=(const Scheduler&) = delete;
         Scheduler(const Scheduler&) = delete;

         ~Scheduler();

         bool active() const;
         void run();
      private:
         FF::MediaFile::Ptr file;
         AV::Audio::Stream<int16_t>::Ptr audio;
         AV::Video::Display::Ptr video;
         bool has_video;
         bool has_audio;
         bool is_active;

         void process_video(AVPacket&);
         void process_audio(AVPacket&);
   };
}

#endif