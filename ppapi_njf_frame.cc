// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <alloca.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "ppapi_njf_demo.h"

void RenderFrameStartup(InstanceInfo* instance) {
  char *ext;
  printf("---- Starting up 3d in NaCl\n");
  ext = (char*) glGetString(GL_EXTENSIONS);
  printf("extensions: %s\n", ext);
  int num_formats;
  glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &num_formats);
  printf("number of compressed formats: %d\n", num_formats);
  if (num_formats > 0) {
    int *formats = (int*)alloca(num_formats * sizeof(int));
    const char *fmtstr;
    char temp[1024];
    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, formats);
    for (int i = 0; i < num_formats; i++) {
      switch(formats[i]) {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
          fmtstr = "GL_COMPRESSED_RGB_S3TC_DXT1";
          break;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
          fmtstr = "GL_COMPRESSED_RGBA_S3TC_DXT1";
          break;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
          fmtstr = "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT";
          break;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
          fmtstr = "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT";
          break;
        default:
          fmtstr = (const char*) sprintf(temp, "Unknown: 0x%0x", formats[i]);
          break;
      }
      printf("format %d is %s\n", i, fmtstr);
    }
  }
}

void RenderFrame(InstanceInfo* instance) {
  static int32_t counter = 0;
  glEnable(GL_SCISSOR_TEST);
  glViewport(0, 0, instance->width, instance->height);
  glScissor(0, 0, instance->width, instance->height);
  glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glScissor(5, 5, instance->width - 10, instance->height - 10);
  float blue = 1.0f;
  glClearColor(0.0f, 0.0f, blue, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  // Draw a box under the cursor.  Need to flip vertically as gl expects
  // the origin to be the lower right corner.
  const int32_t box_size = 40;
  int32_t box_x = instance->cursor_x - box_size / 2;
  int32_t box_y = instance->height - instance->cursor_y - box_size / 2;
  glScissor(box_x, box_y, box_size, box_size);
  glClearColor(1.0f, instance->keys_pressed ? 1.0f : 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  counter++;
}

