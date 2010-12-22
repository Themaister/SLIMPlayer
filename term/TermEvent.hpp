#ifndef __STDIO_EVENT_HPP
#define __STDIO_EVENT_HPP

#include "AV.hpp"
#include "General.hpp"

namespace IO
{
   class TermEvent : public AV::EventHandler, public General::Shared<TermEvent>
   {
      public:
         TermEvent();
         void poll();
         EventHandler::Event event();
      private:
         EventHandler::Event current;
   };
}

#endif
