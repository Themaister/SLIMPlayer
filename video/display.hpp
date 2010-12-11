#ifndef __VIDEO_HPP
#define __VIDEO_HPP

#include "General.hpp"
#include <stdint.h>

namespace AV {
namespace Video {

   class Display : public General::Shared<Display>
   {
      public:
         Display() {}

         virtual void frame(const uint8_t * const * data, const int *pitch, int w, int h) = 0;
         virtual void flip() = 0;

         virtual ~Display() {}
   };

}}

#endif
