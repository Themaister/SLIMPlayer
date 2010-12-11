#include "FF.hpp"
#include "AV.hpp"
#include "audio/stream.hpp"
#include "video/display.hpp"
#include <iostream>
#include <array>

using namespace FF;
using namespace AV::Audio;
using namespace AV::Video;

namespace AV
{
   Scheduler::Scheduler(MediaFile::Ptr in_file, Stream<int16_t>::Ptr in_audio, Display::Ptr in_vid) : file(in_file), audio(in_audio), video(in_vid), is_active(true)
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

   void Scheduler::run()
   {
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
            aud_pkt_queue.push(std::move(pkt));
            break;

         case Packet::Type::Video:
            vid_pkt_queue.push(std::move(pkt));
            break;

         default:
            throw std::runtime_error("What kind of package is this? o.o\n");
      }
   }

   void Scheduler::process_video(AVPacket& pkt)
   {
      size_t size = pkt.size;

      //std::cout << "process_video(), size: " << size << std::endl;

      int finished;

      avcodec_decode_video2(file->video().ctx, frame, &finished, &pkt);

      std::cout << "Video DTS: " << pkt.dts << " DTIME: " << pkt.dts * av_q2d(file->video().ctx->time_base) << std::endl;
      std::cout << "Video PTS: " << pkt.pts << " PTIME: " << pkt.pts * av_q2d(file->video().ctx->time_base) << std::endl;

      if (finished)
      {
         video->frame(frame->data, frame->linesize, file->video().width, file->video().height);
         video->flip();
      }
   }

   void Scheduler::process_audio(AVPacket& pkt)
   {
      if (!has_audio)
         return;

      size_t size = pkt.size;
      //std::cout << "process_audio(), size: " << size << std::endl;

      std::array<int16_t, AVCODEC_MAX_AUDIO_FRAME_SIZE / 2> buf;
      while (size > 0)
      {
         int out_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
         int ret = avcodec_decode_audio3(file->audio().ctx, &buf[0], &out_size, &pkt);
         if (ret <= 0)
            break;

         std::cout << "Audio DTS: " << pkt.dts << " DTIME: " << pkt.dts * av_q2d(file->audio().ctx->time_base) << std::endl;
         std::cout << "Audio PTS: " << pkt.pts << " PTIME: " << pkt.pts * av_q2d(file->audio().ctx->time_base) << std::endl;

         size -= ret;

         audio->write(&buf[0], out_size / 2);
      }
   }

   void Scheduler::video_thread_fn()
   {
      while (threads_active)
      {
         if (vid_pkt_queue.size() > 0)
         {
            Packet pkt = vid_pkt_queue.pull();
            process_video(pkt.get());
            usleep(10000);
         }
         else
            usleep(10000);
      }
   }

   void Scheduler::audio_thread_fn()
   {
      while (threads_active)
      {
         if (aud_pkt_queue.size() > 0)
         {
            Packet pkt = aud_pkt_queue.pull();
            process_audio(pkt.get());
            usleep(10000);
         }
         else
            usleep(10000);
      }
   }
}
