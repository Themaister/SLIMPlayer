
#ifndef __GENERAL_HPP
#define __GENERAL_HPP

#include <memory>

namespace General
{
   template <class T>
   struct SharedVirtual
   {
      typedef std::shared_ptr<T> Ptr;
   };

   template <class T>
   struct Shared : public SharedVirtual<T>
   {
      typedef typename SharedVirtual<T>::Ptr Ptr;

      template <class... P>
      static Ptr shared(P&&... args) 
      { 
         return std::make_shared<T>(std::forward<P>(args)...); 
      }
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
