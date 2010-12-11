#include "FF.hpp"
#include "Audio.hpp"
#include "Video.hpp"
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

   FF::init();

   try
   {
      MediaFile::Ptr media_file = std::make_shared<MediaFile>(argv[1]);
      Stream::Ptr audio = std::make_shared<RSound>(media_file.audio().channels, media_file.audio().rate);
      Display::Ptr video = std::make_shared<GL>(media_file.video().width, media_file().height, media_file().aspect_ratio);

      AV::Scheduler sched(media_file, audio, video);

      while (sched.active())
      {
         sched.run();
      }
   }
   catch (std::exception &e) { std::cerr << e.what() << std::endl; }

   FF::deinit();
}
