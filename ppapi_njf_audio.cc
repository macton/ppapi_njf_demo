// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <GLES2/gl2.h>
#include <stdio.h>
#include <unistd.h>


#include "ppapi_njf_demo.h"

int16_t TriangleWave(int64_t counter, int32_t rate, int32_t freq, float amplitude) {
  int32_t invfreq = rate / freq;
  float range = 4.0f * (counter % invfreq) / float(invfreq);
  float toint16 = 32767.0f;
  float wave;
  if (range > 3.0f)
    wave = range - 4.0f;
  else if (range > 1.0f)
    wave = 2.0f - range;
  else wave = range;
  wave = wave * amplitude * toint16;
  if (wave > toint16)
    wave = toint16;
  else if (wave < -toint16)
    wave = -toint16;
  return int16_t(wave);
}

// The AudioCallback function will be invoked periodically once StartPlayback()
// has been called.  It is important to fill the buffer quickly, as audio
// playback involves meeting realtime requirements.  Avoid invoking library
// calls that could cause the os scheduler to swap this thread out, such as
// mutex locks.
void AudioCallback(void* sample_buffer,
                   uint32_t buffer_size_in_bytes,
                   void* user_data) {
  InstanceInfo* info = static_cast<InstanceInfo*>(user_data);
  int16_t* stereo16 = static_cast<int16_t*>(sample_buffer);
  static int16_t sample = 0;
  static int64_t counter = 0;
  for (int32_t i = 0; i < info->sample_frame_count; i++) {
    if (info->keys_pressed)
      sample = TriangleWave(counter, info->sample_rate, 440, 0.5f);
    stereo16[i * 2 + 0] = sample;  // Left channel
    stereo16[i * 2 + 1] = sample;  // Right channel
    counter++;
  }
}

