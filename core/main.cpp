#include "FF.hpp"
#include "audio/rsound.hpp"
#include "video/opengl.hpp"
#include "AV.hpp"
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
      MediaFile::Ptr media_file = std::make_shared<MediaFile>(argv[1]);
      
      AV::Scheduler sched(media_file);

      while (sched.active())
      {
         sched.run();
         usleep(2000);
      }
   }
   catch (std::exception &e) { std::cerr << e.what() << std::endl; }
}
