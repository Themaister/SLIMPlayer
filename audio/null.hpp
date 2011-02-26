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

#ifndef __AUDIO_NULL_H
#define __AUDIO_NULL_H

#include "stream.hpp"
#include <string>
#include <thread>
#include <iostream>
#include <thread>
#include <chrono>
#include "Scheduler.hpp"

namespace AV 
{
   namespace Audio 
   {
      // This should do better at A/V sync, but hey. :)
      template <class T>
      class Null : public Stream<T>, public General::SmartDefs<Null<T>>
      {
         public:
            DECL_SMART(Null<T>);
            Null(unsigned in_chan, unsigned in_rate) : rate(in_rate), chan(in_chan) {}

            size_t write(const T*, size_t samples)
            {
               AV::Scheduler::sync_sleep((float)samples / (rate * chan));
               return samples;
            }

            bool alive() const
            {
               return true;
            }

            size_t write_avail()
            {
               return 0; // Return something arbitrary.
            }

         private:
            unsigned rate, chan;
      };
   }
}

#endif
