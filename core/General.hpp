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


#ifndef __GENERAL_HPP
#define __GENERAL_HPP

#include <memory>
#include <mutex>
#include <condition_variable>
#include <string>
#include <sstream>

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

   // Inherit this privately to use a generic smart pointer interface.
   template <class T>
   struct SmartDefs
   {
      typedef typename Internal::DeclareShared<T>::type Ptr;
      typedef typename Internal::DeclareUnique<T>::type UPtr;

      template <class... P>
      static Ptr shared(P&&... p)
      {
         return std::make_shared<T>(std::forward<P>(p)...);
      }

      template <class... P>
      static UPtr unique(P&&... p)
      {
         return std::unique_ptr<T>(new T(std::forward<P>(p)...));
      }
   };

   // Apply one of these macros in public: section of your class.
   //
#define DECL_SHARED(type) using ::General::SmartDefs< type >::Ptr; \
   using ::General::SmartDefs< type >::shared

#define DECL_UNIQUE(type) using ::General::SmartDefs< type >::UPtr; \
   using ::General::SmartDefs< type >::unique

#define DECL_SMART(type) DECL_SHARED(type); \
   DECL_UNIQUE(type)


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

   class ProducerConsumer
   {
      public:
         ProducerConsumer();
         ~ProducerConsumer();

         void signal();
         void wait();
      private:
         bool can_sleep;
         std::condition_variable cond;
         std::mutex cond_lock;
   };

   template <class T>
   std::string join(const T& t)
   {
      std::ostringstream stream;
      stream << t;
      return stream.str();
   }

   template <class T, class R, class... P>
   std::string join(const T& t, const R& r, const P&... p)
   {
      std::ostringstream stream;
      stream << t << join(r, p...);
      return stream.str();
   }
}

#endif
