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





#ifndef __AUDIO_QUAD_RESAMPLER_HPP
#define __AUDIO_QUAD_RESAMPLER_HPP

#include "resampler.hpp"
#include <deque>
#include <stdint.h>

namespace Audio {

class QuadraticResampler : public Resampler
{
   public:
      template <class T>
      QuadraticResampler(T& drain_obj, double in_ratio, unsigned in_channels) : Resampler(drain_obj), ratio(in_ratio), 
         channels(in_channels), sum_output_samples(0), sum_input_samples(0) {}

      ssize_t pull(float *out, size_t samples);
   private:
      unsigned channels;
      uint64_t sum_output_samples;
      uint64_t sum_input_samples;
      double ratio;
      std::deque<float> data;

      inline size_t required_samples(size_t samples);
      inline void poly_create_3(float *poly, float *y);
      size_t process(size_t samples, float *out_data);
};

}

#endif
