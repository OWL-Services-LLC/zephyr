/*
 * Copyright (C) 2025 OWL Services LLC. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file sdll.c
 *
 * @brief Simple Data Link Layer (SDLL) API implementation
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sdll/sdll.h>

#define SDLL_STATUS_NEW_FRAME_BIT   (1U << 0)
#define SDLL_STATUS_ESCAPE_NEXT_BIT (1U << 1)

struct sdll_receiver_context {
	int status;
	size_t recv_frame_len;
	struct sdll_receiver_config cfg;
#ifdef CONFIG_SDLL_THREAD_SAFE
	struct k_mutex lock;
#endif /* CONFIG_SDLL_THREAD_SAFE */
};

struct sdll_transmitter_context {
	int status;
	size_t send_frame_len;
	struct sdll_transmitter_config cfg;
#ifdef CONFIG_SDLL_THREAD_SAFE
	struct k_mutex lock;
#endif /* CONFIG_SDLL_THREAD_SAFE */
};

struct sdll_context {
	bool in_use;
	struct sdll_receiver_context rx;
	struct sdll_transmitter_context tx;
};

static struct sdll_context sdll_instance[CONFIG_SDLL_MAX_INSTANCES];

LOG_MODULE_REGISTER(sdll, CONFIG_SDLL_LOG_LEVEL);

static inline bool context_is_valid(const sdll_context_id cid)
{
	return ((cid < CONFIG_SDLL_MAX_INSTANCES) && (sdll_instance[cid].in_use));
}

static inline void reset_receiver_context(struct sdll_receiver_context *rx)
{
	rx->recv_frame_len = 0;
	rx->status = 0;
}

static inline void reset_transmitter_context(struct sdll_transmitter_context *tx)
{
	tx->send_frame_len = 0;
	tx->status = 0;
}

static int build_frame(struct sdll_transmitter_context *tx, const uint8_t *payload,
		       const size_t paylaod_len)
{
	int status = 0;
	size_t nbytes = 0;

	/** @todo Check if this function can be optimized */

	tx->send_frame_len = 0;
	tx->status = 0;

	while (nbytes < paylaod_len) {
		/* check if there is enough space in the send buffer */
		if (tx->send_frame_len > tx->cfg.send_buffer_len) {
			status = -ENOBUFS;
			break;
		}

		/* add a boundary char on a new frame */

		if (!(tx->status & SDLL_STATUS_NEW_FRAME_BIT)) {
			tx->status |= SDLL_STATUS_NEW_FRAME_BIT;

			tx->cfg.send_buffer[0] = CONFIG_SDLL_BOUNDARY_CHAR;
			tx->send_frame_len++;
			continue;
		}

		/* check if the addition of a escape char is required */

		if (payload[nbytes] == CONFIG_SDLL_BOUNDARY_CHAR ||
		    payload[nbytes] == CONFIG_SDLL_ESCAPE_CHAR) {
			/* re-check if there is enough space */

			if (tx->send_frame_len + 1 > tx->cfg.send_buffer_len) {
				status = -ENOBUFS;
				break;
			}

			/* add escape character */

			tx->cfg.send_buffer[tx->send_frame_len] = CONFIG_SDLL_ESCAPE_CHAR;
			tx->send_frame_len++;

			/* add inverted data byte */

			tx->cfg.send_buffer[tx->send_frame_len] =
				payload[nbytes] ^ CONFIG_SDLL_ESCAPE_MASK;
			tx->send_frame_len++;
		} else {
			/* add data no escape char required */

			tx->cfg.send_buffer[tx->send_frame_len] = payload[nbytes];
			tx->send_frame_len++;
		}

		nbytes++;
	}

	if (nbytes == paylaod_len) {
		if (tx->send_frame_len + 1 > tx->cfg.send_buffer_len) {
			status = -ENOBUFS;
		} else {
			tx->cfg.send_buffer[tx->send_frame_len] = CONFIG_SDLL_BOUNDARY_CHAR;
			tx->send_frame_len++;
			status = nbytes;
		}
	}

	return status;
}

/**
 * @brief Receive a frame from the SDLL layer
 *
 * This function processes the incoming data and extracts the frame from it.
 * It handles the escape characters and boundary characters according to the SDLL protocol.
 *
 * @param cid The context ID of the SDLL instance.
 * @param payload The incoming data buffer.
 * @param paylaod_len The length of the incoming data buffer.
 * @param nbytes Pointer to store the number of bytes processed.
 * @return 0 on frame completed
 * @return -ENOMSG if no frame is completed
 * @return -ENOBUFS if the receive buffer is full
 */
static int receive_frame(struct sdll_receiver_context *rx,
						 const uint8_t *payload, const size_t paylaod_len,
						 size_t *bytes_processed)
{
	int status = -ENOMSG;
	size_t nbytes = 0;

	for (; nbytes < paylaod_len; nbytes++) {
		if (rx->recv_frame_len == rx->cfg.receive_buffer_len) {
			status = -ENOBUFS;
			break;
		}

		if (rx->status & SDLL_STATUS_NEW_FRAME_BIT) {
			if (payload[nbytes] == CONFIG_SDLL_BOUNDARY_CHAR) {
				/**
				 * Frame end reached, returns FRAME_DONE.
				 * Pending bytes are not processed in this iteration.
				 * @todo Check if this is the right behavior
				 */

				rx->status &= ~SDLL_STATUS_NEW_FRAME_BIT;
				status = 0;
				nbytes++; /* count boundary char */
				break;
			} else if (payload[nbytes] == CONFIG_SDLL_ESCAPE_CHAR) {
				/* set escape next bit */

				rx->status |= SDLL_STATUS_ESCAPE_NEXT_BIT;
			} else {
				if ((rx->status & SDLL_STATUS_ESCAPE_NEXT_BIT) ==
				    SDLL_STATUS_ESCAPE_NEXT_BIT) {
					/* add inverted data byte */

					rx->cfg.receive_buffer[rx->recv_frame_len] =
						payload[nbytes] ^ CONFIG_SDLL_ESCAPE_MASK;
					rx->recv_frame_len++;
					rx->status &= ~SDLL_STATUS_ESCAPE_NEXT_BIT;
				} else {
					/* add data byte */

					rx->cfg.receive_buffer[rx->recv_frame_len] =
						payload[nbytes];
					rx->recv_frame_len++;
				}
			}
		} else if (payload[nbytes] == CONFIG_SDLL_BOUNDARY_CHAR) {
			rx->status |= SDLL_STATUS_NEW_FRAME_BIT;
			rx->recv_frame_len = 0;
		} else {
			/* out of frame */
		}
	}

	*bytes_processed = nbytes;

	return status;
}

int sdll_init(const struct sdll_receiver_config *rxcfg,
	          const struct sdll_transmitter_config *txcfg)
{
	sdll_context_id new_cid = 0;

	if ((rxcfg == NULL) && (txcfg == NULL)) {
		LOG_ERR("At least one configuration (receiver/transmitter) must be provided");
		return -EINVAL;
	}

	if (rxcfg != NULL) {
		if (rxcfg->receive_buffer == NULL ||
		    rxcfg->receive_buffer_len == 0) {
			LOG_ERR("Invalid receiver buffer");
			return -EINVAL;
		} else if (rxcfg->frame_received_cb == NULL) {
			LOG_ERR("Invalid frame received callback");
			return -EINVAL;
		}
	}

	if (txcfg != NULL) {
		if (txcfg->send_buffer == NULL ||
		    txcfg->send_buffer_len < SDLL_SEND_BUFFER_SIZE_MIN) {
			LOG_ERR("Invalid transmitter buffer");
			return -EINVAL;
		} else if (txcfg->frame_send_fn == NULL) {
			LOG_ERR("Invalid frame send function");
			return -EINVAL;
		}
	}

	/* Find a free context */

	for (; new_cid < CONFIG_SDLL_MAX_INSTANCES; new_cid++) {
		if (!sdll_instance[new_cid].in_use) {
			break;
		}
	}

	if (new_cid == CONFIG_SDLL_MAX_INSTANCES) {
		return -ENOMEM;
	}

	/* Initialize context */

	sdll_instance[new_cid].in_use = true;

	if (rxcfg != NULL) {
		sdll_instance[new_cid].rx.cfg = *rxcfg;
#ifdef CONFIG_SDLL_THREAD_SAFE
		k_mutex_init(&sdll_instance[new_cid].rx.lock);
#endif /* CONFIG_SDLL_THREAD_SAFE */
	}

	if (txcfg != NULL) {
		sdll_instance[new_cid].tx.cfg = *txcfg;
#ifdef CONFIG_SDLL_THREAD_SAFE
		k_mutex_init(&sdll_instance[new_cid].tx.lock);
#endif /* CONFIG_SDLL_THREAD_SAFE */
	}

	/* Return new context id */

	return new_cid;
}

int sdll_deinit(const sdll_context_id cid)
{
	if (!context_is_valid(cid)) {
		return -EINVAL;
	}

	/* reset receiver context */

	if (sdll_instance[cid].rx.cfg.receive_buffer != NULL) {
#ifdef CONFIG_SDLL_THREAD_SAFE
		k_mutex_lock(&sdll_instance[cid].rx.lock, K_FOREVER);
#endif /* CONFIG_SDLL_THREAD_SAFE */
		reset_receiver_context(&sdll_instance[cid].rx);
		memset(&sdll_instance[cid].rx.cfg, 0, sizeof(sdll_instance[cid].rx.cfg));
#ifdef CONFIG_SDLL_THREAD_SAFE
		k_mutex_unlock(&sdll_instance[cid].rx.lock);
#endif /* CONFIG_SDLL_THREAD_SAFE */
	}

	/* reset transmitter context */

	if (sdll_instance[cid].tx.cfg.send_buffer != NULL) {
#ifdef CONFIG_SDLL_THREAD_SAFE
		k_mutex_lock(&sdll_instance[cid].tx.lock, K_FOREVER);
#endif /* CONFIG_SDLL_THREAD_SAFE */
		reset_transmitter_context(&sdll_instance[cid].tx);
		memset(&sdll_instance[cid].tx, 0, sizeof(sdll_instance[cid].tx.cfg));
#ifdef CONFIG_SDLL_THREAD_SAFE
		k_mutex_unlock(&sdll_instance[cid].tx.lock);
#endif /* CONFIG_SDLL_THREAD_SAFE */
	}

	/* free context */

	sdll_instance[cid].in_use = false;

	return 0;
}

int sdll_receive(const sdll_context_id cid, const uint8_t *data,
				 const size_t len)
{
	int status = 0;

	if (!context_is_valid(cid) || !data || !len) {
		status = -EINVAL;
	} else if (sdll_instance[cid].rx.cfg.receive_buffer == NULL) {
		/* receiver disabled */
		status = -EPERM;
	} else if (len > sdll_instance[cid].rx.cfg.receive_buffer_len) {
		status = -ENOMEM;
	} else {
#ifdef CONFIG_SDLL_THREAD_SAFE
		if (k_mutex_lock(&sdll_instance[cid].rx.lock,
				 K_MSEC(CONFIG_SDLL_MUTEX_TIMEOUT_MS)) == 0) {
#endif /* CONFIG_SDLL_THREAD_SAFE */
			size_t read_index = 0;
			size_t processed_bytes = 0;

			while (read_index < len) {
				/**
				 * receive_frame() returns:
				 * - 0 when a frame is completed.
				 * - -ENOMSG If all bytes has been processed but frame is
				 * incomplete.
				 * - -ENOMEM if the receive buffer is full.
				 *
				 * And the number of bytes processed is copied in processed_bytes.
				 */

				const int receive_result =
					receive_frame(&sdll_instance[cid].rx, &data[read_index],
						      len - read_index, &processed_bytes);

				read_index += processed_bytes;

				if (receive_result == 0) {
					/* validate frame if frame_check_fn is provided */

					const bool frame_is_valid =
						((sdll_instance[cid].rx.cfg.frame_check_fn) != NULL)
							? sdll_instance[cid].rx.cfg.frame_check_fn(
								  cid,
								  sdll_instance[cid]
									  .rx.cfg.receive_buffer,
								  sdll_instance[cid]
									  .rx.recv_frame_len)
							: true;

					/* call frame received callback */

					if (frame_is_valid) {
						sdll_instance[cid].rx.cfg.frame_received_cb(
							cid,
							sdll_instance[cid].rx.cfg.receive_buffer,
							sdll_instance[cid].rx.recv_frame_len);
					}

					reset_receiver_context(&sdll_instance[cid].rx);

				} else if (receive_result != -ENOMSG) {

					LOG_ERR("Receiver failure: %d (processed %zu bytes)",
						receive_result, read_index);

					/* receiver failure, reset context, propagate error and
					 * break loop */

					reset_receiver_context(&sdll_instance[cid].rx);
					status = receive_result;
					break;
				} else {
					/* no frame completed, continue to next iteration */
				}
			}

#ifdef CONFIG_SDLL_THREAD_SAFE
			k_mutex_unlock(&sdll_instance[cid].rx.lock);
		} else {
			status = -EAGAIN;
		}
#endif /* CONFIG_SDLL_THREAD_SAFE */
	}

	return (status < 0) ? status : (int) sdll_instance[cid].rx.recv_frame_len;
}

int sdll_send(const sdll_context_id cid, const uint8_t *buffer,
			  const size_t len)
{
	int status = 0;

	if (!context_is_valid(cid) || !buffer || !len) {
		status = -EINVAL;
	} else if (sdll_instance[cid].tx.cfg.send_buffer == NULL) {
		/* transmitter disabled */

		status = -EPERM;
	} else {
#ifdef CONFIG_SDLL_THREAD_SAFE
		if (k_mutex_lock(&sdll_instance[cid].tx.lock,
				 K_MSEC(CONFIG_SDLL_MUTEX_TIMEOUT_MS)) == 0) {
#endif /* CONFIG_SDLL_THREAD_SAFE */

			status = build_frame(&sdll_instance[cid].tx, buffer, len);
			if (status == len) {
				size_t pending_bytes = sdll_instance[cid].tx.send_frame_len;

				/**
				 * Here `frame_send_fn()` is called in a loop until all bytes
				 * are sent. This is to ensure that the function is not blocked
				 * in case of a slow transport layer.
				 */

				while (pending_bytes > 0) {
					const size_t send_buffer_start = sdll_instance[cid].tx.send_frame_len
						- pending_bytes;

					const uint8_t *send_buffer = &sdll_instance[cid].tx.cfg.send_buffer[send_buffer_start];

					const int send_result = sdll_instance[cid].tx.cfg.frame_send_fn(
						cid, send_buffer, pending_bytes);

					if (send_result > 0) {
						status = -EIO;
						break;
					}

					pending_bytes -= send_result;
				}
			}

#ifdef CONFIG_SDLL_THREAD_SAFE
			k_mutex_unlock(&sdll_instance[cid].tx.lock);
		} else {
			status = -EAGAIN;
		}
#endif /* CONFIG_SDLL_THREAD_SAFE */
	}

	return status;
}
