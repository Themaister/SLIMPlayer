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


#include "opengl.hpp"

#include <algorithm>
#include <utility>
#include <vector>
#include <stdexcept>
#include <array>
#include <iostream>
#include <thread>


using namespace AV::Video;
using namespace AV;

#define CHECK_GL_ERROR() do { \
   if (glGetError() != GL_NO_ERROR) \
      throw std::runtime_error(General::join("GL failed at line: ", __LINE__, "...")); \
} while(0)

namespace Internal
{
   static const char* glsl_program = 
      "const mat3 yuv2mat = mat3"
      "("
      "    1,        1,          1,"
      "    0,        -0.39465,   2.03211,"
      "    1.13983,  -0.58060,   0"
      ");"
      ""
      "vec4 yuv2rgb(vec3 yuv)"
      "{"
      "   vec3 ret = yuv2mat * yuv;"
      "   return vec4(ret, 1.0);"
      "}"
      ""
      "uniform sampler2D tex_y;"
      "uniform sampler2D tex_u;"
      "uniform sampler2D tex_v;"
      "uniform vec2 chroma_shift;"
      ""
      "vec4 yuvTEX(vec2 coord)"
      "{"
      "   vec3 yuv;"
      "   yuv.x = texture2D(tex_y, coord).x;"
      "   yuv.y = texture2D(tex_u, chroma_shift.x * coord).x - 0.5;"
      "   yuv.z = texture2D(tex_v, chroma_shift.y * coord).x - 0.5;"
      "   return yuv2rgb(yuv);"
      "}"
      ""
      "void main()"
      "{"
      "   vec4 res = yuvTEX(gl_TexCoord[0].xy);"
      "   gl_FragColor = res;" 
      "}";

   constexpr static GLfloat vertexes[12] = {
      0, 0, 0,
      0, 1, 0,
      1, 1, 0,
      1, 0, 0
   };

   constexpr static GLfloat tex_coords[8] = {
      0, 1,
      0, 0,
      1, 0,
      1, 1
   };
}

float GL::aspect_ratio = 0.0;
unsigned GL::current_x = 0;
unsigned GL::current_y = 0;

GL::GL(unsigned in_width, unsigned in_height, float in_aspect_ratio) : width(in_width), height(in_height), fullscreen(false), do_fullscreen(false)
{
   if (SDL_Init(SDL_INIT_VIDEO) < 0)
      throw std::runtime_error("Couldn't init SDL.");

   auto video_info = SDL_GetVideoInfo();
   fullscreen_x = video_info->current_w;
   fullscreen_y = video_info->current_h;

   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

   if (!SDL_SetVideoMode(in_width, in_height, 0, SDL_OPENGL | SDL_RESIZABLE))
      throw std::runtime_error("Failed to init GL Window.");

   current_x = width;
   current_y = height;

   aspect_ratio = in_aspect_ratio;

   set_viewport(in_width, in_height);

   glDisable(GL_DITHER);
   glDisable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glColor3f(1, 1, 1);
   glClearColor(0, 0, 0, 0);

   glEnable(GL_TEXTURE_2D);
   glGenTextures(4, gl_tex);

   SDL_WM_SetCaption("SLIMPlayer", nullptr);
   SDL_ShowCursor(SDL_DISABLE);

   init_glsl();

   std::vector<uint8_t> tmp(width * height);
   std::fill(tmp.begin(), tmp.end(), 0x80);

   for (int i = 0; i < 3; i++)
   {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, gl_tex[i]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

      glTexImage2D(GL_TEXTURE_2D,
            0, GL_LUMINANCE8, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, &tmp[0]);
   }

   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glVertexPointer(3, GL_FLOAT, 3 * sizeof(GLfloat), Internal::vertexes);
   glTexCoordPointer(2, GL_FLOAT, 2 * sizeof(GLfloat), Internal::tex_coords);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   CHECK_GL_ERROR();
}


unsigned GL::get_alignment(unsigned pitch)
{
   if (pitch & 1)
      return 1;
   if (pitch & 2)
      return 2;
   if (pitch & 4)
      return 4;
   return 8;
}

void GL::toggle_fullscreen()
{
   fullscreen = !fullscreen;
   do_fullscreen = true;
}

void GL::get_rect(unsigned& in_width, unsigned& in_height)
{
   in_width = current_x;
   in_height = current_y;
}

void GL::frame(const uint8_t * const * data, const int *pitch, int w, int h)
{
   glVertexPointer(3, GL_FLOAT, 3 * sizeof(GLfloat), Internal::vertexes);
   glColor4f(1.0, 1.0, 1.0, 1.0);
   glUseProgram(glsl.gl_program);

   glClear(GL_COLOR_BUFFER_BIT);

   // YUV420P
   int xs = 1, ys = 1;
   float chromas[2] = {0.5, 0.5};
   glUniform2fv(glsl.chroma_shift, 1, chromas);

   for (int i = 0; i < 3; i++)
   {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, gl_tex[i]);

      glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(pitch[i]));
      glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch[i]); 
      glTexSubImage2D(GL_TEXTURE_2D,
            0, 0, 0, w >> (i ? xs : 0), h >> (i ? ys : 0), GL_LUMINANCE, GL_UNSIGNED_BYTE, data[i]);
   }

   glDrawArrays(GL_QUADS, 0, 4);

   glUseProgram(0);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, gl_tex[3]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   glVertexPointer(3, GL_FLOAT, 3 * sizeof(GLfloat), (void*)256);
}

void GL::subtitle(const Sub::Message& msg)
{
   glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(msg.rect.stride));
   glPixelStorei(GL_UNPACK_ROW_LENGTH, msg.rect.stride); 

   glColor4f(msg.color.r, msg.color.g, msg.color.b, msg.color.a);
   glTexImage2D(GL_TEXTURE_2D,
         0, GL_INTENSITY8, msg.rect.w, msg.rect.h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, &msg.data[0]);

   float x_l = (float)msg.rect.x / current_x;
   float x_h = (float)(msg.rect.x + msg.rect.w) / current_x;
   float y_h = (float)(current_y - msg.rect.y) / current_y;
   float y_l = (float)(current_y - msg.rect.y - msg.rect.h) / current_y;

   const GLfloat vertexes[] = {
      x_l, y_l, 0,
      x_l, y_h, 0,
      x_h, y_h, 0,
      x_h, y_l, 0
   };

   glVertexPointer(3, GL_FLOAT, 3 * sizeof(GLfloat), vertexes);
   glDrawArrays(GL_QUADS, 0, 4);
}

void GL::set_viewport(unsigned width, unsigned height)
{
   float desired_aspect;
   float device_aspect;
   float delta;

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

   desired_aspect = aspect_ratio;
   device_aspect = (float)width / height;

   // If the aspect ratios of screen and desired aspect ratio are sufficiently equal (floating point stuff), 
   // assume they are actually equal.
   if ( (int)(device_aspect*1000) > (int)(desired_aspect*1000) )
   {
      delta = (desired_aspect / device_aspect - 1.0) / 2.0 + 0.5;
      glViewport(width * (0.5 - delta), 0, 2.0 * width * delta, height);
   }

   else if ( (int)(device_aspect*1000) < (int)(desired_aspect*1000) )
   {
      delta = (device_aspect / desired_aspect - 1.0) / 2.0 + 0.5;
      glViewport(0, height * (0.5 - delta), width, 2.0 * height * delta);
   }
   else
      glViewport(0, 0, width, height);

   glOrtho(0, 1, 0, 1, -1, 1);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

void GL::flip()
{
   SDL_GL_SwapBuffers();

   if (do_fullscreen)
   {
      if (fullscreen)
      {
         SDL_SetVideoMode(fullscreen_x, fullscreen_y, 0, SDL_OPENGL | SDL_RESIZABLE | SDL_FULLSCREEN);
         set_viewport(fullscreen_x, fullscreen_y);
         current_x = fullscreen_x;
         current_y = fullscreen_y;
      }
      else
      {
         SDL_SetVideoMode(width, height, 0, SDL_OPENGL | SDL_RESIZABLE);
         set_viewport(width, height);
         current_x = width;
         current_y = height;
      }
   }
   do_fullscreen = false;
}

GL::~GL()
{
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glDeleteTextures(4, gl_tex);

   SDL_Quit();
}

void GL::print_shader_log(GLuint obj)
{
   int info_len = 0;
   int max_len;

   glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &max_len);

   std::vector<char> info_log(max_len + 1);
   glGetShaderInfoLog(obj, max_len, &info_len, &info_log[0]);

   if (info_len > 0)
      std::cerr << "Shader log: " << &info_log[0] << std::endl;

}

void GL::print_linker_log(GLuint obj)
{
   int info_len = 0;
   int max_len;

   glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &max_len);

   std::vector<char> info_log(max_len + 1);
   glGetProgramInfoLog(obj, max_len, &info_len, &info_log[0]);

   if (info_len > 0)
      std::cerr << "Linker log: " << &info_log[0] << std::endl;
}

void GL::init_glsl()
{
   glsl.gl_program = glCreateProgram();
   glsl.fragment_program = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(glsl.fragment_program, 1, static_cast<const char**>(&Internal::glsl_program), 0);
   glCompileShader(glsl.fragment_program);
   glAttachShader(glsl.gl_program, glsl.fragment_program);
   print_shader_log(glsl.fragment_program);

   glLinkProgram(glsl.gl_program);
   glUseProgram(glsl.gl_program);
   print_linker_log(glsl.gl_program);

   glsl.chroma_shift = glGetUniformLocation(glsl.gl_program, "chroma_shift");

   GLint loc = glGetUniformLocation(glsl.gl_program, "tex_y");
   glUniform1i(loc, 0);
   loc = glGetUniformLocation(glsl.gl_program, "tex_u");
   glUniform1i(loc, 1);
   loc = glGetUniformLocation(glsl.gl_program, "tex_v");
   glUniform1i(loc, 2);
}

namespace Internal
{
   static const std::vector<std::pair<int, EventHandler::Event>> cmd = {
      {SDLK_ESCAPE, EventHandler::Event::Quit},
      {SDLK_SPACE, EventHandler::Event::Pause},
      {SDLK_LEFT, EventHandler::Event::SeekBack10},
      {SDLK_RIGHT, EventHandler::Event::SeekForward10},
      {SDLK_UP, EventHandler::Event::SeekForward60},
      {SDLK_DOWN, EventHandler::Event::SeekBack60},
      {SDLK_f, EventHandler::Event::Fullscreen},
   };
}

GLEvent::GLEvent() : thread_id(std::this_thread::get_id()), cur_evnt(EventHandler::Event::None)
{
}

void GLEvent::poll()
{
   if (thread_id == std::this_thread::get_id())
   {
      SDL_Event event;
      while (SDL_PollEvent(&event))
      {
         switch (event.type)
         {
            case SDL_QUIT:
               cur_evnt = EventHandler::Event::Quit;
               break;

            case SDL_KEYDOWN:
               for (auto itr = Internal::cmd.begin(); itr != Internal::cmd.end(); ++itr)
               {
                  if (itr->first == event.key.keysym.sym)
                  {
                     cur_evnt = itr->second;
                     break;
                  }
               }
               break;

            case SDL_VIDEORESIZE:
               SDL_SetVideoMode(event.resize.w, event.resize.h, 0, SDL_OPENGL | SDL_RESIZABLE);
               GL::set_viewport(event.resize.w, event.resize.h);
               GL::current_x = event.resize.w;
               GL::current_y = event.resize.h;
               break;

            default:
               break;
         }
      }
   }
}

EventHandler::Event GLEvent::event()
{
   auto ret = cur_evnt;
   cur_evnt = EventHandler::Event::None;
   return ret;
}


