#ifndef __ASS_RENDER_HPP
#define __ASS_RENDER_HPP

extern "C" 
{
#include <ass/ass.h>
}

#include <list>
#include <memory>
#include <string>
#include "General.hpp"
#include "subtitle.hpp"

namespace AV 
{
   namespace Sub 
   {
      class ASSRenderer : public Renderer, public General::Shared<ASSRenderer>
      {
         public:
            ASSRenderer();
            ~ASSRenderer();

            void push_msg(const std::string &msg, double video_pts);
            const ListType& msg_list(double timestamp);

         private:
            ASS_Library *library;
            ASS_Renderer *renderer;
            ASS_Track *track;

            ListType active_list;

            static Message create_message(ASS_Image *img);
      };
   }
}

#endif
