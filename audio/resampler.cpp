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





#include "resampler.hpp"
#include <stddef.h>
#include <vector>
#include <algorithm>

namespace Audio {

ssize_t Resampler::read(float *out, size_t samples)
{
   while (buf.size() < samples)
   {
      float *data;
      ssize_t size = callback->read(&data);
      if (size <= 0)
         return size;
      if (data == NULL)
         return -1;

      buf.insert(buf.end(), data, data + size);
   }

   std::copy(buf.begin(), buf.begin() + samples, out);
   buf.erase(buf.begin(), buf.begin() + samples);
   return samples;
}

}
