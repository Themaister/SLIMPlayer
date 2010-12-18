#ifndef __SUBTITLE_HPP
#define __SUBTITLE_HPP

#include "General.hpp"
#include <list>
#include <stdint.h>
#include <array>
#include <vector>

namespace AV
{
   namespace Sub
   {
      // A message to the screen. Float values are relative [0.0, 1.0]. Picture format is YV12.
      struct Message : public General::Shared<Message>
      {
         Message(unsigned in_x, unsigned in_y, unsigned in_w, unsigned in_h,
               const uint32_t *in_data) :
            x(in_x), y(in_y), w(in_w), h(in_h)
         {
            data = std::vector<uint32_t>(in_data, in_data + w * h);
         }

         Message(Message&&) = default;

         unsigned x, y, w, h;
         std::vector<uint32_t> data;
      };

      class Renderer : public General::SharedAbstract<Renderer>
      {
         public:
            virtual ~Renderer() {};

            virtual void push_msg(const std::string& msg, double video_pts) = 0;
            typedef std::list<Message> ListType;

            virtual const ListType& msg_list(double timestamp) = 0;
      };
   }
}


#endif
