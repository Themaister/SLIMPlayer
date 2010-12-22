#include "TermEvent.hpp"
#include <poll.h>
#include <iostream>
#include <termios.h>
#include <stdexcept>
#include <fcntl.h>
#include <string.h>

using namespace IO;
using namespace AV;

namespace IO
{
   namespace Internal
   {
      static const std::vector<std::pair<const char*, EventHandler::Event>> events = {
         { "h", EventHandler::Event::SeekBack10},
         { "j", EventHandler::Event::SeekBack60},
         { "k", EventHandler::Event::SeekForward60},
         { "l", EventHandler::Event::SeekForward10},
         { " ", EventHandler::Event::Pause},
         { "q", EventHandler::Event::Quit},
         { "\e[D", EventHandler::Event::SeekBack10},
         { "\e[B", EventHandler::Event::SeekBack60},
         { "\e[A", EventHandler::Event::SeekForward60},
         { "\e[C", EventHandler::Event::SeekForward10},
         { "\e", EventHandler::Event::Quit}
      };
   }
}


TermEvent::TermEvent() : current(EventHandler::Event::None)
{
   struct termios term, term_orig;
   if (tcgetattr(0, &term_orig))
      throw std::runtime_error("Couldn't set up termios!\n");

   term = term_orig;
   term.c_lflag &= ~(ICANON | ECHO);
   term.c_cc[VMIN] = 0;
   term.c_cc[VTIME] = 0;

   if (tcsetattr(0, TCSANOW, &term))
      throw std::runtime_error("Failed to set termios...\n");

   if (fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK) < 0)
      throw std::runtime_error("Failed to set fcntl...\n");
}

EventHandler::Event TermEvent::event()
{
   auto event = current;
   current = EventHandler::Event::None;
   return event;
}

void TermEvent::poll()
{
   struct pollfd fd = {0, POLLIN};

   ::poll(&fd, 1, 0);

   if (fd.revents & POLLIN)
   {
      char buf[5] = {0};
      ssize_t rc = read(0, buf, sizeof(buf) - 1);
      if (rc <= 0)
         return;

      for (auto itr = Internal::events.begin(); itr != Internal::events.end(); ++itr)
      {
         if (strcmp(buf, itr->first) == 0)
         {
            current = itr->second;
            break;
         }
      }
   }
   else
      current = EventHandler::Event::None;
}
