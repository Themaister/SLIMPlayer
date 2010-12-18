#include "opengl.hpp"

#define GL_GLEXT_PROTOTYPES
#include <GL/glfw.h>
#include <GL/glext.h>

#include <Cg/cg.h>
#include <Cg/cgGL.h>

#include <algorithm>
#include <utility>
#include <vector>
#include <stdexcept>
#include <array>
#include <iostream>

using namespace AV::Video;
using namespace AV;

namespace Internal 
{
   static const char* cg_program = 
      "void main_vertex"
      "("
      " float4 position	: POSITION,"
      " float4 color	: COLOR,"
      " float2 texCoord : TEXCOORD0,"
      ""
      " uniform float4x4 modelViewProj,"
      ""
      " out float4 oPosition : POSITION,"
      " out float4 oColor    : COLOR,"
      " out float2 otexCoord : TEXCOORD"
      " )"
      "{"
      "   oPosition = mul(modelViewProj, position);"
      "   oColor = color;"
      "   otexCoord = texCoord;"
      "}"
      ""
      ""
      "struct output"
      "{"
      "   float4 color : COLOR;"
      "};"
      ""
      "static const float3x3 yuv2mat = float3x3"
      "("
      "    1,   0,          1.13983,"
      "    1,   -0.39465,   -0.58060,"
      "    1,   2.03211,    0"
      ");"
      ""
      "float4 yuv2rgb(float3 yuv)"
      "{"
      "   float3 ret = mul(yuv2mat, yuv);"
      "   return float4(ret, 1.0);"
      "}"
      ""
      "uniform sampler2D tex_y : TEXUNIT0;"
      "uniform sampler2D tex_u : TEXUNIT1;"
      "uniform sampler2D tex_v : TEXUNIT2;"
      "uniform float2 chroma_shift;"
      ""
      ""
      "float4 yuvTEX(float2 coord)"
      "{"
      "   float3 yuv;"
      "   yuv.x = tex2D(tex_y, coord).x;"
      "   yuv.y = tex2D(tex_u, chroma_shift.x * coord).x - 0.5;"
      "   yuv.z = tex2D(tex_v, chroma_shift.y * coord).x - 0.5;"
      "   return yuv2rgb(yuv);"
      "}"
      ""
      "output main_fragment (float2 tex : TEXCOORD0)"
      "{"
      "   output OUT;"
      "   OUT.color = yuvTEX(tex);"
      "   return OUT;"
      "}";

   static float aspect_ratio;

   extern "C" {
      static void GLFWCALL resize(int, int);
   }

   static void GLFWCALL resize(int width, int height)
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

   static const GLfloat vertexes[] = {
      0, 0, 0,
      0, 1, 0,
      1, 1, 0,
      1, 0, 0
   };

   static const GLfloat tex_coords[] = {
      0, 1,
      0, 0,
      1, 0,
      1, 1
   };
}

GL::GL(unsigned in_width, unsigned in_height, float in_aspect_ratio) : width(in_width), height(in_height), aspect_ratio(in_aspect_ratio), cg_inited(false)
{
   glfwInit();

   if(glfwOpenWindow(width, height, 0, 0, 0, 0, 0, 0, GLFW_WINDOW) != GL_TRUE)
   {
      throw std::runtime_error("Failed to init GLFW.\n");
   }

   Internal::aspect_ratio = aspect_ratio;

   glfwSetWindowSizeCallback(Internal::resize);
   glfwSwapInterval(1);

   glDisable(GL_DITHER);
   glDisable(GL_DEPTH_TEST);
   glColor3f(1, 1, 1);
   glClearColor(0, 0, 0, 0);

   glGenTextures(3, gl_tex);
   glGenBuffers(1, &pbo);
   glGenBuffers(1, &vbo);

   glfwSetWindowTitle("SLIMPlayer");

   init_cg();

   std::vector<uint8_t> buf(6 * width * height);
   std::fill(buf.begin(), buf.end(), 0x80);

   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
   glBufferData(GL_PIXEL_UNPACK_BUFFER, 6 * width * height, &buf[0], GL_STREAM_DRAW);

   for (int i = 0; i < 3; i++)
   {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, gl_tex[i]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D,
            0, GL_LUMINANCE8, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (void*)(width * height * i * 2));
   }

   std::array<GLfloat, 64> vertex_buf;
   std::copy(Internal::vertexes, Internal::vertexes + 12, &vertex_buf[0]);
   std::copy(Internal::tex_coords, Internal::tex_coords + 8, &vertex_buf[32]);

   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertex_buf.size(), &vertex_buf[0], GL_STATIC_DRAW);

   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glVertexPointer(3, GL_FLOAT, 3 * sizeof(GLfloat), nullptr);
   glTexCoordPointer(2, GL_FLOAT, 2 * sizeof(GLfloat), (GLvoid*)(32 * sizeof(GLfloat)));

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

int GL::get_alignment(int pitch)
{
   if (pitch & 1)
      return 1;
   if (pitch & 2)
      return 2;
   if (pitch & 4)
      return 4;
   return 8;
}

void GL::frame(const uint8_t * const * data, const int *pitch, int w, int h)
{
   glClear(GL_COLOR_BUFFER_BIT);

   // YUV420P
   int xs = 1, ys = 1;
   cgSetParameter2f(cg.chroma_shift, 0.5, 0.5);

   for (int i = 0; i < 3; i++)
   {
      glBufferSubData(GL_PIXEL_UNPACK_BUFFER, width * height * i * 2, (pitch[i] * h) >> (i ? ys : 0), data[i]);
   }

   for (int i = 0; i < 3; i++)
   {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, gl_tex[i]);

      glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(pitch[i]));
      glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch[i]); 
      glTexSubImage2D(GL_TEXTURE_2D,
            0, 0, 0, w >> (i ? xs : 0), h >> (i ? ys : 0), GL_LUMINANCE, GL_UNSIGNED_BYTE, (void*)(width * height * i * 2));
   }

}

void GL::subtitle(const Sub::Message& msg)
{
   std::cout << "Got subtitle!!!" << std::endl;
   for (int i = 0; i < 3; i++)
   {
      glBufferSubData(GL_PIXEL_UNPACK_BUFFER, width * height * i * 2, msg.w * msg.h >> (i ? 2 : 0), &msg.data[i][0]);
   }

   for (int i = 0; i < 3; i++)
   {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, gl_tex[i]);

      glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(msg.w >> (i ? 1 : 0)));
      glPixelStorei(GL_UNPACK_ROW_LENGTH, msg.w >> (i ? 1 : 0)); 
      glTexSubImage2D(GL_TEXTURE_2D,
            0, msg.x >> (i ? 1 : 0), msg.y >> (i ? 1 : 0), msg.w >> (i ? 1 : 0), msg.h >> (i ? 1 : 0), GL_LUMINANCE, GL_UNSIGNED_BYTE, (void*)(width * height * i * 2));

   }
}

void GL::flip()
{
   glFlush();
   glDrawArrays(GL_QUADS, 0, 4);
   glfwSwapBuffers();
}

GL::~GL()
{
   uninit_cg();
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glDeleteTextures(3, gl_tex);
   glDeleteBuffers(1, &pbo);
   glDeleteBuffers(1, &vbo);

   glfwTerminate();
}

void GL::init_cg()
{
   CGparameter cg_mvp_matrix;
   cg.cgCtx = cgCreateContext();
   if (cg.cgCtx == nullptr)
   {
      throw std::runtime_error("Failed to create Cg context\n");
   }
   cg.cgFProf = cgGLGetLatestProfile(CG_GL_FRAGMENT);
   cg.cgVProf = cgGLGetLatestProfile(CG_GL_VERTEX);
   if (cg.cgFProf == CG_PROFILE_UNKNOWN || cg.cgVProf == CG_PROFILE_UNKNOWN)
   {
      throw std::runtime_error("Cg: Invalid profile type\n");
   }
   cgGLSetOptimalOptions(cg.cgFProf);
   cgGLSetOptimalOptions(cg.cgVProf);
   cg.cgFPrg = cgCreateProgram(cg.cgCtx, CG_SOURCE, Internal::cg_program, cg.cgFProf, "main_fragment", 0);
   cg.cgVPrg = cgCreateProgram(cg.cgCtx, CG_SOURCE, Internal::cg_program, cg.cgVProf, "main_vertex", 0);
   if (cg.cgFPrg == nullptr || cg.cgVPrg == NULL)
   {
      throw std::runtime_error("Cg compile error\n");
   }

   cgGLLoadProgram(cg.cgFPrg);
   cgGLLoadProgram(cg.cgVPrg);
   cgGLEnableProfile(cg.cgFProf);
   cgGLEnableProfile(cg.cgVProf);
   cgGLBindProgram(cg.cgFPrg);
   cgGLBindProgram(cg.cgVPrg);

   cg_mvp_matrix = cgGetNamedParameter(cg.cgVPrg, "modelViewProj");
   cg.chroma_shift = cgGetNamedParameter(cg.cgFPrg, "chroma_shift");
   cgGLSetStateMatrixParameter(cg_mvp_matrix, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
   cg_inited = true;
}

void GL::uninit_cg()
{
   if (cg_inited)
   {
      cgDestroyContext(cg.cgCtx);
      cg_inited = false;
   }
}

namespace Internal
{
   static const std::vector<std::pair<int, EventHandler::Event>> glfw_cmd = {
      {GLFW_KEY_ESC, EventHandler::Event::Quit},
      {GLFW_KEY_SPACE, EventHandler::Event::Pause},
      {GLFW_KEY_LEFT, EventHandler::Event::SeekBack10},
      {GLFW_KEY_RIGHT, EventHandler::Event::SeekForward10},
      {GLFW_KEY_UP, EventHandler::Event::SeekForward60},
      {GLFW_KEY_DOWN, EventHandler::Event::SeekBack60}
   };

   static auto current_event = EventHandler::Event::None;

   extern "C" {
      static GLFWCALL void keypress_cb(int, int);
      static GLFWCALL int winclose_cb();
   }

   static GLFWCALL void keypress_cb(int key, int action)
   {
      if (action == GLFW_RELEASE)
         return;

      auto itr = std::find_if(glfw_cmd.begin(), glfw_cmd.end(), 
            [key](const std::pair<int, EventHandler::Event>& event)
            {
               return key == event.first;
            });

      if (itr != glfw_cmd.end())
         current_event = itr->second;
   }

   static GLFWCALL int winclose_cb()
   {
      current_event = EventHandler::Event::Quit;
      return GL_FALSE;
   }
}

GLEvent::GLEvent() : thr(pthread_self())
{
   glfwSetKeyCallback(Internal::keypress_cb);
   glfwSetWindowCloseCallback(Internal::winclose_cb);
}

void GLEvent::poll()
{
   // Need to do this in same thread as GL. :(
   if (pthread_equal(pthread_self(), thr))
      glfwPollEvents();
}

EventHandler::Event GLEvent::event()
{
   auto tmp = Internal::current_event;
   Internal::current_event = EventHandler::Event::None;
   return tmp;
}

