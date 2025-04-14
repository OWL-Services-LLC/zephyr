/*
 * Copyright (C) 2025 OWL Services LLC. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
 * @file sdll.h
 *
 * @brief Simple Data Link Layer (SDLL) API.
 */

#ifndef SDLL_SDLL_H__
#define SDLL_SDLL_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief SDLL Context ID
 */
typedef uint8_t sdll_context_id;

/**
 * @brief SDLL frame received callback
 *
 * This function is called when a frame is received and validated in case
 * `frame_check()` is not NULL, else it is called when a frame is received.
 *
 * @param cid SDLL context id
 * @param data Pointer to the received data
 * @param len Length of the received data
 */
typedef void (*sdll_frame_received_callback_t)(const sdll_context_id cid, const uint8_t * data, const size_t len);

/**
 * @brief SDLL frame validation function
 *
 * This function is called after a frame is completely received and before
 * calling `frame_received()`. It is used to validate the content of the frame
 * (i.e. a checksum or CRC-16 check).
 *
 * @param data Pointer to the received data
 * @param len Length of the received data
 * @return true if the frame is valid, false otherwise
  */
typedef bool (*sdll_frame_validation_check_function_t)(const sdll_context_id cid, const uint8_t * data, const
size_t len);

/**
 * @brief SDLL frame send function
 *
 * This function is called to send a frame.
 *
 * @param data Pointer to the data to send
 * @param len Length of the data to send
 * @return Number of bytes sent
 * @return -EINVAL if one or more parameters are invalid
 */
typedef int (*sdll_frame_send_function_t) (const sdll_context_id cid, const uint8_t * data,
    const size_t len);

/**
 * @brief SDLL frame sent callback
 *
 * This function is called when a frame is completely sent.
 *
 * @param data Pointer to the sent data
 * @param len Length of the sent data
 */
typedef void (*sdll_frame_sent_callback_t) (const sdll_context_id cid, const uint8_t * data, const size_t len);

/**
 * @brief SDLL configuration
 *
 * This structure is used to configure the SDLL frame receiver
 * on initialization.
 */
struct sdll_receiver_config {

    /** @brief Buffer to store frames received */
    uint8_t * receive_buffer;

    /** @brief Length of the buffer. Must be at least
     * `SDLL_MINIMUM_BUFFER_SIZE` bytes */
    size_t receive_buffer_len;

    /** @brief Frame received callback */
    sdll_frame_received_callback_t frame_received_cb;

    /** @brief Frame check function */
    sdll_frame_validation_check_function_t frame_check_fn;
};

/**
 * @brief SDLL configuration
 *
 * This structure is used to configure the SDLL frame transmitter
 * on initialization.
 */
struct sdll_transmitter_config {

    /** @brief Buffer to store frames to be sent */
    uint8_t * send_buffer;

    /** @brief Length of the buffer. Must be at least
     * `SDLL_MINIMUM_BUFFER_SIZE` bytes */
    size_t send_buffer_len;

    /** @brief Frame send function */
    sdll_frame_send_function_t frame_send_fn;

#ifdef CONFIG_SDLL_ASYNC
    /** @brief Frame sent callback */
    sdll_frame_sent_callback_t frame_sent_cb;
#endif /* CONFIG_SDLL_ASYNC */
};

/**
 * @brief SDLL initialization function
 *
 * This function initializes the SDLL library and configures both the transmitter
 * stage and/or the receiver stage if provided.
 *
 * @note:
 * The buffer size must be at least `SDLL_MINIMUM_BUFFER_SIZE` bytes.
 *
 * @param rxcfg SDLL receiver configuration (NULL disables receiver)
 * @param txcfg SDLL transmitter configuration (NULL disables transmitter)
 * @return The initialized context id on success.
 * @return -EINVAL if one or more parameters are invalid
 * @return -ENOMEM if context allocation fails
 */
int sdll_init(const struct sdll_receiver_config * rxcfg, const struct sdll_transmitter_config * txcfg);

/**
 * @brief SDLL deinitialization function
 *
 * This function deinitializes the SDLL library.
 *
 * @param context_id The SDLL context id.
 * @return 0 if the deinitialization is successful.
 * @return -EINVAL if `context_id` is invalid.
 */
int sdll_deinit(const sdll_context_id context_id);

/**
 * @brief SDLL frame reception function
 *
 * This function must be called when a new frame is received.
 *
 * The function blocks until the buffer is completely procesed, also the
 * validatation function and frame received callbacks are called in this
 * context.
 *
 * @param ctx SDLL context id
 * @param buffer Buffer with received data
 * @param len Length of the buffer
 * @return 0 if all bytes were processed.
 * @return -EINVAL if one or more parameters are invalid.
 * @return -ENOMEM if the received frame is too big for the buffer.
 */
int sdll_receive(const sdll_context_id cid, const uint8_t * buffer, const size_t len);

/**
 * @brief SDLL frame send function
 *
 * This function sends a frame by calling the provided `frame_send` function.
 *
 * Function waits for the `frame_send` function to finish before returning.
 *
 * @param cid SDLL context id
 * @param buffer Buffer with data to send
 * @param len Length of the buffer
 * @return Number of bytes sent.
 * @return -EINVAL if one or more parameters are invalid
 * @return -EPERM if the transmitter is disabled
 * @return -ENOBUFS if the frame is too big for the buffer
 * @return -EIO if the frame send function fails to send the frame
 */
int sdll_send(const sdll_context_id cid, const uint8_t * buffer, const size_t len);

#ifdef CONFIG_SDLL_ASYNC

/** @todo Add related configs to the FHD */
/** @todo Consider providing the callback in this function instead of config */
/**
 * @brief SDLL frame reception function
 *
 * This function must be called when a new frame is received.
 *
 * The function does not block and returns immediately after the buffer is
 * enqueued to be processed by the system workqueue or a dedicated thread
 * (configurable).
 *
 * @param cid SDLL context id
 * @param buffer Buffer with received data
 * @param len Length of the buffer
 * @return Number of bytes processed.
 * @return -EINVAL if one or more parameters are invalid.
 * @return -ENOMEM if the received frame is too big for the buffer.
 */
int sdll_receive_async(const sdll_context_id cid, const uint8_t * buffer, const size_t len);


/** @todo Add related configs to the FHD */
/** @todo Consider providing the callback in this function instead of config */
/**
 * @brief SDLL frame send function
 *
 * This function sends a frame by calling the provided frame send function.
 *
 * The function does not block and returns immediately after the buffer is
 * enqueued to be sent by the system workqueue or a dedicated thread
 * (configurable). Provided callback for `frame_sent` is called in this context.
 *
 * @param cid SDLL context id
 * @param buffer Buffer with data to send
 * @param len Length of the buffer
 * @return Number of bytes sent.
 * @return -EINVAL if one or more parameters are invalid
 */
int sdll_send_async(const sdll_context_id cid, const uint8_t * buffer, const size_t len);

#endif /* CONFIG_SDLL_ASYNC */

#endif /* SDLL_SDLL_H__ */