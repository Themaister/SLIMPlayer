#include "FF.hpp"
#include "AV.hpp"

using namespace AV;
using namespace FF;

PacketQueue::PacketQueue() : is_final(false)
{}

void PacketQueue::push(Packet&& in)
{
   lock.lock();
   queue.push(std::move(in));
   lock.unlock();
}

Packet PacketQueue::pull()
{
   if (size() == 0)
      throw std::runtime_error("Tried to pull nothing from queue!\n");

   lock.lock();
   Packet pkt = std::move(queue.front());
   queue.pop();
   lock.unlock();
   return pkt;
}

size_t PacketQueue::size() const
{
   lock.lock();
   size_t len = queue.size();
   lock.unlock();
   return len;
}

void PacketQueue::clear()
{
   lock.lock();
   while (!queue.empty())
      queue.pop();
   lock.unlock();
}

bool PacketQueue::alive() const
{
   std::lock_guard<std::mutex> f(lock);
   return queue.size() > 0 || !is_final;
}

void PacketQueue::finalize()
{
   std::lock_guard<std::mutex> f(lock);
   is_final = true;
}
