
#ifndef __GENERAL_HPP
#define __GENERAL_HPP

#include <memory>

namespace General
{
   template <class T>
   struct Shared
   {
      typedef std::shared_ptr<T> Ptr;
   };

   template<class T>
   class RefCounted
   {
      public:
         unsigned& ref()
         {
            return cnt;
         }

      private:
         static unsigned cnt;
   };

   template<class T>
   unsigned RefCounted<T>::cnt = 0;
}

#endif
