#ifndef __STDIO_EVENT_HPP
#define __STDIO_EVENT_HPP

#include "AV.hpp"
#include "General.hpp"

namespace IO
{
   class TermEvent : public AV::EventHandler, private General::SmartDefs<TermEvent>
   {
      public:
         DECL_SMART(TermEvent);
         TermEvent();
         void poll();
         EventHandler::Event event();
      private:
         EventHandler::Event current;
   };
}

#endif
