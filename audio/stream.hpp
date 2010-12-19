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


#ifndef __AUDIO_STREAM_H
#define __AUDIO_STREAM_H

#include <mutex>
#include <stddef.h>
#include <algorithm>
#include <limits>
#include <string.h>
#include <stdlib.h>
#include "General.hpp"
#include <atomic>

namespace AV 
{
   namespace Audio 
   {
      template<class T>
      class Stream : public General::SharedAbstract<Stream<T>>
      {
         public:
            Stream() : m_callback(nullptr), m_saved_callback(NULL) {}

            // Returns number of samples you can write without blocking.
            virtual size_t write_avail() = 0;
            // Writes 'samples' samples to buffer. Will block until everything is written. Writing more data than write_avail() returns with an empty buffer is not safe.
            virtual size_t write(const T* in, size_t samples) = 0;
            // Notifies interface that you will not be writing more data to buffer until unpause();
            virtual void pause() 
            { 
               m_saved_callback = m_callback; 
               m_callback = nullptr;
            }
            // Notifies interface that you would like to start writing data again.
            virtual void unpause() 
            {
               if (!m_callback)
                  m_callback = m_saved_callback;
            }
            // Returns false if initialization failed. It is possible that that a pause()/unpause() sequence will remedy this.
            virtual bool alive() const { return true; }

            // Returns current audio latency in seconds.
            virtual float delay() { return 0.0; }

            // By giving this a function pointer different than nullptr, callback interface is activated. write() and write_avail() are no-ops. The callback will call this function sporadically. You can return a number of frames less than desired, but this will usually mean the driver itself will fill the rest with silence. cb_data is userdefined callback data. This can be NULL. After activating callback, by calling this again with NULL for callback argument, callbacks will be disabled and you can use normal, blocking write() and write_avail() again.
            virtual void set_audio_callback(ssize_t (*cb)(T*, size_t frames, void *data), void *cb_data = nullptr)
            {
               m_callback = cb;
               data = cb_data;
            }

            virtual ~Stream() {}

         protected:
            inline bool callback_active()
            {
               return m_callback;
            }

            inline ssize_t callback(T* out, size_t frames)
            {
               if (!m_callback)
               {
                  return -1;
               }

               return m_callback(out, frames, data);
            }

         private:
            ssize_t (*m_callback)(T*, size_t, void*);
            ssize_t (*m_saved_callback)(T*, size_t, void*);
            void* data;
      };
   }
}

#endif
