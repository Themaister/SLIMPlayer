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
            CGprogram cgSFPrg;
            CGprogram cgSVPrg;
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
