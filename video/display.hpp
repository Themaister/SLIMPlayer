#ifndef __VIDEO_HPP
#define __VIDEO_HPP

#include "General.hpp"

namespace AV {
namespace Video {

   class Display : public General::Shared<Display>
   {
      public:
         virtual ~Display();
   };

}}

#endif
