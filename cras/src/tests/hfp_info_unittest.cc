/* Copyright (c) 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <gtest/gtest.h>
#include <stdint.h>
#include <time.h>

extern "C" {
  #include "cras_hfp_info.c"
}

static struct hfp_info *info;
static struct cras_iodev dev;
static cras_audio_format format;

void ResetStubData() {
  format.format = SND_PCM_FORMAT_S16_LE;
  format.num_channels = 1;
  format.frame_rate = 8000;
  dev.format = &format;
}

namespace {

TEST(HfpInfo, AddRmDev) {
  info = hfp_info_create();
  ASSERT_NE(info, (void *)NULL);
  dev.direction = CRAS_STREAM_OUTPUT;

  /* Test add dev */
  ASSERT_EQ(0, hfp_info_add_iodev(info, &dev));
  ASSERT_TRUE(hfp_info_has_iodev(info));

  /* Test remove dev */
  ASSERT_EQ(0, hfp_info_rm_iodev(info, &dev));
  ASSERT_FALSE(hfp_info_has_iodev(info));

  hfp_info_destroy(info);
}

TEST(HfpInfo, AddRmDevInvalid) {
  info = hfp_info_create();
  ASSERT_NE(info, (void *)NULL);

  dev.direction = CRAS_STREAM_OUTPUT;

  /* Remove an iodev which doesn't exist */
  ASSERT_NE(0, hfp_info_rm_iodev(info, &dev));

  /* Adding an iodev twice returns error code */
  ASSERT_EQ(0, hfp_info_add_iodev(info, &dev));
  ASSERT_NE(0, hfp_info_add_iodev(info, &dev));

  hfp_info_destroy(info);
}

TEST(HfpInfo, AcquirePlaybackBuffer) {
  unsigned buffer_frames, buffer_frames2, queued;
  uint8_t *samples;

  ResetStubData();

  info = hfp_info_create();
  ASSERT_NE(info, (void *)NULL);

  dev.direction = CRAS_STREAM_OUTPUT;
  ASSERT_EQ(0, hfp_info_add_iodev(info, &dev));

  buffer_frames = 500;
  hfp_buf_acquire(info, &dev, &samples, &buffer_frames);
  ASSERT_EQ(500, buffer_frames);

  hfp_buf_release(info, &dev, 500);
  ASSERT_EQ(500, hfp_buf_queued(info, &dev));

  /* Assert the amount of frames of available buffer + queued buf equals
   * the buffer size, 2 bytes per frame */
  queued = hfp_buf_queued(info, &dev);
  buffer_frames = 500;
  hfp_buf_acquire(info, &dev, &samples, &buffer_frames);
  ASSERT_EQ(HFP_BUF_SIZE_BYTES / 2, buffer_frames + queued);

  /* Consume all queued data from read buffer */
  put_read_buf_bytes(info->playback_buf, queued * 2);

  queued = hfp_buf_queued(info, &dev);
  ASSERT_EQ(0, queued);

  /* Assert consecutive acquire buffer will acquire full used size of buffer */
  buffer_frames = 500;
  hfp_buf_acquire(info, &dev, &samples, &buffer_frames);
  hfp_buf_release(info, &dev, buffer_frames);

  buffer_frames2 = 500;
  hfp_buf_acquire(info, &dev, &samples, &buffer_frames2);
  hfp_buf_release(info, &dev, buffer_frames2);

  ASSERT_EQ(HFP_BUF_SIZE_BYTES / 2, buffer_frames + buffer_frames2);

  hfp_info_destroy(info);
}

TEST(HfpInfo, AcquireCaptureBuffer) {
  unsigned buffer_frames, buffer_frames2;
  uint8_t *samples;

  ResetStubData();

  info = hfp_info_create();
  ASSERT_NE(info, (void *)NULL);

  dev.direction = CRAS_STREAM_INPUT;
  ASSERT_EQ(0, hfp_info_add_iodev(info, &dev));

  /* Put fake data 100 bytes(50 frames) in capture buf for test */
  put_write_buf_bytes(info->capture_buf, 100);

  /* Assert successfully acquire and release 100 bytes of data */
  buffer_frames = 50;
  hfp_buf_acquire(info, &dev, &samples, &buffer_frames);
  ASSERT_EQ(50, buffer_frames);

  hfp_buf_release(info, &dev, buffer_frames);
  ASSERT_EQ(0, hfp_buf_queued(info, &dev));

  /* Push fake data to capture buffer */
  put_write_buf_bytes(info->capture_buf, HFP_BUF_SIZE_BYTES - 100);
  put_write_buf_bytes(info->capture_buf, 100);

  /* Assert consecutive acquire call will consume the whole buffer */
  buffer_frames = 500;
  hfp_buf_acquire(info, &dev, &samples, &buffer_frames);
  hfp_buf_release(info, &dev, buffer_frames);
  ASSERT_GT(500, buffer_frames);

  buffer_frames2 = 500;
  hfp_buf_acquire(info, &dev, &samples, &buffer_frames2);
  hfp_buf_release(info, &dev, buffer_frames2);

  ASSERT_EQ(HFP_BUF_SIZE_BYTES / 2, buffer_frames + buffer_frames2);

  hfp_info_destroy(info);
}

} // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
