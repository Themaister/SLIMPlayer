/*
 *  SLIMPlayer - Simple and Lightweight Media Player
 *  Copyright (C) 2010 - Hans-Kristian Arntzen
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


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
   signal();
}

Packet PacketQueue::pull()
{
   if (size() == 0)
      throw std::runtime_error("Tried to pull nothing from queue!\n");

   lock.lock();
   Packet pkt = std::move(queue.front());
   queue.pop();
   lock.unlock();
   signal();
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
   signal();
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
   signal();
}

