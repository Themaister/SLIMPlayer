#ifndef __OPENGL_HPP
#define __OPENGL_HPP

#include "display.hpp"

namespace AV {
namespace Video {

   class GL : public Display
   {
      public:
         GL(unsigned in_width, unsigned in_height, float in_aspect_ratio);

         GL(const GL&) = delete;
         void operator=(const GL&) = delete;
         ~GL();
      private:
         unsigned width;
         unsigned height;
         float aspect_ratio;
   };

}}

#endif
