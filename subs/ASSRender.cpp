#include "ASSRender.hpp"
#include <stdarg.h>
#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <array>
#include <vector>

using namespace AV::Sub;

namespace AV
{
   namespace Sub
   {
      namespace Internal
      {
         extern "C" 
         {
            static void ass_msg_cb(int level, const char *fmt, va_list args, void *data);
         }

         static void ass_msg_cb(int level, const char *fmt, va_list args, void *)
         {
            std::cerr << "libass debug:" << std::endl;
            vfprintf(stderr, fmt, args);
            std::cerr << std::endl;
         }
      }
   }
}

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)

#define clip(f) (uint16_t)(f > 255.0 ? 255 : (f < 0.0 ? 0 : (uint16_t)f))

#define rgb_to_y(r, g, b) clip(0.299 * r + 0.587 * g + 0.114 * b)
#define rgb_to_u(r, g, b) clip(-0.147 * r - 0.2888 * g + 0.436 * b)
#define rgb_to_v(r, g, b) clip(0.615 * r - 0.515 * g - 0.100 * b)

Message ASSRenderer::create_message(ASS_Image *img)
{
   std::array<std::vector<uint8_t>, 3> data = { 
      {  std::vector<uint8_t>(img->w * img->h), 
         std::vector<uint8_t>(img->w * img->h), 
         std::vector<uint8_t>(img->w * img->h) } 
   }; 

   std::fill(data[1].begin(), data[1].end(), 128);
   std::fill(data[2].begin(), data[2].end(), 128);

   int stride = img->stride;
   for (int y = 0; y < img->h; y++)
   {
      for (int x = 0; x < img->w; x++)
      {
         data[0][y * img->w + x] = img->bitmap[y * stride + x];
      }
   }

   const uint8_t *ptr[] = { &data[0][0], &data[1][0], &data[2][0] };
   return Message(img->dst_x, img->dst_y, img->w, img->h, ptr);
}

ASSRenderer::ASSRenderer()
{
   library = ass_library_init();
   ass_set_message_cb(library, Internal::ass_msg_cb, nullptr);

   renderer = ass_renderer_init(library);
   // Hardcode for now.
   ass_set_frame_size(renderer, 1280, 720);
   ass_set_fonts(renderer, nullptr, "Sans", 1, nullptr, 1);

   //track = ass_new_track(library);
   track = ass_read_file(library, const_cast<char*>("/tmp/test.ass"), nullptr);
   assert(track);
}

ASSRenderer::~ASSRenderer()
{
   ass_free_track(track);
   ass_renderer_done(renderer);
   ass_library_done(library);
}

// Grab decoded messages from ffmpeg here.
void ASSRenderer::push_msg(const std::string &msg, double video_pts)
{
   //std::cout << "Push_msg!" << std::endl;

   //ass_process_codec_private(track, const_cast<char*>(msg.c_str()), msg.size());
}

// Return a list of messages to overlay on frame at pts.
const ASSRenderer::ListType& ASSRenderer::msg_list(double pts)
{
   std::cout << "Trying to render!" << std::endl;
   int change;
   ASS_Image *img = ass_render_frame(renderer, track, (long long)(pts * 1000), &change);
   
   if (img)
   {
      std::cout << "Yay, got image! :D" << std::endl;
   }

   if (change)
   {
      active_list.clear();
      while (img)
      {
         active_list.push_back(create_message(img));
         img = img->next;
      }
   }

   // Process some stuff and return something sensible.
   return active_list;
}

