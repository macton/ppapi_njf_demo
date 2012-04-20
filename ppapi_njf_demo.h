// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include <stdint.h>

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/ppb.h"

const int kInvalidInstance = 0;
const int kInvalidResource = 0;

struct InstanceInfo 
{
  PPB_GetInterface browser_interface;
  PP_Instance      instance_id;
  PP_Resource      graphics3d_id;
  int32_t          frame_counter;
  int32_t          width;
  int32_t          height;
  int32_t          cursor_x;
  int32_t          cursor_y;
  volatile int32_t keys_pressed;
};

extern void RenderFrameStartup(InstanceInfo* instance);
extern void RenderFrame(InstanceInfo* instance);
