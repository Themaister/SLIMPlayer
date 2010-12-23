#ifndef __TERM_INFO_OUTPUT_HPP
#define __TERM_INFO_OUTPUT_HPP

#include "InfoOutput.hpp"

namespace IO
{
   class TermInfoOutput : public InfoOutput, public General::Shared<TermInfoOutput>
   {
      public:
         void output(double video_pts, double audio_pts, bool show_video, bool show_audio);
         ~TermInfoOutput();
   };
}

#endif
