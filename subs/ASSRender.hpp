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
#include <utility>
#include <vector>

namespace AV 
{
   namespace Sub 
   {
      class ASSRenderer : public Renderer, public General::Shared<ASSRenderer>
      {
         public:
            ASSRenderer(const std::vector<std::pair<std::string, std::vector<uint8_t>>> fonts, const std::vector<uint8_t>& ass_data, unsigned width, unsigned height);
            ~ASSRenderer();

            void push_msg(const std::string &msg, double video_pts);
            const ListType& msg_list(double timestamp);
            void flush();

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
