#ifndef __ASS_RENDER_HPP
#define __ASS_RENDER_HPP

extern "C" 
{
#include <ass/ass.h>
}

#include <list>
#include <memory>

namespace AV 
{
   namespace Sub 
   {
      class ASSRenderer
      {
         public:
            ASSRenderer();
            ~ASSRenderer();

            void push_msg(const std::string &msg);


         private:
            ASS_Library *lib;
            ASS_Track *track;
            ASS_Renderer *renderer;
      };
   }
}

#endif
