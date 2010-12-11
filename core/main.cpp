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
      Stream<int16_t>::Ptr audio = std::make_shared<RSound<int16_t>>("localhost", media_file->audio().channels, media_file->audio().rate);
      Display::Ptr video = std::make_shared<GL>(media_file->video().width, media_file->video().height, media_file->video().aspect_ratio);

      AV::Scheduler sched(media_file, audio, video);

      while (sched.active())
      {
         sched.run();
      }
   }
   catch (std::exception &e) { std::cerr << e.what() << std::endl; }

}
