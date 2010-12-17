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
         Message(float in_x, float in_y, float in_w, float in_h, 
               unsigned in_img_w, unsigned in_img_h, const uint8_t *in_data[3]) :
            img_w(in_img_w), img_h(in_img_h), x(in_x), y(in_y), w(in_w), h(in_h)
         {
            for (int i = 0; i < 3; i++)
               data[i] = std::vector<uint8_t>(in_data[i], in_data[i] + img_w * img_h);
         }

         unsigned img_w, img_h;
         float x, y, w, h;
         std::array<std::vector<uint8_t>, 3> data;
      };

      class Renderer : public General::SharedAbstract<Renderer>
      {
         public:
            virtual ~Renderer() {};

            virtual void push_msg(const std::string& msg) = 0;
            typedef std::list<Message> ListType;

            virtual const ListType& msg_list(double timestamp) const = 0;
      };
   }
}


#endif
