#ifndef __OPENGL_HPP
#define __OPENGL_HPP

#include "display.hpp"
#include "AV.hpp"

#include "subs/subtitle.hpp"

#define GL_GLEXT_PROTOTYPES
#include <Cg/cg.h>
#include <GL/glfw.h>
#include <pthread.h>

namespace AV {
namespace Video {

   class GL : public Display, public General::Shared<GL>
   {
      public:
         GL(unsigned in_width, unsigned in_height, float in_aspect_ratio);

         GL(const GL&) = delete;
         void operator=(const GL&) = delete;

         void frame(const uint8_t * const * data, const int *pitch, int w, int h);
         void subtitle(const AV::Sub::Message& msg);
         void flip();

         ~GL();
      private:
         uintptr_t width;
         uintptr_t height;
         float aspect_ratio;
         bool cg_inited;

         GLuint gl_tex[4];
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

   class GLEvent : public AV::EventHandler, public General::Shared<GLEvent>
   {
      public:
         GLEvent();
         Event event();
         void poll();
      private:
         // Find C++0x solution for this!
         pthread_t thr;
   };

}}

#endif
