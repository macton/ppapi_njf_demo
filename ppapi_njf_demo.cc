// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Demo for PPB_Graphics3D functions.

#include <assert.h>
#include <GLES2/gl2.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_graphics_3d.h"
#include "ppapi/c/pp_input_event.h"
#include "ppapi/c/pp_point.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/ppb.h"
#include "ppapi/c/ppp.h"
#include "ppapi/c/ppb_audio.h"
#include "ppapi/c/ppb_audio_config.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/c/ppb_graphics_3d.h"
#include "ppapi/c/ppb_image_data.h"
#include "ppapi/c/ppb_input_event.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/c/ppb_opengles2.h"
#include "ppapi/c/ppb_url_loader.h"
#include "ppapi/c/ppb_view.h"
#include "ppapi/c/ppp_input_event.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/gles2/gl2ext_ppapi.h"

#include "ppapi_njf_demo.h"

namespace {

// PPAPI Interfaces
const PPB_Audio* PPBAudio = NULL;
const PPB_AudioConfig* PPBAudioConfig = NULL;
const PPB_Core* PPBCore = NULL;
const PPB_Graphics3D* PPBGraphics3D = NULL;
const PPB_InputEvent* PPBInputEvent = NULL;
const PPB_Instance* PPBInstance = NULL;
const PPB_MouseInputEvent* PPBMouseInputEvent = NULL;
const PPB_OpenGLES2* PPBOpenGLES2 = NULL;
const PPB_View* PPBView = NULL;

// RenderLoop ---------------------------------------------------------------

void RenderLoop(void* user_data, int32_t result) {
  InstanceInfo* instance = static_cast<InstanceInfo*>(user_data);
  glSetCurrentContextPPAPI(instance->graphics3d_id);
  if (instance->frame_counter == 0)
    RenderFrameStartup(instance);
  RenderFrame(instance);
  instance->frame_counter++;
  glSetCurrentContextPPAPI(0);
  PP_CompletionCallback cc = PP_MakeCompletionCallback(RenderLoop, instance);
  int32_t swap_result;
  swap_result = PPBGraphics3D->SwapBuffers(instance->graphics3d_id, cc);
}


// Instance entrypoints ------------------------------------------------------

InstanceInfo global;

PP_Bool Instance_DidCreate(PP_Instance instance_id,
                           uint32_t argc,
                           const char* argn[],
                           const char* argv[]) {
  // Only support one instance at a time.
  assert(global.instance_id == kInvalidInstance);
  global.instance_id = instance_id;
  global.graphics3d_id = kInvalidResource;
  // Request the event types that this instance will recieve.
  PPBInputEvent->RequestInputEvents(instance_id,
      PP_INPUTEVENT_CLASS_MOUSE | PP_INPUTEVENT_CLASS_KEYBOARD);
  // Create audio & register application audio callback.
  global.sample_rate = PPBAudioConfig->RecommendSampleRate(instance_id);
  // If RecommendSampleRate was unable to determine a sample rate, assume 48kHz.
  if (global.sample_rate == PP_AUDIOSAMPLERATE_NONE)
    global.sample_rate = PP_AUDIOSAMPLERATE_48000;
  // Request a 10ms sample frame count; use what is recommended.
  global.sample_frame_count = PPBAudioConfig->RecommendSampleFrameCount(
      instance_id, global.sample_rate, global.sample_rate / 100);
  // Create the audio device and start playback to trigger AudioCallback.
  // Note: The audio callback to fill the next buffer occurs on its own
  // dedicated thread.
  global.audio_config_id = PPBAudioConfig->CreateStereo16Bit(instance_id,
      global.sample_rate, global.sample_frame_count);
  global.audio_id = PPBAudio->Create(instance_id, global.audio_config_id,
      AudioCallback, &global);
  PPBAudio->StartPlayback(global.audio_id);
  return PP_TRUE;
}

void Instance_DidDestroy(PP_Instance instance_id) {
  assert(instance_id == global.instance_id);
  if (global.graphics3d_id != kInvalidResource) {
    PPBCore->ReleaseResource(global.graphics3d_id);
    global.graphics3d_id = kInvalidResource;
  }
  if (global.audio_id != kInvalidResource) {
    PPBAudio->StopPlayback(global.audio_id);
    PPBCore->ReleaseResource(global.audio_id);
    global.audio_id = kInvalidResource;
  }
  if (global.audio_config_id != kInvalidResource) {
    PPBCore->ReleaseResource(global.audio_config_id);
    global.audio_config_id = kInvalidResource;
  }
  global.instance_id = kInvalidInstance;
}

void Instance_DidChangeView(PP_Instance instance_id,
                            PP_Resource view) {
  PP_Rect position;
  PP_Rect clip;
  PPBView->GetRect(view, &position);
  PPBView->GetClipRect(view, &clip);

  assert(instance_id == global.instance_id);
  // On first call, create graphics3d resource.
  if (global.graphics3d_id == kInvalidResource) {
    int32_t attribs[] = {
        PP_GRAPHICS3DATTRIB_WIDTH, position.size.width,
        PP_GRAPHICS3DATTRIB_HEIGHT, position.size.height,
        PP_GRAPHICS3DATTRIB_DEPTH_SIZE, 24,
        PP_GRAPHICS3DATTRIB_NONE};
    global.graphics3d_id =
        PPBGraphics3D->Create(instance_id, kInvalidResource, attribs);
    int32_t success =
        PPBInstance->BindGraphics(instance_id, global.graphics3d_id);
    if (success == PP_TRUE) {
      PP_CompletionCallback cc = PP_MakeCompletionCallback(RenderLoop, &global);
      PPBCore->CallOnMainThread(0, cc, PP_OK);
    }
  } else if (position.size.width != global.width ||
             position.size.height != global.height) {
    PPBGraphics3D->ResizeBuffers(global.graphics3d_id, position.size.width,
                                                       position.size.height);
  }
  global.width = position.size.width;
  global.height = position.size.height;
  // Pending completion callbacks actually occur here after exit.
}

void Instance_DidChangeFocus(PP_Instance instance_id,
                             PP_Bool has_focus) {
}

PP_Bool Instance_HandleDocumentLoad(PP_Instance instance_id,
                                    PP_Resource pp_url_loader) {
  return PP_FALSE;
}

// InputEvent entrypoints ------------------------------------------------------

PP_Bool InputEvent_HandleEvent(PP_Instance instance_id, PP_Resource input_event) {
  // Handle events here...
  assert(instance_id == global.instance_id);
  PP_InputEvent_Type type = PPBInputEvent->GetType(input_event);
  switch (type) {
    case PP_INPUTEVENT_TYPE_MOUSEMOVE:
      PP_Point point;
      point = PPBMouseInputEvent->GetPosition(input_event);
      global.cursor_x = point.x;
      global.cursor_y = point.y;
      break;
    case PP_INPUTEVENT_TYPE_MOUSEDOWN:
    case PP_INPUTEVENT_TYPE_KEYDOWN:
      global.keys_pressed++;
      break;
    case PP_INPUTEVENT_TYPE_MOUSEUP:
    case PP_INPUTEVENT_TYPE_KEYUP:
      global.keys_pressed--;
      break;
    default:
      break;
  }
  return PP_TRUE;
}


}  // namespace

// Global entrypoints --------------------------------------------------------

PP_EXPORT int32_t PPP_InitializeModule(PP_Module module,
                                       PPB_GetInterface get_browser_interface) {
  if (!get_browser_interface)
    return -1;

  memset(&global, 0, sizeof(global));
  global.instance_id = kInvalidInstance;
  global.browser_interface = get_browser_interface;

  PPBAudio = (const PPB_Audio*)
    get_browser_interface(PPB_AUDIO_INTERFACE);
  PPBAudioConfig = (const PPB_AudioConfig*)
    get_browser_interface(PPB_AUDIO_CONFIG_INTERFACE);
  PPBCore = (const PPB_Core*)
    get_browser_interface(PPB_CORE_INTERFACE);
  PPBInstance = (const PPB_Instance*)
    get_browser_interface(PPB_INSTANCE_INTERFACE);
  PPBOpenGLES2 = (const PPB_OpenGLES2*)
    get_browser_interface(PPB_OPENGLES2_INTERFACE);
  PPBInputEvent = (const PPB_InputEvent*)
    get_browser_interface(PPB_INPUT_EVENT_INTERFACE);
  PPBGraphics3D = (const PPB_Graphics3D*)
    get_browser_interface(PPB_GRAPHICS_3D_INTERFACE);
  PPBMouseInputEvent = (const PPB_MouseInputEvent*)
    get_browser_interface(PPB_MOUSE_INPUT_EVENT_INTERFACE);
  PPBView = (const PPB_View*)
    get_browser_interface(PPB_VIEW_INTERFACE);
  if (!PPBAudio ||
      !PPBAudioConfig ||
      !PPBCore ||
      !PPBInputEvent ||
      !PPBInstance ||
      !PPBOpenGLES2 ||
      !PPBGraphics3D ||
      !PPBMouseInputEvent ||
      !PPBView)
    return -1;
  // Initialize gl w/ browser interface
  if (!glInitializePPAPI(get_browser_interface)) {
    return -1;
  }
  return PP_OK;
}

PP_EXPORT void PPP_ShutdownModule() {
  glTerminatePPAPI();
}

PP_EXPORT const void* PPP_GetInterface(const char* interface_name) {
  static PPP_Instance instance_interface = {
    &Instance_DidCreate,
    &Instance_DidDestroy,
    &Instance_DidChangeView,
    &Instance_DidChangeFocus,
    &Instance_HandleDocumentLoad
  };
  static PPP_InputEvent input_event_interface = {
    &InputEvent_HandleEvent
  };
  if (strcmp(interface_name, PPP_INSTANCE_INTERFACE) == 0)
    return &instance_interface;
  if (strcmp(interface_name, PPP_INPUT_EVENT_INTERFACE) == 0)
    return &input_event_interface;
  return NULL;
}

