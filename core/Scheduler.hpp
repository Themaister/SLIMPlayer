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


#ifndef __SCHEDULER_HPP
#define __SCHEDULER_HPP

#include "AV.hpp"
#include "term/InfoOutput.hpp"

namespace AV
{
   class Scheduler : private General::SmartDefs<Scheduler>
   {
      public:
         DECL_SMART(Scheduler);
         Scheduler(FF::MediaFile::Ptr in_file);
         void operator=(const Scheduler&) = delete;
         Scheduler(const Scheduler&) = delete;

         ~Scheduler();

         void add_event_handler(EventHandler::Ptr ptr);
         void add_info_handler(IO::InfoOutput::Ptr ptr);

         bool active() const;
         void run();
         static void sync_sleep(float time);

      private:
         FF::MediaFile::Ptr file;
         bool has_video;
         bool has_audio;
         volatile bool is_active;
         double video_pts;
         double audio_pts;
         double audio_pts_ts;
         double video_pts_ts;
         size_t audio_written;
         volatile bool is_paused;
         std::mutex avlock;
         std::mutex audio_lock;
         std::mutex gfx_lock;

         std::list<EventHandler::Ptr> event_handlers;
         std::list<IO::InfoOutput::Ptr> info_handlers;
         EventHandler::Event next_event();

         volatile bool video_thread_active;
         volatile bool audio_thread_active;
         PacketQueue vid_pkt_queue;
         PacketQueue aud_pkt_queue;
         PacketQueue sub_pkt_queue;
         AV::Sub::Renderer::Ptr sub_renderer;

         std::thread video_thread;
         std::thread audio_thread;
         Video::Display::Ptr video;
         Audio::Stream<int16_t>::Ptr audio;

         void perform_seek(double delta);

         void process_subtitle(AV::Video::Display::Ptr);
         void process_video(AVPacket&, AV::Video::Display::Ptr, AVFrame*);
         void process_audio(AVPacket&, AlignedBuffer<int16_t>&);
         void pause_toggle();

         void video_thread_fn();
         void audio_thread_fn();

         double frame_time() const;
         static double get_time();
         void show_info();
   };
}


#endif
