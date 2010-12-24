#include "General.hpp"

using namespace General;

ProducerConsumer::ProducerConsumer() : can_sleep(true)
{}

ProducerConsumer::~ProducerConsumer()
{
   cond.notify_all();
}

void ProducerConsumer::wait()
{
   std::unique_lock<std::mutex> l(cond_lock);
   if (can_sleep)
   {
      can_sleep = false;
      cond.notify_one();
      cond.wait(l);
      can_sleep = true;
   }
   else
      cond.notify_one();
}

void ProducerConsumer::signal()
{
   cond.notify_one();
}
