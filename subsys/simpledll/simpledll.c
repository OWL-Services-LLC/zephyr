/*
 * Copyright (C) 2025 OWL Services LLC. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

  /**
 * @file sdll.c
 *
 * @brief Simple Data Link Layer (SimpleDLL) API implementation
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <zephyr/kernel.h>

#include <subsys/simpledll/simpledll.h>

#define SDLL_MINIMUM_BUFFER_SIZE 4

#define SDLL_STATUS_NEW_FRAME_BIT 0
#define SDLL_STATUS_ESCAPE_NEXT_BIT 1

struct sdll_context {
    struct sdll_config cfg;
    int send_status;
    size_t send_frame_len;
    int recv_status;
};

static struct sdll_context sdll_instance[CONFIG_SIMPLEDLL_MAX_INSTANCES];
static size_t sdll_instance_count;

static int frame_put(struct sdll_context * ctx, const uint8_t * payload, const size_t paylaod_len)
{
    int status = 0;
    size_t nbytes = 0;

    /** @todo Check if this function can be optimized */

    while (nbytes < paylaod_len)
    {
        if (ctx->send_frame_len > ctx->cfg.send_buffer.len)
        {
            status = -ENOMEM;
            break;
        }

        if (!(ctx->send_status & SDLL_STATUS_NEW_FRAME_BIT))
        {
            ctx->send_status |= SDLL_STATUS_NEW_FRAME_BIT;

            ctx->cfg.ptr[0] = CONFIG_SIMPLEDLL_BOUNDARY_CHAR;
            ctx->send_frame_len++;
            continue;
        }

        if (payload[nbytes] == CONFIG_SIMPLEDLL_BOUNDARY_CHAR || payload[nbytes] == CONFIG_SIMPLEDLL_ESCAPE_CHAR)
        {
            if (ctx->send_frame_len + 1 > ctx->cfg.send_buffer.len)
            {
                status = -ENOMEM;
                break;
            }

            /* Send escape character */

            ctx->cfg.ptr[ctx->send_frame_len] = CONFIG_SIMPLEDLL_ESCAPE_CHAR;
            ctx->send_frame_len++;

            /* Send inverted character */

            ctx->cfg.ptr[ctx->send_frame_len] = payload[nbytes] ^ CONFIG_SIMPLEDLL_INVERT_MASK;
            ctx->send_frame_len++;
        }
        else
        {
            /* No special char escape required */

            ctx->cfg.ptr[ctx->send_frame_len] = payload[nbytes];
            ctx->send_frame_len++;
        }

        nbytes++;
    }

    if (nbytes == paylaod_len)
    {
        if (ctx->send_frame_len + 1 > ctx->cfg.send_buffer.len)
        {
            status = -ENOMEM;
        }
        else
        {
            ctx->cfg.ptr[ctx->send_frame_len] = CONFIG_SIMPLEDLL_BOUNDARY_CHAR;
            ctx->send_frame_len++;
        }
    }

    return status;
}

static int frame_put(struct sdll_context * ctx, const uint8_t payload_byte, const size_t paylaod_len)
{
    if (ctx->send_frame_len >= ctx->cfg.send_buffer.len)
    {
        return -ENOMEM;
    }

    if (!(ctx->send_status & BIT(SDLL_STATUS_NEW_FRAME_BIT)))
    {
        ctx->send_status |= BIT(SDLL_STATUS_NEW_FRAME_BIT);

        /* The first byte is the boundary char */

        ctx->cfg.ptr[0] = CONFIG_SIMPLEDLL_BOUNDARY_CHAR;
        ctx->send_frame_len = 1;

        /* Re-check buffer length */

        if (ctx->send_frame_len >= ctx->cfg.send_buffer.len)
        {
            return -ENOMEM;
        }
    }

    if (payload_byte == CONFIG_SIMPLEDLL_BOUNDARY_CHAR || payload_byte == CONFIG_SIMPLEDLL_ESCAPE_CHAR)
    {
        /* Re-check buffer length */

        if (ctx->send_frame_len + 1 >= ctx->cfg.send_buffer.len)
        {
            return -ENOMEM;
        }

        /* Send escape character */

        ctx->cfg.ptr[ctx->send_frame_len] = CONFIG_SIMPLEDLL_ESCAPE_CHAR;
        ctx->send_frame_len++;

        /* Send inverted character */

        ctx->cfg.ptr[ctx->send_frame_len] = payload_byte ^ CONFIG_SIMPLEDLL_INVERT_MASK;
        ctx->send_frame_len++;
    }
    else
    {

        /* No special char escape required */

        ctx->cfg.ptr[ctx->send_frame_len] = payload_byte;
        ctx->send_frame_len++;
    }

    return 0;
}

int sdll_init(struct sdll_config * cfg, struct sdll_context ** ctx)
{
    struct sdll_context * new_sdll;

    if (!cfg || !ctx)
    {
        return -EINVAL;
    }

    if (sdll_instance_count >= CONFIG_SIMPLEDLL_MAX_INSTANCES)
    {
        return -ENOMEM;
    }

    if (!cfg->receive_buffer.ptr || cfg->receive_buffer.len < SDLL_MINIMUM_BUFFER_SIZE)
    {
        return -EINVAL;
    }

    if (!cfg->send_buffer.ptr || cfg->send_buffer.len < SDLL_MINIMUM_BUFFER_SIZE)
    {
        return -EINVAL;
    }

    new_sdll = &sdll_instance[sdll_instance_count];

    /* Initialize context */


    new_sdll->cfg = *cfg;
    new_sdll->send_status = 0;
    new_sdll->send_frame_len = 0;
    new_sdll->recv_status = 0;

    /* Return new context */

    *ctx = &sdll_instance[sdll_instance_count];
    sdll_instance_count++;

    return 0;
}

int sdll_receive(struct sdll_context * ctx, const uint8_t * buffer, const size_t len)
{
    if (!ctx || !buffer || !len) {
        return -EINVAL;
    }

    if (len > ctx->recv_buffer_len) {
        return -ENOMEM;
    }

    if (ctx->cb.frame_check && !ctx->cb.frame_check(buffer, len)) {
        return -EINVAL;
    }

    if (ctx->cb.frame_received) {
        ctx->cb.frame_received(buffer, len);
    }

    return len;
}