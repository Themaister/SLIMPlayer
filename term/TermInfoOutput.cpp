#include "TermInfoOutput.hpp"
#include <iostream>

using namespace IO;

void TermInfoOutput::output(double video_pts, double audio_pts, bool show_video, bool show_audio)
{
   printf("\r");
   if (show_video)
      printf("  V: %7.2lf", video_pts);
   if (show_audio)
      printf("  A: %7.2lf", audio_pts);
   if (show_video && show_audio)
      printf("  Delta: %7.2lf", video_pts - audio_pts);
   printf("         ");
   fflush(stdout);
}

// Clear out a newline, to make ZSH happy. ;)
TermInfoOutput::~TermInfoOutput()
{
   puts("");
}

