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


#ifndef __AV_HPP
#define __AV_HPP

#include "General.hpp"
#include "FF.hpp"
#include "video/display.hpp"
#include "audio/stream.hpp"
#include <queue>
#include <list>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <stdint.h>

namespace AV
{
   class PacketQueue
   {
      public:
         PacketQueue();
         ~PacketQueue();
         void push(FF::Packet&& in);
         FF::Packet pull();
         size_t size() const;
         void clear();
         void finalize();
         bool alive() const;
         void wait() const;
         void signal() const;

      private:
         std::queue<FF::Packet> queue;
         mutable std::mutex lock;
         mutable std::condition_variable cond;
         mutable std::mutex cond_lock;
         bool is_final;
   };

   class EventHandler : public General::SharedAbstract<EventHandler>
   {
      public:
         enum class Event : unsigned
         {
            Pause,
            Quit,
            SeekBack10,
            SeekForward10,
            SeekBack60,
            SeekForward60,
            None,
         };

         virtual Event event() = 0;
         virtual void poll() = 0;

         virtual ~EventHandler() {}
   };

}

#endif
