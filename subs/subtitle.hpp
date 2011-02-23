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


#ifndef __SUBTITLE_HPP
#define __SUBTITLE_HPP

#include "General.hpp"
#include <list>
#include <stdint.h>
#include <array>
#include <vector>

namespace AV
{
   namespace Sub
   {
      struct Rect
      {
         Rect(unsigned _x, unsigned _y, unsigned _w, unsigned _h, unsigned _stride) 
            : x(_x), y(_y), w(_w), h(_h), stride(_stride) {}
         unsigned x, y, w, h, stride;
      };

      struct Color
      {
         Color(float _r, float _g, float _b) 
            : r(_r), g(_g), b(_b) {}
         float r, g, b;
      };

      // A message to the screen. Float values are relative [0.0, 1.0]. Picture format is YV12.
      struct Message : public General::Shared<Message>
      {
         Message(const Rect& in_rect, const Color& in_color, const uint8_t *in_data) :
            rect(in_rect), color(in_color)
         {
            data = std::vector<uint8_t>(in_data, in_data + rect.stride * rect.h);
         }

         Message(Message&&) = default;

         Rect rect;
         Color color;
         std::vector<uint8_t> data;
      };

      class Renderer : public General::SharedAbstract<Renderer>
      {
         public:
            virtual ~Renderer() {};

            virtual void push_msg(const std::string& msg, double video_pts) = 0;
            typedef std::list<Message> ListType;

            virtual const ListType& msg_list(double timestamp) = 0;
            virtual void flush() = 0;
      };
   }
}


#endif
