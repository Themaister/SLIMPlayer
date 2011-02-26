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

namespace AV {
namespace Video {

   class GLEvent;

   class GL : public Display, private General::SmartDefs<GL>
   {
      public:
         DECL_SMART(GL);
         GL(unsigned in_width, unsigned in_height, float in_aspect_ratio);

         GL(const GL&) = delete;
         void operator=(const GL&) = delete;

         void frame(const uint8_t * const * data, const int *pitch, int w, int h);
         void subtitle(const AV::Sub::Message& msg);
         void flip();
         void toggle_fullscreen();
         void get_rect(unsigned& w, unsigned& h);

         ~GL();
      private:
         unsigned width;
         unsigned height;
         unsigned fullscreen_x;
         unsigned fullscreen_y;
         static unsigned current_x;
         static unsigned current_y;
         bool fullscreen;
         bool do_fullscreen;

         GLuint gl_tex[4];

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
         friend class GLEvent;
   };

   class GLEvent : public AV::EventHandler, private General::SmartDefs<GLEvent>
   {
      public:
         DECL_SMART(GLEvent);
         GLEvent();
         Event event();
         void poll();
      private:
         std::thread::id thread_id;
         EventHandler::Event cur_evnt;
   };

}}

#endif
