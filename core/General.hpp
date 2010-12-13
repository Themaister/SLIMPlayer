
#ifndef __GENERAL_HPP
#define __GENERAL_HPP

#include <memory>

namespace General
{
   // Template magic incoming!
   namespace Internal
   {
      template <class T>
      struct DeclareShared
      {
         typedef std::shared_ptr<T> type;
      };

      template <class T>
      struct DeclareUnique
      {
         typedef std::unique_ptr<T> type;
      };
   }

   // Inherit one of these classes to use a generic smart pointer interface.

   // If class is abstract, use this interface.
   template <class T>
   struct SharedAbstract
   {
      typedef typename Internal::DeclareShared<T>::type APtr;
   };

   template <class T>
   struct Shared
   {
      typedef typename Internal::DeclareShared<T>::type Ptr;

      template <class... P>
      static Ptr shared(P&&... args) 
      { 
         return std::make_shared<T>(std::forward<P>(args)...); 
      }
   };

   template <class T>
   struct UniqueAbstract
   {
      typedef typename Internal::DeclareUnique<T>::type AUPtr;
   };

   template <class T>
   struct Unique
   {
      typedef typename Internal::DeclareUnique<T>::type UPtr;

      template <class... P>
      static UPtr shared(P&&... args) 
      { 
         return std::unique_ptr<T>(new T(std::forward<P>(args)...)); 
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
