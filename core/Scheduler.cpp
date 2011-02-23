/*
 *  SLIMPlayer - Simple and Lightweight Media Player
 *  Copyright (C) 2010 - Hans-Kristian Arntzen
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


// Seem to be some libstdc++ issue for now.
#define _GLIBCXX_USE_NANOSLEEP

#include "FF.hpp"
#include "AV.hpp"
#include "Scheduler.hpp"
#include "audio/alsa.hpp"
#include "audio/null.hpp"
#include "video/opengl.hpp"
#include "subs/ASSRender.hpp"
#include <iostream>
#include <array>
#include <memory>
#include <thread>
#include <chrono>
#include <algorithm>


using namespace FF;
using namespace AV::Audio;
using namespace AV::Video;
using namespace AV::Sub;

namespace AV
{
   Scheduler::Scheduler(MediaFile::Ptr in_file) : file(in_file), is_active(true), video_pts(0.0), audio_pts(0.0), audio_pts_ts(get_time()), video_pts_ts(get_time()), audio_written(0), is_paused(false), audio_pts_hack(false), video_thread_active(false), audio_thread_active(false)
   {
      has_video = file->video().active;
      has_audio = file->audio().active;

      if (has_video)
      {
         video_thread_active = true;
         video_thread = std::thread(&Scheduler::video_thread_fn, this);
      }
      if (has_audio)
      {
         audio_thread_active = true;
         audio_thread = std::thread(&Scheduler::audio_thread_fn, this);
      }
   }

   Scheduler::~Scheduler()
   {
      video_thread_active = false;
      audio_thread_active = false;

      if (has_video)
         video_thread.join();
      if (has_audio)
         audio_thread.join();
   }

   bool Scheduler::active() const
   {
      bool res = is_active || audio_thread_active || video_thread_active;
      return res;
   }

   EventHandler::Event Scheduler::next_event()
   {
      auto event = EventHandler::Event::None;

      std::for_each(event_handlers.begin(), event_handlers.end(), 
            [&event](EventHandler::APtr& handler) 
            {
               handler->poll();
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

   void Scheduler::perform_seek(double time)
   {
      avlock.lock();

      if (sub_renderer.get() != nullptr)
      {
         gfx_lock.lock();
         sub_renderer->flush();
         gfx_lock.unlock();
      }

      if (has_audio)
      {
         audio_lock.lock();
         audio->pause();
         audio_lock.unlock();
      }

      video_pts += time;

      // We have to seek and be able to calculate what the new audio PTS will be :( :( 
      // Very dirty, but can't find a better way for now.
      if (audio_pts_hack)
      {
         if (file->video().active)
            std::cerr << "Audio PTS hack activated! Video might blow up or seek really slow." << std::endl;

         // We will seek to this absolute time.
         audio_written = ((has_video ? video_pts : audio_pts) + time) * (file->audio().rate * file->audio().channels * 2);
         audio_lock.lock();
         gfx_lock.lock();
         file->seek(video_pts, has_video ? video_pts : audio_pts, time, FF::SeekTarget::Audio);
         gfx_lock.unlock();
         audio_lock.unlock();

      }
      else
      {
         audio_written += file->audio().rate * file->audio().channels * time * 2;
         file->seek(video_pts, audio_pts, time);
      }

      if (has_audio)
      {
         audio_lock.lock();
         audio->unpause();
         audio_lock.unlock();
      }

      aud_pkt_queue.clear();
      vid_pkt_queue.clear();
      avlock.unlock();
      is_paused = false;
   }

   void Scheduler::show_info()
   {
      std::for_each(info_handlers.begin(), info_handlers.end(),
            [this](IO::InfoOutput::APtr& ptr)
            {
               double time = get_time();
               if (is_paused)
                  ptr->output(video_pts, audio_pts, file->video().active, file->audio().active);
               else
                  ptr->output(video_pts + (time - video_pts_ts), audio_pts + (time - audio_pts_ts), file->video().active, file->audio().active);
            });
   }

   void Scheduler::run()
   {
      avlock.lock();
      auto event = next_event();
      avlock.unlock();

      if (!is_active)
      {
         sync_sleep(0.01);
         return;
      }

      show_info();

      switch (event)
      {
         case EventHandler::Event::Quit:
            //std::cerr << "Quitting!!!" << std::endl;
            is_active = false;
            video_thread_active = false;
            audio_thread_active = false;
            return;

         case EventHandler::Event::Pause:
            //std::cerr << "Pause toggling stream!!!" << std::endl;
            pause_toggle();
            break;

         case EventHandler::Event::SeekBack10:
            //std::cerr << "Seeking backwards!!!" << std::endl;
            perform_seek(-10.0);
            break;

         case EventHandler::Event::SeekForward10:
            //std::cerr << "Seeking forward!!!" << std::endl;
            perform_seek(10.0);
            break;

         case EventHandler::Event::SeekBack60:
            //std::cerr << "Seeking backwards!!!" << std::endl;
            perform_seek(-60.0);
            break;

         case EventHandler::Event::SeekForward60:
            //std::cerr << "Seeking forward!!!" << std::endl;
            perform_seek(60.0);
            break;

         case EventHandler::Event::None:
            break;

         default:
            throw std::runtime_error("Unknown event popped up :V\n");
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
            // Signal to threads that there won't be any more data.
            aud_pkt_queue.finalize();
            vid_pkt_queue.finalize();
            return;
            
         case Packet::Type::None:
            return;

         case Packet::Type::Audio:
            while (aud_pkt_queue.size() > 16 && (vid_pkt_queue.size() > 16 || !has_video))
               aud_pkt_queue.wait();

            aud_pkt_queue.push(std::move(pkt));
            break;

         case Packet::Type::Video:
            while (vid_pkt_queue.size() > 16 && (aud_pkt_queue.size() > 16 || !has_audio))
               vid_pkt_queue.wait();

            vid_pkt_queue.push(std::move(pkt));
            break;

         case Packet::Type::Subtitle:
            while (sub_pkt_queue.size() > 64 && (aud_pkt_queue.size() > 16 || !has_audio) && (vid_pkt_queue.size() > 16 || !has_video))
               sub_pkt_queue.wait();

            sub_pkt_queue.push(std::move(pkt));
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

   double Scheduler::frame_time() const
   {
      double frame_time = av_q2d(file->video().ctx->time_base) * file->video().ctx->ticks_per_frame;
      if (file->video().ctx->time_base.num == 1 && file->video().ctx->time_base.den > 1000)
         frame_time *= 1000.0;
      return frame_time;
   }

   void Scheduler::process_video(AVPacket& pkt, Display::APtr vid, AVFrame *frame)
   {
      if (!has_video)
         return;

      int finished = 0;

      uint64_t pts = pkt.pts;
      FF::set_global_pts(pkt.pts);

      gfx_lock.lock();
      avcodec_decode_video2(file->video().ctx, frame, &finished, &pkt);
      gfx_lock.unlock();

      if (pkt.dts == (int64_t)AV_NOPTS_VALUE && frame->opaque && *(uint64_t*)frame->opaque != AV_NOPTS_VALUE)
      {
         pts = *(uint64_t*)frame->opaque;
      }
      else if (pkt.dts != (int64_t)AV_NOPTS_VALUE)
      {
         pts = pkt.dts;
      }

      if (finished)
      {
         if (pts != AV_NOPTS_VALUE)
            video_pts = pts * av_q2d(file->video().time_base);
         else
            video_pts += frame_time();

         video_pts += frame->repeat_pict / (2.0 * frame_time());

         if (file->video().ctx->pix_fmt == PIX_FMT_YUV420P)
         {
            vid->frame(frame->data, frame->linesize, file->video().width, file->video().height);
         }
         else // Should fix this up.
         {
            std::swap(frame->data[1], frame->data[2]);
            std::swap(frame->linesize[1], frame->linesize[2]);
            vid->frame(frame->data, frame->linesize, file->video().width, file->video().height);
         }
      }
   }

   void Scheduler::process_audio(AVPacket& pkt)
   {
      if (!has_audio)
         return;

      uint8_t *data = pkt.data;
      size_t size = pkt.size;

      // AVCODEC_MAX_AUDIO_FRAME_SIZE would do, but FFmpeg needs some extra padding-stuff, so why not...
      std::array<uint8_t, AVCODEC_MAX_AUDIO_FRAME_SIZE * 2> buf;
      size_t written = 0;
      while (pkt.size > 0)
      {
         int out_size = buf.size() - written;
         audio_lock.lock();
         int ret = avcodec_decode_audio3(file->audio().ctx, reinterpret_cast<int16_t*>(&buf[written]), &out_size, &pkt);
         audio_lock.unlock();
         if (ret <= 0)
            break;

         pkt.size -= ret;
         pkt.data += ret;
         written += out_size;
      }
      pkt.data = data;
      pkt.size = size;

      audio_lock.lock();
      audio->write(reinterpret_cast<const int16_t*>(&buf[0]), written / 2);
      audio_lock.unlock();

      avlock.lock();
      audio_written += written;

      if (pkt.pts != (int64_t)AV_NOPTS_VALUE)
      {
         audio_pts = pkt.pts * av_q2d(file->audio().time_base) - audio->delay();
      }
      else if (pkt.dts != (int64_t)AV_NOPTS_VALUE)
      {
         audio_pts = pkt.dts * av_q2d(file->audio().time_base) - audio->delay();
      }
      else
      {
         audio_pts = (double)audio_written/(file->audio().rate * file->audio().channels * 2) - audio->delay();
         //std::cerr << "Couldn't get audio pts nor dts. Guessing!" << std::endl;
         audio_pts_hack = true;
      }
      //std::cout << "Audio PTS: " << audio_pts << std::endl;

      audio_pts_ts = get_time();
      avlock.unlock();
   }

   void Scheduler::process_subtitle(Display::APtr&& vid)
   {
      for (;;)
      {
         avlock.lock();
         if (sub_pkt_queue.size() == 0)
         {
            avlock.unlock();
            break;
         }
         auto packet = sub_pkt_queue.pull();
         avlock.unlock();

         auto& pkt = packet.get();

         uint8_t *data = pkt.data;
         size_t size = pkt.size;

         int finished = 0;
         AVSubtitle sub;

         int ret;

         while (pkt.size > 0)
         {
            gfx_lock.lock();
            ret = avcodec_decode_subtitle2(file->sub().ctx, &sub, &finished, &pkt);
            gfx_lock.unlock();

            if (ret <= 0)
            {
               std::cerr << "Decode subtitle failed." << std::endl;
               break;
            }

            pkt.data += ret;
            pkt.size -= ret;
         }
         pkt.data = data;
         pkt.size = size;

         if (finished)
         {
            for (unsigned i = 0; i < sub.num_rects; i++)
            {
               if (sub.rects[i]->ass)
               {
                  // Push our new message to handler queue. We just handle ASS atm, but hey ;)
                  gfx_lock.lock();
                  sub_renderer->push_msg(sub.rects[i]->ass, video_pts);
                  gfx_lock.unlock();
               }
            }
         }
         else
         {
            std::cout << "Did not finish a subtitle frame." << std::endl;
         }

         avsubtitle_free(&sub);
      }

      // Print all subtitle currently active in this PTS to screen.
      gfx_lock.lock();
      auto& list = sub_renderer->msg_list(video_pts);
      std::for_each(list.begin(), list.end(), 
            [&vid](const Message& msg)
            {
               vid->subtitle(msg);
            });
      gfx_lock.unlock();
   }

   void Scheduler::add_event_handler(EventHandler::APtr handler)
   {
      std::lock_guard<std::mutex> f(avlock);
      event_handlers.push_back(handler);
   }

   void Scheduler::add_info_handler(IO::InfoOutput::APtr handler)
   {
      std::lock_guard<std::mutex> f(avlock);
      info_handlers.push_back(handler);
   }

   // Video thread
   void Scheduler::video_thread_fn()
   {
      auto vid = GL::shared(file->video().width, file->video().height, file->video().aspect_ratio);
      auto event = GLEvent::shared();

      if (file->sub().active)
         sub_renderer = ASSRenderer::shared(file->sub().fonts, file->sub().ass_data, file->video().width, file->video().height);

      AVFrame *frame = avcodec_alloc_frame();

      // Add event handler for GL.
      add_event_handler(event);

      while (video_thread_active && vid_pkt_queue.alive())
      {
         event->poll(); 
         avlock.lock();
         if (vid_pkt_queue.size() > 0 && !is_paused)
         {
            auto pkt = vid_pkt_queue.pull();
            avlock.unlock();
            process_video(pkt.get(), vid, frame);

            if (file->sub().active)
               process_subtitle(vid);

            // We have to calculate how long we should wait before swapping frame to screen.
            // We sync everything to audio clock.
            double delta = get_time();

            avlock.lock();
            delta -= audio_pts_ts;
            double sleep_time = video_pts - (audio_pts + delta);
            avlock.unlock();

            // Yes, it can happen! :(
            if (delta < 0.0)
               delta = 0.0;
            //std::cout << "Delta: " << delta << std::endl;

            if (video_pts > (audio_pts + delta) && audio_thread_active)
            {
               double last_frame_delta = get_time();
               last_frame_delta -= video_pts_ts;

               // :(
               if (last_frame_delta < 0.0)
                  last_frame_delta = 0.0;

               // We try to keep the sleep time to a somewhat small value to avoid choppy video in some cases.
               // Max sleep time should be a bit over 1 frame time to allow audio to catch up.
               double max_sleep = 1.2 * frame_time() - last_frame_delta;

               if (max_sleep < 0.0)
                  max_sleep = 0.0;

               if (sleep_time > max_sleep)
               {
                  sleep_time = max_sleep;
               }
               //std::cout << "Sleep for " << sleep_time << std::endl;
               sync_sleep(sleep_time);
            }

            video_pts_ts = get_time();
            vid->flip();
         }
         else
         {
            avlock.unlock();

            if (is_paused)
               sync_sleep(0.01);
            else
            {
               // Having some race conditions... quickfix it for now.
               vid_pkt_queue.wait();
               //sync_sleep(0.01);
               //vid_pkt_queue.signal();
            }
         }
      }
      video_thread_active = false;
      av_free(frame);
   }

   // Audio thread
   void Scheduler::audio_thread_fn()
   {
      audio = ALSA<int16_t>::shared(file->audio().channels, file->audio().rate);
      if (!audio->alive())
      {
         std::cerr << "Failed to open audio driver, using Null driver!" << std::endl;
         audio = Null<int16_t>::shared(file->audio().channels, file->audio().rate);
      }

      while (audio_thread_active && aud_pkt_queue.alive())
      {
         if (is_paused)
         {
            audio->pause();
            while (is_paused)
               sync_sleep(0.01);

            audio->unpause();
         }

         avlock.lock();
         if (aud_pkt_queue.size() > 0)
         {
            auto pkt = aud_pkt_queue.pull();
            avlock.unlock();
            process_audio(pkt.get());
         }
         else
         {
            avlock.unlock();
            // Having some race conditions, quickfix it for now...
            aud_pkt_queue.wait();
            //sync_sleep(0.01);
            //aud_pkt_queue.signal();
         }
      }
      audio_thread_active = false;
   }
}
