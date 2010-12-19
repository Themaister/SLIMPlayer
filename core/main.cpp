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



#include "FF.hpp"
#include "audio/rsound.hpp"
#include "video/opengl.hpp"
#include "AV.hpp"
#include "Scheduler.hpp"
#include <stdexcept>
#include <iostream>

using namespace FF;
using namespace AV;
using namespace AV::Audio;
using namespace AV::Video;

int main(int argc, char *argv[])
{
   if (argc != 2)
   {
      std::cerr << "Usage: " << argv[0] << " file" << std::endl;
      return 1;
   }

   try
   {
      auto media_file = MediaFile::shared(argv[1]);
      
      AV::Scheduler sched(media_file);

      while (sched.active())
      {
         sched.run();
      }
   }
   catch (std::exception &e) { std::cerr << e.what() << std::endl; }
}
