#ifndef __STDIO_EVENT_HPP
#define __STDIO_EVENT_HPP

#include "AV.hpp"
#include "General.hpp"

namespace Input
{
   class TermEvent : public AV::EventHandler, public General::Shared<TermEvent>
   {
      public:
         TermEvent();
         ~TermEvent();

         void poll();
         EventHandler::Event event();
   };
}

#endif
