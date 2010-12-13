
#ifndef __AV_HPP
#define __AV_HPP

#include "General.hpp"
#include "FF.hpp"
#include "video/display.hpp"
#include "audio/stream.hpp"
#include <queue>
#include <list>
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
         void clear();

      private:
         std::queue<FF::Packet> queue;
         mutable std::mutex lock;
   };

   class EventHandler : public General::SharedAbstract<EventHandler>
   {
      public:
         enum class Event : unsigned
         {
            Pause,
            Quit,
            Seek,
            None,
         };

         virtual Event event() = 0;
         virtual void poll() = 0;

         virtual ~EventHandler() {}
   };

   class Scheduler : public General::Shared<Scheduler>
   {
      public:
         Scheduler(FF::MediaFile::Ptr in_file);
         void operator=(const Scheduler&) = delete;
         Scheduler(const Scheduler&) = delete;

         ~Scheduler();

         void add_event_handler(EventHandler::APtr& ptr);

         bool active() const;
         void run();

      private:
         FF::MediaFile::Ptr file;
         bool has_video;
         bool has_audio;
         bool is_active;
         AVFrame *frame;
         double video_pts;
         double audio_pts;
         double audio_pts_ts;
         double video_pts_ts;
         size_t audio_written;
         volatile bool is_paused;
         std::mutex avlock;

         std::list<EventHandler::APtr> event_handlers;
         EventHandler::Event next_event();

         volatile bool threads_active;
         PacketQueue vid_pkt_queue;
         PacketQueue aud_pkt_queue;

         std::thread video_thread;
         std::thread audio_thread;

         void process_video(AVPacket&, AV::Video::Display::APtr&&);
         void process_audio(AVPacket&, AV::Audio::Stream<int16_t>::APtr&&);
         void pause_toggle();

         void video_thread_fn();
         void audio_thread_fn();

         static void sync_sleep(float time);
         static double get_time();
         double time_base() const;
   };
}

#endif
