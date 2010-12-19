#ifndef __AV_HPP
#define __AV_HPP

#include "General.hpp"
#include "FF.hpp"
#include "video/display.hpp"
#include "audio/stream.hpp"
#include <queue>
#include <list>
#include <mutex>
#include <thread>
#include <stdint.h>

namespace AV
{
   class PacketQueue
   {
      public:
         PacketQueue();
         void push(FF::Packet&& in);
         FF::Packet pull();
         size_t size() const;
         void clear();
         void finalize();
         bool alive() const;

      private:
         std::queue<FF::Packet> queue;
         mutable std::mutex lock;
         bool is_final;
   };

   class EventHandler : public General::SharedAbstract<EventHandler>
   {
      public:
         enum class Event : unsigned
         {
            Pause,
            Quit,
            SeekBack10,
            SeekForward10,
            SeekBack60,
            SeekForward60,
            None,
         };

         virtual Event event() = 0;
         virtual void poll() = 0;

         virtual ~EventHandler() {}
   };

}

#endif
