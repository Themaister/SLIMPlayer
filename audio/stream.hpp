
#ifndef __AUDIO_STREAM_H
#define __AUDIO_STREAM_H

#include <mutex>
#include <stddef.h>
#include <algorithm>
#include <limits>
#include <string.h>
#include <stdlib.h>
#include "../General.hpp"

namespace Audio 
{
   template<class T>
   class Stream : public General::Shared<Stream<T>>
   {
      public:
         Stream() : m_callback(NULL), m_saved_callback(NULL) {}

         // Returns number of samples you can write without blocking.
         virtual size_t write_avail() = 0;
         // Writes 'samples' samples to buffer. Will block until everything is written. Writing more data than write_avail() returns with an empty buffer is not safe.
         virtual size_t write(const T* in, size_t samples) = 0;
         // Notifies interface that you will not be writing more data to buffer until unpause();
         virtual void pause() 
         { 
            std::scoped_lock foo(lock);
            m_saved_callback = m_callback; 
            m_callback = NULL;
         }
         // Notifies interface that you would like to start writing data again.
         virtual void unpause() 
         {
            std::scoped_lock foo(lock);
            if (!m_callback)
               m_callback = m_saved_callback;
         }
         // Returns false if initialization failed. It is possible that that a pause()/unpause() sequence will remedy this.
         virtual bool alive() const { return true; }

         // Returns current audio latency in seconds.
         virtual float delay() { return 0.0; }

         // By giving this a function pointer different than NULL, callback interface is activated. write() and write_avail() are no-ops. The callback will call this function sporadically. You can return a number of frames less than desired, but this will usually mean the driver itself will fill the rest with silence. cb_data is userdefined callback data. This can be NULL. After activating callback, by calling this again with NULL for callback argument, callbacks will be disabled and you can use normal, blocking write() and write_avail() again.
         virtual void set_audio_callback(ssize_t (*cb)(T*, size_t frames, void *data), void *cb_data)
         {
            std::scoped_lock foo(lock);
            m_callback = cb;
            data = cb_data;
         }

         virtual ~Stream() {}
         
      protected:
         inline bool callback_active()
         {
            std::scoped_lock foo(lock);
            return m_callback;
         }

         inline ssize_t callback(T* out, size_t frames)
         {
            std::scoped_lock foo(lock);
            if (!m_callback)
               return -1;

            return m_callback(out, frames, data);
         }

      private:
         ssize_t (*m_callback)(T*, size_t, void*);
         ssize_t (*m_saved_callback)(T*, size_t, void*);
         Threads::Mutex lock;
         void *data;
   };
}

#endif
