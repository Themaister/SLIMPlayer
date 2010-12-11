#ifndef __OPENGL_HPP
#define __OPENGL_HPP

#include "display.hpp"

#define GL_GLEXT_PROTOTYPES
#include <Cg/cg.h>
#include <GL/glfw.h>

namespace AV {
namespace Video {

   class GL : public Display
   {
      public:
         GL(unsigned in_width, unsigned in_height, float in_aspect_ratio);

         GL(const GL&) = delete;
         void operator=(const GL&) = delete;

         void show(const uint8_t * const * data, const int *pitch, int w, int h);

         ~GL();
      private:
         unsigned width;
         unsigned height;
         float aspect_ratio;
         bool cg_inited;

         GLuint gl_tex[3];
         GLuint pbo;
         GLuint vbo;

         void init_cg();
         void uninit_cg();

         struct
         {
            CGcontext cgCtx;
            CGprogram cgFPrg;
            CGprogram cgVPrg;
            CGprofile cgFProf;
            CGprofile cgVProf;
            CGparameter chroma_shift;
         } cg;

         static int get_alignment(int pitch);
   };

}}

#endif
