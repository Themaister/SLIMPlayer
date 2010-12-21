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
            ASSRenderer(const std::vector<std::pair<std::string, std::vector<uint8_t>>>& fonts, const std::vector<uint8_t>& ass_data, unsigned width, unsigned height);
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
