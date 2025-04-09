/*
 * Copyright (C) 2025 OWL Services LLC. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
 * @file sdll.h
 *
 * @brief Simple Data Link Layer (SimpleDLL) API.
 */

#ifndef SIMPLEDLL_SIMPLEDLL_H__
#define SIMPLEDLL_SIMPLEDLL_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief SimpleDLL frame received callback
 *
 * This function is called when a frame is received and validated in case
 * `frame_check()` is not NULL, else it is called when a frame is received.
 *
 * @param data Pointer to the received data
 * @param len Length of the received data
 */
typedef void (*sdll_frame_received_callback_t)(const uint8_t * data, const size_t len);

/**
 * @brief SimpleDLL frame validation function
 *
 * This function is called after a frame is completely received and before
 * calling `frame_received()`. It is used to validate the content of the frame
 * (i.e. a checksum or CRC-16 check).
 *
 * @param data Pointer to the received data
 * @param len Length of the received data
 * @return true if the frame is valid, false otherwise
  */
typedef bool (*sdll_frame_validation_check_function_t)(const uint8_t * data, const
size_t len);

/**
 * @brief SimpleDLL frame send function
 *
 * This function is called to send a frame.
 *
 * @param data Pointer to the data to send
 * @param len Length of the data to send
 * @return Number of bytes sent
 * @return -EINVAL if one or more parameters are invalid
 */
typedef int (*sdll_frame_send_function_t) (const uint8_t * data,
    const size_t len);

/**
 * @brief SimpleDLL frame sent callback
 *
 * This function is called when a frame is completely sent.
 *
 * @param data Pointer to the sent data
 * @param len Length of the sent data
 */
typedef void (*sdll_frame_sent_callback_t) (const uint8_t * data, const size_t len);

/**
 * @brief SimpleDLL configuration
 *
 * This structure is used to configure the SimpleDLL library on initialization.
 */
struct sdll_config {
    struct {
        uint8_t * ptr;
        size_t len;
    } receive_buffer;
    struct {
        uint8_t * ptr;
        size_t len;
    } send_buffer;
    struct {
      sdll_frame_received_callback_t frame_received;
      sdll_frame_validation_check_function_t frame_check;
      sdll_frame_send_function_t frame_send;
      sdll_frame_sent_callback_t frame_sent;
    } callbacks;
};

/**
 * @brief SimpleDLL context
 */
struct sdll_context;

/**
 * @brief SimpleDLL initialization function
 *
 * This function initializes the SimpleDLL library.
 *
 * @param cfg SimpleDLL configuration
 * @param ctx Pointer to initialize SimpleDLL context
 * @return 0 if the initialization is successful
 * @return -EINVAL if one or more parameters are invalid
 * @return -ENOMEM if context allocation fails
 */
int sdll_init(struct sdll_config * cfg, struct sdll_context ** ctx);

/**
 * @brief SimpleDLL frame reception function
 *
 * This function must be called when a new frame is received.
 *
 * The function blocks until the buffer is completely procesed, also the
 * validatation function and frame received callbacks are called in this
 * context.
 *
 * @param ctx SimpleDLL context
 * @param buffer Buffer with received data
 * @param len Length of the buffer
 * @return Number of bytes processed.
 * @return -EINVAL if one or more parameters are invalid.
 * @return -ENOMEM if the received frame is too big for the buffer.
 */
int sdll_receive(struct sdll_context * ctx, const uint8_t * buffer, const size_t len);

/** @todo Add related configs to the FHD */
/** @todo Consider providing the callback in this function instead of config */
/**
 * @brief SimpleDLL frame reception function
 *
 * This function must be called when a new frame is received.
 *
 * The function does not block and returns immediately after the buffer is
 * enqueued to be processed by the system workqueue or a dedicated thread
 * (configurable).
 *
 * @param ctx SimpleDLL context
 * @param buffer Buffer with received data
 * @param len Length of the buffer
 * @return Number of bytes processed.
 * @return -EINVAL if one or more parameters are invalid.
 * @return -ENOMEM if the received frame is too big for the buffer.
 */
int sdll_receive_async(struct sdll_context * ctx, const uint8_t * buffer, const size_t len);

/** @todo Consider providing the callback in this function instead of config */
/**
 * @brief SimpleDLL frame send function
 *
 * This function sends a frame by calling the provided `frame_send` function. If
 * a `frame_sent` callback is provided, it will not be called here.
 *
 * The function blocks until the frame is completely sent.
 *
 * @param ctx SimpleDLL context
 * @param buffer Buffer with data to send
 * @param len Length of the buffer
 * @return Number of bytes sent.
 * @return -EINVAL if one or more parameters are invalid
 */
int sdll_send(struct sdll_context * ctx, const uint8_t * buffer, const size_t len);

/** @todo Add related configs to the FHD */
/** @todo Consider providing the callback in this function instead of config */
/**
 * @brief SimpleDLL frame send function
 *
 * This function sends a frame by calling the provided frame send function.
 *
 * The function does not block and returns immediately after the buffer is
 * enqueued to be sent by the system workqueue or a dedicated thread
 * (configurable). Provided callback for `frame_sent` is called in this context.
 *
 * @param ctx SimpleDLL context
 * @param buffer Buffer with data to send
 * @param len Length of the buffer
 * @return Number of bytes sent.
 * @return -EINVAL if one or more parameters are invalid
 */
int sdll_send_async(struct sdll_context * ctx, const uint8_t * buffer, const size_t len);

#endif /* SIMPLEDLL_SIMPLEDLL_H__ */