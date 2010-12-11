/******************************************************************************* 
 *  -- Cellframework -  Open framework to abstract the common tasks related to
 *                      PS3 application development.
 *
 *  Copyright (C) 2010
 *       Hans-Kristian Arntzen
 *       Stephen A. Damm
 *       Daniel De Matteis
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ********************************************************************************/





#ifndef __AUDIO_RESAMPLER_HPP
#define __AUDIO_RESAMPLER_HPP

#include <deque>
#include <stddef.h>
#include <unistd.h>

namespace Audio {

class Resampler
{
   public:
      template <class T>
      Resampler(T& drain_obj) : callback(new Caller<T>(drain_obj)) {}
      virtual ~Resampler() { delete callback; }

      virtual ssize_t pull(float *out, size_t samples) = 0;

   protected:
      ssize_t read(float *out, size_t samples);

   private:
      struct Callback
      {
         virtual ssize_t read(float **data) = 0;
         virtual ~Callback() {}
      };

      template<class T>
      struct Caller : public Callback
      {
         Caller(T& in) : obj(in) {}
         ssize_t read(float **data) { return obj(data); }
         T& obj;
      };

      Callback *callback;
      std::deque<float> buf;
};

}

#endif
