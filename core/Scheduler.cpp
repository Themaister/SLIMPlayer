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
   }

   Scheduler::~Scheduler()
   {
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
            process_audio(pkt.get());
            break;

         case Packet::Type::Video:
            process_video(pkt.get());
            break;

         default:
            throw std::runtime_error("What kind of package is this? o.o\n");
      }
   }

   void Scheduler::process_video(AVPacket& pkt)
   {
      size_t size = pkt.size;

      std::cout << "process_video(), size: " << size << std::endl;

      int finished;

      avcodec_decode_video2(file->video().ctx, frame, &finished, &pkt);

      if (finished)
         video->show(frame->data, frame->linesize, file->video().width, file->video().height);
   }

   void Scheduler::process_audio(AVPacket& pkt)
   {
      if (!has_audio)
         return;

      size_t size = pkt.size;
      std::cout << "process_audio(), size: " << size << std::endl;

      std::array<int16_t, AVCODEC_MAX_AUDIO_FRAME_SIZE / 2> buf;
      while (size > 0)
      {
         int out_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
         int ret = avcodec_decode_audio3(file->audio().ctx, &buf[0], &out_size, &pkt);
         if (ret <= 0)
            break;

         size -= ret;

         audio->write(&buf[0], out_size / 2);
      }
   }
}
