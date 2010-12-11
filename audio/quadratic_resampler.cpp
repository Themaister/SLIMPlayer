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





#include "quadratic_resampler.hpp"
#include <vector>

namespace Audio {

inline size_t QuadraticResampler::required_samples(size_t samples)
{
   size_t after_sum = sum_output_samples + samples;

   size_t min_input_samples = (size_t)((after_sum / ratio) + channels * 2);
   return ((min_input_samples - sum_input_samples)/channels) * channels;
}

inline void QuadraticResampler::poly_create_3(float *poly, float *y)
{
   poly[2] = (y[0] - 2*y[1] + y[2])/2;
   poly[1] = -1.5*y[0] + 2*y[1] - 0.5*y[2];
   poly[0] = y[0];
}

size_t QuadraticResampler::process(size_t samples, float *out_data)
{
   size_t frames = samples/channels;
   size_t frames_used = 0;
   uint64_t pos_out;
   double pos_in = 0.0;

   uint64_t sum_output_frames = sum_output_samples / channels;
   uint64_t sum_input_frames = sum_input_samples / channels;
   for (uint64_t x = sum_output_frames; x < sum_output_frames + frames; x++)
   {
      pos_out = x - sum_output_frames;
      pos_in  = ((double)x / ratio) - (double)sum_input_frames;
      for (unsigned c = 0; c < channels; c++)
      {
         float poly[3];
         float rdata[3];
         float x_val;

         if ((int)pos_in == 0)
         {
            rdata[0] = data[0 * channels + c];
            rdata[1] = data[1 * channels + c];
            rdata[2] = data[2 * channels + c];
            x_val = pos_in;
         }
         else
         {
            rdata[0] = data[((int)pos_in - 1) * channels + c];
            rdata[1] = data[((int)pos_in + 0) * channels + c];
            rdata[2] = data[((int)pos_in + 1) * channels + c];
            x_val = pos_in - (int)pos_in + 1.0;
         }

         poly_create_3(poly, rdata);

         out_data[pos_out * channels + c] = poly[2] * x_val * x_val + poly[1] * x_val + poly[0];
      }
   }
   frames_used = (int)pos_in;
   return frames_used * channels;
}

ssize_t QuadraticResampler::pull(float *out, size_t samples)
{
   // How many samples must we have to resample?
   size_t req_samples = required_samples(samples);

   // Do we need to read more data?
   ssize_t ret;
   if (data.size() < req_samples)
   {
      size_t must_read = req_samples - data.size();
      std::vector<float> buffer(must_read);
      ret = read(&buffer[0], must_read);

      if (ret <= 0) // We're done.
         return -1;
      data.insert(data.end(), buffer.begin(), buffer.end());
   }

   // Phew. We should have enough data in our buffer now to be able to process the data we need.

   size_t samples_used = process(samples, out);
   sum_input_samples += samples_used;
   data.erase(data.begin(), data.begin() + samples_used);
   sum_output_samples += samples;

   return samples;
}

}

