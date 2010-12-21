#include "TermEvent.hpp"

using namespace Input;
using namespace AV;

TermEvent::TermEvent()
{}

TermEvent::~TermEvent()
{}

EventHandler::Event TermEvent::event()
{
   return EventHandler::Event::None;
}

void TermEvent::poll()
{}
