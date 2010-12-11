#include "rsound.hpp"
#include <iostream>
#include <iterator>
#include <vector>
#include <stdio.h>
#include <unistd.h>

using namespace AV::Audio;

static ssize_t callback(int16_t *out, size_t frames, void*)
{
   if (feof(stdin))
      return -1;

   return fread(out, 4, frames, stdin);
}

int main()
{
   Stream<int16_t>::Ptr aud = std::make_shared<RSound<int16_t>>("localhost", 2, 44100);

   std::cout << "Aud alive? " << aud->alive() << std::endl;
   
   aud->set_audio_callback(callback);

   sleep(10);
   aud->set_audio_callback(0);
   sleep(1);
   aud->set_audio_callback(callback);
   sleep(5);
}
