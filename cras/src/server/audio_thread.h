/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef AUDIO_THREAD_H_
#define AUDIO_THREAD_H_

#include <pthread.h>
#include <stdint.h>

#include "cras_types.h"

struct buffer_share;
struct cras_iodev;
struct cras_rstream;
struct dev_stream;

/* Open input/output devices.
 *    dev - The device.
 *    wake_ts - When callback is needed to avoid xrun.
 *    coarse_rate_adjust - Hack for when the sample rate needs heavy correction.
 *    input_streaming - For capture, has the input started?
 */
struct open_dev {
	struct cras_iodev *dev;
	struct timespec wake_ts;
	int coarse_rate_adjust;
	int for_pinned_streams;
	int input_streaming;
	struct open_dev *prev, *next;
};

/* Hold communication pipes and pthread info for the thread used to play or
 * record audio.
 *    to_thread_fds - Send a message from main to running thread.
 *    to_main_fds - Send a synchronous response to main from running thread.
 *    tid - Thread ID of the running playback/capture thread.
 *    started - Non-zero if the thread has started successfully.
 *    suspended - Non-zero if the thread is suspended.
 *    open_devs - Lists of open input and output devices.
 *    loopback_devs - Keep loopback input and output devices (loopback_iodev).
 */
struct audio_thread {
	int to_thread_fds[2];
	int to_main_fds[2];
	pthread_t tid;
	int started;
	int suspended;
	struct open_dev *open_devs[CRAS_NUM_DIRECTIONS];
	struct cras_iodev *loopback_devs[CRAS_NUM_DIRECTIONS];

};

/* Callback function to be handled in main loop in audio thread.
 * Args:
 *    data - The data for callback function.
 */
typedef int (*thread_callback)(void *data);

/* Creates an audio thread.
 * Args:
 *    loopback_output - A device that keeps track of what the system is playing.
 *    loopback_input - A device to record what the system is playing.
 * Returns:
 *    A pointer to the newly create audio thread.  It must be freed by calling
 *    audio_thread_destroy().  Returns NULL on error.
 */
struct audio_thread *audio_thread_create(struct cras_iodev *loopback_output,
					 struct cras_iodev *loopback_input);

/* Adds an open device.
 * Args:
 *    thread - The thread to add open device to.
 *    dev - The open device to add.
 */
int audio_thread_add_open_dev(struct audio_thread *thread,
			      struct cras_iodev *dev);

/* Removes an open device.
 * Args:
 *    thread - The thread to remove open device from.
 *    dev - The open device to remove.
 *    is_device_removal - True if the I/O device is being removed.
 */
int audio_thread_rm_open_dev(struct audio_thread *thread,
			     struct cras_iodev *dev,
			     int is_device_removal);

/* Adds an thread_callback to audio thread.
 * Args:
 *    fd - The file descriptor to be polled for the callback.
 *      The callback will be called when fd is readable.
 *    cb - The callback function.
 *    data - The data for the callback function.
 */
void audio_thread_add_callback(int fd, thread_callback cb,
                               void *data);

/* Adds an thread_callback to audio thread.
 * Args:
 *    fd - The file descriptor to be polled for the callback.
 *      The callback will be called when fd is writeable.
 *    cb - The callback function.
 *    data - The data for the callback function.
 */
void audio_thread_add_write_callback(int fd, thread_callback cb,
				     void *data);

/* Removes an thread_callback from audio thread.
 * Args:
 *    fd - The file descriptor of the previous added callback.
 */
void audio_thread_rm_callback(int fd);

/* Enables or Disabled the callback associated with fd. */
void audio_thread_enable_callback(int fd, int enabled);

/* Starts a thread created with audio_thread_create.
 * Args:
 *    thread - The thread to start.
 * Returns:
 *    0 on success, return code from pthread_crate on failure.
 */
int audio_thread_start(struct audio_thread *thread);

/* Frees an audio thread created with audio_thread_create(). */
void audio_thread_destroy(struct audio_thread *thread);

/* Add a stream to the thread. After this call, the ownership of the stream will
 * be passed to the audio thread. Audio thread is responsible to release the
 * stream's resources.
 * Args:
 *    thread - a pointer to the audio thread.
 *    stream - the new stream to add.
 *    dev - device to attach stream. NULL to attach to all the default devices.
 * Returns:
 *    zero on success, negative error from the AUDIO_THREAD enum above when an
 *    the thread can't be added.
 */
int audio_thread_add_stream(struct audio_thread *thread,
			    struct cras_rstream *stream,
			    struct cras_iodev *dev);

/* Begin draining a stream and check the draining status.
 * Args:
 *    thread - a pointer to the audio thread.
 *    stream - the stream to drain/remove.
 * Returns:
 *    zero if the stream is drained and can be deleted.  If the stream is not
 *    completely drained, then the number of milliseconds until is is drained
 *    are returned.
 */
int audio_thread_drain_stream(struct audio_thread *thread,
			      struct cras_rstream *stream);

/* Disconnect a stream from the client.
 * Args:
 *    thread - a pointer to the audio thread.
 *    stream - the stream to be disconnected.
 *    iodev - the device to disconnect from.
 * Returns:
 *    0 on success, negative if error.
 */
int audio_thread_disconnect_stream(struct audio_thread *thread,
				   struct cras_rstream *stream,
				   struct cras_iodev *iodev);

/* Dumps information about all active streams to syslog. */
int audio_thread_dump_thread_info(struct audio_thread *thread,
				  struct audio_debug_info *info);

#endif /* AUDIO_THREAD_H_ */
