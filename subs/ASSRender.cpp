#include "ASSRender.hpp"
#include <stdarg.h>
#include <stdio.h>
#include <iostream>

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

ASSRenderer::ASSRenderer()
{
   library = ass_library_init();
   ass_set_message_cb(library, Internal::ass_msg_cb, nullptr);

   renderer = ass_renderer_init(library);
   ass_set_frame_size(renderer, 1280, 720);

   track = ass_new_track(library);
}

ASSRenderer::~ASSRenderer()
{
   ass_free_track(track);
   ass_renderer_done(renderer);
   ass_library_done(library);
}

void ASSRenderer::push_msg(const std::string &msg, double video_pts)
{
   std::cout << "Push_msg!" << std::endl;

   ass_process_codec_private(track, const_cast<char*>(msg.c_str()), msg.size());
}

const ASSRenderer::ListType& ASSRenderer::msg_list(double pts) const
{
   int change;
   ASS_Image *img = ass_render_frame(renderer, track, (long long)(pts * 1000), &change);
   
   if (img)
   {
      std::cout << "Yay, got image! :D" << std::endl;
   }

   return active_list;
}

