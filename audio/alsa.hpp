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

#ifndef __AUDIO_ALSA_H
#define __AUDIO_ALSA_H

#include "stream.hpp"
#include <string>
#include <thread>
#include <iostream>
#include <alsa/asoundlib.h>

namespace AV 
{
   namespace Audio 
   {

      template <class T>
      class ALSA : public Stream<T>, private General::SmartDefs<ALSA<T>>
      {
         public:
            DECL_SMART(ALSA<T>);
            ALSA(unsigned channels, unsigned samplerate, const std::string& device = "default") : runnable(true), pcm(nullptr), params(nullptr), fps(samplerate)
            {
               int rc = snd_pcm_open(&pcm, device.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
               if (rc < 0)
               {
                  runnable = false;
                  throw DeviceException(General::join("Unable to open PCM device ", snd_strerror(rc)));
               }

               snd_pcm_format_t fmt = type_to_format(T());

               if (snd_pcm_hw_params_malloc(&params) < 0)
               {
                  runnable = false;
                  throw DeviceException("Failed to allocate memory.");
               }

               runnable = false;
               if (
                     (snd_pcm_hw_params_any(pcm, params) < 0) ||
                     (snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) ||
                     (snd_pcm_hw_params_set_channels(pcm, params, channels) < 0) ||
                     (snd_pcm_hw_params_set_format(pcm, params, fmt) < 0) ||
                     (snd_pcm_hw_params_set_rate(pcm, params, samplerate, 0) < 0) ||
                     ((rc = snd_pcm_hw_params(pcm, params)) < 0)
                  )
                  throw DeviceException(General::join("Unable to install HW params: ", snd_strerror(rc)));

               runnable = true;
            }

            ~ALSA()
            {
               if (params)
                  snd_pcm_hw_params_free(params);
               if (pcm)
               {
                  snd_pcm_drop(pcm);
                  snd_pcm_close(pcm);
               }
            }

            size_t write(const T* in, size_t samples)
            {
               if (!runnable)
                  return 0;

               snd_pcm_sframes_t frames = snd_pcm_bytes_to_frames(pcm, samples * sizeof(T));
               snd_pcm_sframes_t rc = snd_pcm_writei(pcm, in, frames);
               if (rc == -EPIPE || rc == -EINTR || rc == -ESTRPIPE)
               {
                  if (snd_pcm_recover(pcm, rc, 1) < 0)
                  {
                     runnable = false;
                     return 0;
                  }
               }
               return samples;
            }

            bool alive() const
            {
               return runnable;
            }

            size_t write_avail()
            {
               snd_pcm_sframes_t rc = snd_pcm_avail(pcm);
               if (rc < 0)
               {
                  runnable = false;
                  return 0;
               }

               return snd_pcm_frames_to_bytes(pcm, rc) / sizeof(T);
            }

            void pause()
            {
               if (runnable)
               {
                  if (snd_pcm_drop(pcm) < 0)
                     runnable = false;
               }
            }

            void unpause()
            {
               if (runnable)
               {
                  if (snd_pcm_prepare(pcm) < 0)
                     runnable = false;
               }
            }

            float delay()
            {
               if (!runnable)
                  return 0.0;

               snd_pcm_sframes_t delay;
               snd_pcm_delay(pcm, &delay);

               return (float)delay / fps;
            }

         private:

            bool runnable;
            snd_pcm_t *pcm;
            snd_pcm_hw_params_t *params;
            unsigned fps;

            // Little-endian only so far!
            snd_pcm_format_t type_to_format(int16_t) { return SND_PCM_FORMAT_S16_LE; }
      };

   }
}

#endif
