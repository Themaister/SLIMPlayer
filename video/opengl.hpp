#ifndef __OPENGL_HPP
#define __OPENGL_HPP

#include "display.hpp"

namespace AV {
namespace Video {

   class GL : public Display
   {
      public:
         GL(unsigned width, unsigned height, float aspect_ratio);
         GL(GL&&);
         GL& operator=(GL&&);

         GL(const GL&) = delete;
         void operator=(const GL&) = delete;
         ~GL();
   };

}}

#endif
