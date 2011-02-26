#ifndef __INFO_OUTPUT_HPP
#define __INFO_OUTPUT_HPP

#include "General.hpp"

namespace IO
{
   class InfoOutput : private General::SmartDefs<InfoOutput>
   {
      public:
         DECL_SMART(InfoOutput);
         virtual ~InfoOutput() {}
         virtual void output(double video_pts, double audio_pts, bool show_video, bool show_audio) = 0;
   };
}

#endif
