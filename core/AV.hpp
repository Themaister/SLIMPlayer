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
   class PacketQueue : public General::ProducerConsumer
   {
      public:
         PacketQueue();
         void push(FF::Packet&& in);
         FF::Packet pull();
         size_t size() const;
         void clear();
         void finalize();
         bool alive() const;

      private:
         std::queue<FF::Packet> queue;
         mutable std::mutex lock;
         bool is_final;
   };

   template <class T>
   class AlignedBuffer
   {
      public:
         AlignedBuffer(size_t in_size = 0) : m_buf((T*)av_mallocz(in_size * sizeof(T))), m_size(in_size) {}

         AlignedBuffer(const AlignedBuffer& in_buf) { *this = in_buf; }
         AlignedBuffer(AlignedBuffer&& in_buf) { *this = std::move(in_buf); }

         AlignedBuffer& operator=(const AlignedBuffer& in_buf)
         {
            resize(in_buf.size());
            std::copy(&in_buf[0], &in_buf[0] + in_buf.size(), m_buf);
            return *this;
         }

         AlignedBuffer& operator=(AlignedBuffer&& in_buf)
         {
            if (m_buf) av_free(m_buf);
            m_buf = in_buf.m_buf;
            m_size = in_buf.m_size;
            in_buf.m_buf = nullptr;
            in_buf.m_size = 0;

            return *this;
         }

         T& operator[](size_t index) { return m_buf[index]; }
         const T& operator[](size_t index) const { return m_buf[index]; }

         size_t size() const { return m_size; }
         void resize(size_t in_size) { m_buf = av_realloc(m_buf, in_size * sizeof(T)); m_size = size; }

      private:
         T *m_buf;
         size_t m_size;
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
            Fullscreen,
            None,
         };

         virtual Event event() = 0;
         virtual void poll() = 0;

         virtual ~EventHandler() {}
   };

}

#endif
