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
#include <GL/gl.h>
#include <GL/glext.h>
#define NO_SDL_GLEXT
#include "SDL.h"
#include "SDL_opengl.h"
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
         bool cg_inited;

         GLuint gl_tex[4];
         GLuint pbo;
         GLuint vbo;

         void init_glsl();

         struct
         {
            GLuint gl_program;
            GLuint fragment_program;
            GLint chroma_shift;
         } glsl;

         static unsigned get_alignment(unsigned pitch);
         static void set_viewport(unsigned width, unsigned height);
         static void print_shader_log(GLuint obj);
         static void print_linker_log(GLuint obj);

         static float aspect_ratio;
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
