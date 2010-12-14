// Seem to be some libstdc++ issue for now.
#define _GLIBCXX_USE_NANOSLEEP

#include "FF.hpp"
#include "AV.hpp"
#include "audio/rsound.hpp"
#include "video/opengl.hpp"
#include <iostream>
#include <array>
#include <memory>
#include <thread>
#include <chrono>
#include <algorithm>

using namespace FF;
using namespace AV::Audio;
using namespace AV::Video;

namespace AV
{
   Scheduler::Scheduler(MediaFile::Ptr in_file) : file(in_file), is_active(true), video_pts(0.0), audio_pts(0.0), audio_pts_ts(get_time()), video_pts_ts(get_time()), audio_written(0), is_paused(false)
   {
      has_video = file->video().active;
      has_audio = file->audio().active;

      if (has_video)
         frame = avcodec_alloc_frame();

      threads_active = true;

      if (has_video)
         video_thread = std::thread(&Scheduler::video_thread_fn, this);
      if (has_audio)
         audio_thread = std::thread(&Scheduler::audio_thread_fn, this);
   }

   Scheduler::~Scheduler()
   {
      threads_active = false;

      if (has_video)
         video_thread.join();
      if (has_audio)
         audio_thread.join();

      if (has_video)
         av_free(frame);
   }

   bool Scheduler::active() const
   {
      return is_active;
   }

   EventHandler::Event Scheduler::next_event()
   {
      auto event = EventHandler::Event::None;

      std::for_each(event_handlers.begin(), event_handlers.end(), 
            [&event](EventHandler::APtr& handler) 
            {
               //handler->poll();
               auto evnt = handler->event();

               if (event == EventHandler::Event::None)
                  event = evnt;
            });
      return event;
   }

   void Scheduler::pause_toggle()
   {
      avlock.lock();
      is_paused = !is_paused;
      avlock.unlock();
   }

   void Scheduler::run()
   {
      avlock.lock();
      auto event = next_event();
      avlock.unlock();

      switch (event)
      {
         case EventHandler::Event::Quit:
            std::cerr << "Quitting!!!" << std::endl;
            is_active = false;
            break;

         case EventHandler::Event::Pause:
            std::cerr << "Pause toggling stream!!!" << std::endl;
            pause_toggle();
            break;

         case EventHandler::Event::SeekBack10:
            std::cerr << "Seeking backwards!!!" << std::endl;
            avlock.lock();
            aud_pkt_queue.clear();
            vid_pkt_queue.clear();
            is_paused = true;

            file->seek(audio_pts, -10.0);
            video_pts -= 10.0;
            audio_pts -= 10.0;
            avlock.unlock();
            is_paused = false;
            break;

         case EventHandler::Event::SeekForward10:
            std::cerr << "Seeking forward!!!" << std::endl;
            avlock.lock();
            aud_pkt_queue.clear();
            vid_pkt_queue.clear();
            is_paused = true;

            file->seek(audio_pts, 10.0);
            video_pts += 10.0;
            audio_pts += 10.0;
            avlock.unlock();
            is_paused = false;
            break;

         default:
            break;
      }

      if (is_paused)
      {
         sync_sleep(0.01);
         return;
      }

      Packet pkt;

      Packet::Type type = file->packet(pkt);

      switch(type)
      {
         case Packet::Type::Error:
            is_active = false;
            return;
            
         case Packet::Type::None:
            return;

         case Packet::Type::Audio:
            while (aud_pkt_queue.size() > 16)
               sync_sleep(0.01);

            aud_pkt_queue.push(std::move(pkt));
            break;

         case Packet::Type::Video:
            while (vid_pkt_queue.size() > 16)
               sync_sleep(0.01);

            vid_pkt_queue.push(std::move(pkt));
            break;

         default:
            throw std::runtime_error("What kind of package is this? o.o\n");
      }
   }

   void Scheduler::sync_sleep(float secs)
   {
      std::this_thread::sleep_for(std::chrono::nanoseconds((int64_t)(secs * 1000000000)));
   }

   double Scheduler::get_time()
   {
      auto clock = std::chrono::monotonic_clock::now();
      std::chrono::nanoseconds time = clock.time_since_epoch();
      double secs = time.count() * 0.000000001;
      return secs;
   }

   double Scheduler::time_base() const
   {
      float timebase = file->video().ctx->ticks_per_frame * av_q2d(file->video().ctx->time_base);
      // FFmpeg hack :(
      if (file->video().ctx->time_base.den > 1000 && file->video().ctx->time_base.num == 1)
         timebase *= 1000.0;

      return timebase;
   }

   void Scheduler::process_video(AVPacket& pkt, Display::APtr&& vid)
   {
      if (!has_video)
         return;

      int finished = 0;

      uint64_t pts = pkt.pts;
      FF::set_global_pts(pkt.pts);

      avcodec_decode_video2(file->video().ctx, frame, &finished, &pkt);

      if (pkt.dts == (int64_t)AV_NOPTS_VALUE && frame->opaque && *(uint64_t*)frame->opaque != AV_NOPTS_VALUE)
      {
         pts = *(uint64_t*)frame->opaque;
      }
      else if (pkt.dts != (int64_t)AV_NOPTS_VALUE)
      {
         pts = pkt.dts;
      }

      // We're not using this value atm... It doesn't seem quite right.
      pts *= time_base();
      // This PTS value seems to be bogus when you compare it to audio PTS!
      // PTS sometimes doesn't increase either after one finished iteration (?!?!).

      // If we got a finished frame, show it to the screen. :)
      // Always seems to be finished for some strange reason.
      if (finished)
      {
         // I really doubt this will work with variable FPS, but hey. This approach seems to work well for the videos I've tested so far.
         video_pts += time_base();
         video_pts += frame->repeat_pict / (2.0 * time_base());

         vid->frame(frame->data, frame->linesize, file->video().width, file->video().height);

         // We have to calculate how long we should wait before swapping frame to screen.
         // We sync everything to audio clock.
         double delta = get_time();

         avlock.lock();
         delta -= audio_pts_ts;
         double sleep_time = video_pts - (audio_pts + delta);
         avlock.unlock();

         if (video_pts > (audio_pts + delta))
         {
            double last_frame_delta = get_time();
            last_frame_delta -= video_pts_ts;

            // We try to keep the sleep time to a somewhat small value to avoid choppy video in some cases.
            // Max sleep time should be a bit over 1 frame time to allow audio to catch up.
            double max_sleep = 1.5 * time_base() - last_frame_delta;

            if (max_sleep < 0.0)
               max_sleep = 0.0;

            if (sleep_time > max_sleep)
            {
               sleep_time = max_sleep;
            }
            sync_sleep(sleep_time);
         }

         video_pts_ts = get_time();
         vid->flip();
      }
   }

   void Scheduler::process_audio(AVPacket& pkt, Stream<int16_t>::APtr&& aud)
   {
      if (!has_audio)
         return;

      uint8_t *pkt_data = pkt.data;
      size_t pkt_size = pkt.size;

      // AVCODEC_MAX_AUDIO_FRAME_SIZE / 2 would do, but FFmpeg needs some extra padding-stuff, so why not...
      std::array<int16_t, AVCODEC_MAX_AUDIO_FRAME_SIZE> buf;
      while (pkt.size > 0)
      {
         int out_size = AVCODEC_MAX_AUDIO_FRAME_SIZE * 2;
         int ret = avcodec_decode_audio3(file->audio().ctx, &buf[0], &out_size, &pkt);
         if (ret <= 0)
            break;

         pkt.size -= ret;
         pkt.data += ret;

         if (out_size <= 0)
            continue;

         aud->write(&buf[0], out_size / 2);
         audio_written += out_size;

         // Update Audio timestamp. Doesn't use the actual audio pts, but I really doubt we'll need that. PCM is PCM after all :)
         avlock.lock();
         audio_pts = (float)audio_written/(file->audio().rate * file->audio().channels * 2) - aud->delay();
         audio_pts_ts = get_time();
         avlock.unlock();
      }

      pkt.data = pkt_data;
      pkt.size = pkt_size;
   }

   // Video thread
   void Scheduler::video_thread_fn()
   {
      auto vid = GL::shared(file->video().width, file->video().height, file->video().aspect_ratio);
      auto event = GLEvent::shared();

      // Add event handler for GL.
      avlock.lock();
      event_handlers.push_back(event);
      avlock.unlock();

      while (threads_active)
      {
         if (vid_pkt_queue.size() > 0 && !is_paused)
         {
            Packet pkt = vid_pkt_queue.pull();
            process_video(pkt.get(), vid);
         }
         else
         {
            event->poll(); 
            sync_sleep(0.01);
         }
      }
   }

   // Audio thread
   void Scheduler::audio_thread_fn()
   {
      auto aud = RSound<int16_t>::shared("localhost", file->audio().channels, file->audio().rate);

      while (threads_active)
      {
         if (is_paused)
         {
            aud->pause();
            while (is_paused)
               sync_sleep(0.01);

            aud->unpause();
         }

         if (aud_pkt_queue.size() > 0)
         {
            Packet pkt = aud_pkt_queue.pull();
            process_audio(pkt.get(), aud);
         }
         else
            usleep(10000);
      }
   }
}
