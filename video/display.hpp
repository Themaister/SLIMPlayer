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


#ifndef __VIDEO_HPP
#define __VIDEO_HPP

#define _STDC_CONSTANT_MACROS 1
#include "General.hpp"
#include <stdint.h>
#include "subs/subtitle.hpp"

namespace AV 
{
   namespace Video 
   {

      class Display : public General::SharedAbstract<Display>
      {
         public:
            Display() {}

            virtual void frame(const uint8_t * const * data, const int *pitch, int w, int h) = 0;
            virtual void subtitle(const Sub::Message& sub) = 0;
            virtual void flip() = 0;
            virtual void toggle_fullscreen() = 0;

            virtual ~Display() {}
      };

   }
}

#endif
