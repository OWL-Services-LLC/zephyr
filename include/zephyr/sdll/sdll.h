/*
 * Copyright (C) 2025 OWL Services LLC. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SDLL_SDLL_H_
#define ZEPHYR_INCLUDE_SDLL_SDLL_H_

/**
 * @brief Simple Data Link Layer (SDLL)
 * @defgroup sdll_api Simple Data Link Layer (SDLL) API
 * @version 1.0.0
 * @ingroup utilities
 * @{
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SDLL minimal buffer size
 *
 * This is the minimum size of the buffer used to store received or sent frames.
 * The size must be at least 4 bytes to store 1 byte of data and 3 bytes of
 * overhead in the worst case (boundaries and escape chars).
 */
#define SDLL_SEND_BUFFER_SIZE_MIN 4U

/**
 * @brief SDLL Context ID
 */
typedef unsigned int sdll_context_id;

/**
 * @brief SDLL frame received callback
 *
 * This function is called when a frame is received and validated in case
 * `frame_check()` is not NULL, otherwise it is called when a frame is received.
 *
 * @param cid SDLL context id
 * @param data Pointer to the received data
 * @param len Length of the received data
 */
typedef void (*sdll_frame_received_cb_t)(const sdll_context_id cid,
	                                     const uint8_t *data,
                                         const size_t len);

/**
 * @brief SDLL frame validator function
 *
 * This function is called after a frame is completely received and before
 * calling `frame_received()`. It is used to validate the content of the frame
 * (i.e. a checksum or CRC-16 check).
 *
 * @param cid SDLL context id
 * @param data Pointer to the received data
 * @param len Length of the received data
 * @return true if the frame is valid, false otherwise
 */
typedef bool (*sdll_frame_validator_t)(const sdll_context_id cid,
						               const uint8_t *data,
									   const size_t len);

/**
 * @brief SDLL frame sender function
 *
 * This function is called to send a frame to the transport layer. The function
 * must return the number of bytes sent or a negative value in case of error.
 *
 * @param data Pointer to the data to send
 * @param len Length of the data to send
 * @return Number of bytes sent
 * @return -EINVAL if one or more parameters are invalid
 */
typedef int (*sdll_frame_sender_t)(const sdll_context_id cid,
	                               const uint8_t *data,
					               const size_t len);

/**
 * @struct sdll_receiver_config
 * @brief SDLL receiver configuration
 *
 * This structure is used to configure the SDLL frame receiver
 * on initialization.
 */
struct sdll_receiver_config {

	/** @brief Buffer to store received frames */
	uint8_t *receive_buffer;

	/** @brief Length of the buffer. Must be at least
	 * `SDLL_SEND_BUFFER_SIZE_MIN` bytes */
	size_t receive_buffer_len;

	/** @brief Received frame callback */
	sdll_frame_received_cb_t frame_received_cb;

	/** @brief Frame check function */
	sdll_frame_validator_t frame_check_fn;
};

/**
 * @struct sdll_transmitter_config
 * @brief SDLL transmitter configuration
 *
 * This structure is used to configure the SDLL frame transmitter
 * on initialization.
 */
struct sdll_transmitter_config {

	/** @brief Buffer to store frames to be sent */
	uint8_t *send_buffer;

	/** @brief Length of the buffer. Must be at least
	 * `SDLL_SEND_BUFFER_SIZE_MIN` bytes */
	size_t send_buffer_len;

	/** @brief Frame send function */
	sdll_frame_sender_t frame_send_fn;
};

/**
 * @brief SDLL initialization function
 *
 * This function initializes the SDLL library and configures both the
 * transmitter stage and/or the receiver stage if provided.
 *
 * @pre Buffers to store received or packed frames must provided by the
 * application inside the configuration structures.
 * @pre The send buffer size must be at least `SDLL_SEND_BUFFER_SIZE_MIN` bytes.
 *
 * @param rxcfg SDLL receiver configuration (NULL disables receiver)
 * @param txcfg SDLL transmitter configuration (NULL disables transmitter)
 * @return The initialized context id on success.
 * @return -EINVAL if one or more parameters are invalid
 * @return -ENOMEM if context allocation fails
 */
int sdll_init(const struct sdll_receiver_config *rxcfg,
	          const struct sdll_transmitter_config *txcfg);

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
 * This function must be called when new data is received. Data is treated as a
 * stream of bytes that may contain part of a frame, a complete frame or more
 * than one frame. The processed bytes are stored in the receive buffer.
 *
 * When a complete frame is received, the function will first call the
 * `frame_check` function if provided to validate the frame and then call the
 * `frame_received` callback if the validation is successful.
 *
 * The function blocks until the data is completely procesed. validatation
 * function and frame received callbacks are called in this context.
 *
 * @param cid SDLL context id
 * @param data Data received
 * @param len Length of the received data
 * @return Number of bytes in the receive buffer
 * @return -EPERM when the receiver is disabled
 * @return -EINVAL when one or more parameters are invalid
 * @return -ENOMEM when the received frame is too big for the receiver buffer
 * @return -EAGAIN when `CONFIG_SDLL_THREAD_SAFE` is enabled and the mutex is
 *         not available
 */
int sdll_receive(const sdll_context_id cid,
	             const uint8_t *data,
				 const size_t len);

/**
 * @brief SDLL frame send function
 *
 * This function generates a frame `data` and calls the provided `frame_send`
 * function in the transmitter configuration to send the frame.
 *
 * Function waits for the `frame_send` function to finish before returning.
 *
 * @param cid SDLL context id
 * @param data Data to be sent
 * @param len Length of the buffer
 * @return Number of bytes sent.
 * @return -EINVAL when one or more parameters are invalid
 * @return -EPERM when the transmitter is disabled
 * @return -ENOBUFS when the frame is too big for the transmitter buffer
 * @return -EIO when the frame send function fails to send the frame (returns a
 * 			negative value)
 * @return -EAGAIN when `CONFIG_SDLL_THREAD_SAFE` is enabled and the mutex is
 *         not available.
 */
int sdll_send(const sdll_context_id cid,
	          const uint8_t *data,
			  const size_t len);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_SDLL_SDLL_H_ */
