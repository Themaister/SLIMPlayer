#include "FF.hpp"
#include <stdexcept>
#include <iostream>

namespace FF
{

   namespace Internal
   {
      static uint64_t g_video_pkt_pts = AV_NOPTS_VALUE;

      extern "C" {
         static int get_buffer(AVCodecContext *c, AVFrame *pic);
         static void release_buffer(AVCodecContext *c, AVFrame *pic);
      }

      static int get_buffer(AVCodecContext *c, AVFrame *pic)
      {
         int ret = avcodec_default_get_buffer(c, pic);
         uint64_t *pts = (uint64_t*)av_malloc(sizeof(uint64_t));
         *pts = g_video_pkt_pts;
         pic->opaque = pts;
         return ret;
      }

      static void release_buffer(AVCodecContext *c, AVFrame *pic)
      {
         if (pic) av_freep(&pic->opaque);
         avcodec_default_release_buffer(c, pic);
      }


   }

   void set_global_pts(uint64_t pts)
   {
      Internal::g_video_pkt_pts = pts;
   }


   FFMPEG::FFMPEG()
   {
      ref()++;

      if (ref() == 1)
      {
         avcodec_init();
         av_register_all();
      }
   }

   FFMPEG::~FFMPEG()
   {}

   Packet::Packet() : pkt(nullptr)
   {
      //std::cerr << "Packet()" << std::endl;
      pkt = (AVPacket*)av_mallocz(sizeof(AVPacket));
      //av_init_packet(pkt);
   }

   Packet& Packet::operator=(Packet&& in_pkt)
   {
      //std::cerr << "Packet::operator=(&&)" << std::endl;
      if (pkt && pkt->data)
         av_free_packet(pkt);
      if (pkt)
         av_freep(&pkt);

      pkt = in_pkt.pkt;
      in_pkt.pkt = nullptr;
      return *this;
   }

   Packet::Packet(Packet&& in_pkt) : pkt(nullptr)
   {
      //std::cerr << "Packet(&&)" << std::endl;
      *this = std::move(in_pkt);
   }

   AVPacket& Packet::get()
   {
      if (pkt == nullptr)
         throw std::runtime_error("Trying to dereference nullptr\n");

      //std::cerr << "get()" << std::endl;
      return *pkt;
   }

   Packet::~Packet()
   {
      //std::cerr << "~Packet()" << std::endl;

      if (pkt && pkt->data)
         av_free_packet(pkt);
      if (pkt)
         av_freep(&pkt);
   }

   MediaFile::MediaFile(const char *path) : vcodec(nullptr), acodec(NULL), actx(NULL), vctx(NULL), fctx(NULL), vid_stream(-1), aud_stream(-1)
   {
      if (path == nullptr)
         throw std::runtime_error("Got null-path\n");

      if (av_open_input_file(&fctx, path, nullptr, 0, NULL) != 0)
         throw std::runtime_error("Failed to open file\n");

      if (av_find_stream_info(fctx) < 0)
         throw std::runtime_error("Failed to get stream information\n");

      resolve_codecs();
      set_media_info();

      // Debug
      dump_format(fctx, 0, path, false);
   }

   MediaFile::~MediaFile()
   {
      if (actx)
         avcodec_close(actx);
      if (vctx)
         avcodec_close(vctx);
      if (fctx)
         av_close_input_file(fctx);
   }

   void MediaFile::resolve_codecs()
   {

      for (unsigned i = 0; i < fctx->nb_streams; i++)
      {
         if (fctx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
         {
            vid_stream = i;
            break;
         }
      }

      for (unsigned i = 0; i < fctx->nb_streams; i++)
      {
         if (fctx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO)
         {
            aud_stream = i;
            break;
         }
      }

      if (vid_stream >= 0)
      {
         vctx = fctx->streams[vid_stream]->codec;
         vcodec = avcodec_find_decoder(vctx->codec_id);
         if (vcodec)
            avcodec_open(vctx, vcodec);
      }

      if (aud_stream >= 0)
      {
         actx = fctx->streams[aud_stream]->codec;
         acodec = avcodec_find_decoder(actx->codec_id);
         if (acodec)
            avcodec_open(actx, acodec);
      }
   }

   void MediaFile::set_media_info()
   {
      if (acodec)
      {
         aud_info.channels = actx->channels;
         aud_info.rate = actx->sample_rate;
         aud_info.active = true;
         aud_info.time_base = fctx->streams[aud_stream]->time_base;
         aud_info.ctx = actx;
      }
      else
         aud_info.active = false;

      if (vcodec)
      {
         vid_info.width = vctx->width;
         vid_info.height = vctx->height;

         // TODO: Fix me.
         vid_info.aspect_ratio = (float)vctx->width / vctx->height;
         vid_info.active = true;
         vid_info.time_base = fctx->streams[vid_stream]->time_base;
         vid_info.ctx = vctx;

         vctx->get_buffer = Internal::get_buffer;
         vctx->release_buffer = Internal::release_buffer;
      }
      else
         vid_info.active = false;
   }

   void MediaFile::seek(double video_pts, double audio_pts, double rel, SeekTarget target)
   {
      int flags = (rel < 0.0) ? AVSEEK_FLAG_BACKWARD : 0;

      double seek_to = 0.0;
      int stream = -1;
      if (vid_stream >= 0 && target != SeekTarget::Audio)
      {
         stream = vid_stream;
         seek_to = (video_pts + rel) / av_q2d(fctx->streams[vid_stream]->time_base);
      }
      else if (aud_stream >= 0)
      {
         stream = aud_stream;
         seek_to = (audio_pts + rel) / av_q2d(fctx->streams[aud_stream]->time_base);
      }

      if (av_seek_frame(fctx, stream, seek_to, flags) < 0)
         throw std::runtime_error("av_seek_frame() failed");

      if (acodec)
         avcodec_flush_buffers(actx);
      if (vcodec)
         avcodec_flush_buffers(vctx);
   }

   const MediaFile::audio_info& MediaFile::audio() const
   {
      return aud_info;
   }

   const MediaFile::video_info& MediaFile::video() const
   {
      return vid_info;
   }

   Packet::Type MediaFile::packet(Packet& pkt)
   {
      if (av_read_frame(fctx, &pkt.get()) < 0)
         return Packet::Type::Error;

      Packet::Type type = Packet::Type::None;

      int index = pkt.get().stream_index;
      if (index == aud_stream)
         type = Packet::Type::Audio;
      else if (index == vid_stream)
         type = Packet::Type::Video;

      return type;
   }
}
