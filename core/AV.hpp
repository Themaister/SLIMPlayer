
#ifndef __AV_HPP
#define __AV_HPP

#include "General.hpp"
#include "FF.hpp"
#include "video/display.hpp"
#include "audio/stream.hpp"
#include <queue>
#include <mutex>
#include <thread>
#include <stdint.h>

namespace AV
{
   class PacketQueue
   {
      public:
         void push(FF::Packet&& in);
         FF::Packet pull();
         size_t size() const;

      private:
         std::queue<FF::Packet> queue;
         mutable std::mutex lock;
   };


   class Scheduler : public General::Shared<Scheduler>
   {
      public:
         Scheduler(FF::MediaFile::Ptr in_file);
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
         AVFrame *frame;

         volatile bool threads_active;
         PacketQueue vid_pkt_queue;
         PacketQueue aud_pkt_queue;

         std::thread video_thread;
         std::thread audio_thread;

         void process_video(AVPacket&, AV::Video::Display::Ptr&);
         void process_audio(AVPacket&, AV::Audio::Stream<int16_t>::Ptr&);

         void video_thread_fn();
         void audio_thread_fn();
   };
}

#endif
