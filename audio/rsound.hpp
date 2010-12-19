
#ifndef __AUDIO_RSOUND_H
#define __AUDIO_RSOUND_H

#include "stream.hpp"
#include "rsound.h"
#include <string>
#include <thread>
#include <iostream>

namespace AV 
{
   namespace Audio 
   {

      template <class T>
      class RSound : public Stream<T>, public General::Shared<RSound<T>>
      {
         public:
            RSound(std::string server, int channels, int samplerate, int buffersize = 8092, int latency = 0) : thread_active(false), m_chan(channels)
         {
            rsd_init(&rd);
            int format = type_to_format(T());
            rsd_set_param(rd, RSD_FORMAT, &format);
            rsd_set_param(rd, RSD_CHANNELS, &channels);
            rsd_set_param(rd, RSD_HOST, const_cast<char*>(server.c_str()));
            rsd_set_param(rd, RSD_SAMPLERATE, &samplerate);

            if (buffersize < 256)
               buffersize = 256;

            buffersize *= sizeof(T);
            rsd_set_param(rd, RSD_BUFSIZE, &buffersize);
            rsd_set_param(rd, RSD_LATENCY, &latency);
            m_latency = latency;

            int rc = rsd_start(rd);
            if (rc < 0) // Couldn't start, don't do anything after this, might implement some proper error handling here.
               runnable = false;
            else
               runnable = true;

            emptybuf = new uint8_t[buffersize];
            memset(emptybuf, 0, buffersize);
         }

            ~RSound()
            {
               stop_thread();
               delete[] emptybuf;
               rsd_stop(rd);
               rsd_free(rd);
            }

            size_t write(const T* in, size_t samples)
            {
               if (!runnable || this->callback_active() || samples == 0)
                  return 0;

               rsd_delay_wait(rd);
               size_t rc = rsd_write(rd, in, samples * sizeof(T))/sizeof(T);
               if (rc == 0)
               {
                  runnable = false;
                  return 0;
               }

               // Close to underrun, fill up buffer
               if (rsd_delay_ms(rd) < (size_t)(m_latency / 2))
               {
                  size_t size = rsd_get_avail(rd);
                  rsd_write(rd, emptybuf, size);
               }
               return rc;
            }

            bool alive() const
            {
               return runnable;
            }

            size_t write_avail()
            {
               if (!runnable || this->callback_active())
                  return 0;

               // We'll block
               if (rsd_delay_ms(rd) > (size_t)m_latency)
                  return 0;

               return rsd_get_avail(rd) / sizeof(T);
            }

            void pause()
            {
               stop_thread();

               if (runnable)
               {
                  runnable = false;
                  rsd_pause(rd, 1);
               }
            }

            void set_audio_callback(ssize_t (*cb)(T*, size_t, void*), void *data = nullptr)
            {
               Stream<T>::set_audio_callback(cb, data);

               if (this->callback_active())
                  start_thread();
               else
                  stop_thread();
            }

            void unpause()
            {
               if (!runnable)
               {
                  if (rsd_pause(rd, 0) < 0)
                     runnable = false;
                  else
                     runnable = true;

                  start_thread();
               }
            }

            float delay()
            {
               if (!runnable)
                  return 0.0;

               return rsd_delay_ms(rd) / 1000.0f;
            }

         private:
            bool runnable;
            rsound_t *rd;
            int m_latency;
            uint8_t *emptybuf;
            volatile bool thread_active;
            unsigned m_chan;
            std::thread thread;

            int type_to_format(uint8_t) { return RSD_U8; }
            int type_to_format(int8_t) { return RSD_S8; }
            int type_to_format(int16_t) { return RSD_S16_NE; }
            int type_to_format(uint16_t) { return RSD_U16_NE; }
            int type_to_format(uint32_t) { return RSD_U32_NE; }
            int type_to_format(int32_t) { return RSD_S32_NE; }

            void start_thread()
            {
               if (runnable && this->callback_active() && !thread_active)
               {
                  std::cout << "Starting thread!" << std::endl;
                  thread_active = true;
                  thread = std::thread(&RSound<T>::callback_thread, this);
               }
            }

            void stop_thread()
            {
               if (runnable && thread_active)
               {
                  std::cout << "Stopping thread!" << std::endl;
                  thread_active = false;
                  thread.join();
               }
            }

            void callback_thread()
            {
               std::vector<T> buf(256 * m_chan); // Just some arbitrary size
               while (thread_active)
               {
                  ssize_t ret = callback(&buf[0], 256);
                  if (ret < 0)
                     break;

                  std::fill(buf.begin() + ret * m_chan, buf.end(), 0);

                  rsd_delay_wait(rd);
                  if (rsd_write(rd, &buf[0], 256 * m_chan * sizeof(T)) == 0)
                     break;
               }
            }
      };

   }
}

#endif
