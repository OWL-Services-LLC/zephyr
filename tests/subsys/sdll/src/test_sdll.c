/*
 * Copyright (C) 2025 OWL Services LLC. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sdll/sdll.h>

#define COMPARE_BUFFERS_MAX 2
#define COMPARE_BUFFERS_SIZE 32

struct buffer {
    uint8_t buffer[COMPARE_BUFFERS_SIZE];
    size_t length;
};

struct buffer hook_frame_received_cb_data[COMPARE_BUFFERS_MAX];
uint8_t hook_frame_received_cb_data_count = 0;
uint8_t hook_frame_received_cb_data_last = 0;

struct buffer hook_frame_send_fn_data[COMPARE_BUFFERS_MAX];
uint8_t hook_frame_send_fn_data_count = 0;
uint8_t hook_frame_send_fn_data_last = 0;

static int hook_send_fn(const sdll_context_id cid, const uint8_t * data,
    const size_t len)
{
    zassert_not_null(data, "data is NULL");
    zassert_true(len > 0, "len is 0");
    zassert_true(len <= COMPARE_BUFFERS_SIZE, "buffer overflow");

    zassert_true(hook_frame_send_fn_data_count < COMPARE_BUFFERS_MAX, "buffer count overflow");

    memcpy(hook_frame_send_fn_data[hook_frame_send_fn_data_count].buffer, data, len);
    hook_frame_send_fn_data[hook_frame_send_fn_data_count].length = len;
    hook_frame_send_fn_data_count++;

    return len;
}

static int hook_send_1byte_fn(const sdll_context_id cid, const uint8_t * data,
    const size_t len)
{
    zassert_not_null(data, "data is NULL");
    zassert_true(len > 0, "len is 0");
    zassert_true(len <= COMPARE_BUFFERS_SIZE, "buffer overflow");

    zassert_true(hook_frame_send_fn_data_count < COMPARE_BUFFERS_MAX, "buffer count overflow");

    memcpy(hook_frame_send_fn_data[hook_frame_send_fn_data_count].buffer, data, 1);
    hook_frame_send_fn_data[hook_frame_send_fn_data_count].length = 1;
    hook_frame_send_fn_data_count++;

    return 1;
}

static void hook_reset_send_fn(void)
{
    hook_frame_send_fn_data_last = 0;
    hook_frame_send_fn_data_count = 0;
}

static void hook_check_send_fn(const uint8_t * data, const size_t len)
{
    zassert_mem_equal(hook_frame_send_fn_data[hook_frame_send_fn_data_last].buffer, data, len, "data mismatch");
    zassert_equal(hook_frame_send_fn_data[hook_frame_send_fn_data_last].length, len, "length mismatch");
    hook_frame_send_fn_data_last++;
    printk ("sdll_void_frame_sent_fn: #%d, len: %d\n", hook_frame_send_fn_data_last, len);
}

static void hook_frame_received_cb(const sdll_context_id cid, const uint8_t * data, const size_t len)
{
    zassert_not_null(data, "data is NULL");
    zassert_true(len > 0, "len is 0");
    zassert_true(len <= COMPARE_BUFFERS_SIZE, "buffer size overflow");
    zassert_true(hook_frame_received_cb_data_last < hook_frame_received_cb_data_count, "buffer count overflow");
    zassert_mem_equal(hook_frame_received_cb_data[hook_frame_received_cb_data_last].buffer, data, len, "data mismatch");
    hook_frame_received_cb_data_last++;

    printk ("hook_frame_received_cb: #%d, cid: %d, len: %d\n", hook_frame_received_cb_data_last, cid, len);
    return;
}

static void hook_reset_frame_received_cb(void)
{
    hook_frame_received_cb_data_last = 0;
    hook_frame_received_cb_data_count = 0;
}

static void hook_add_frame_received_cb(const uint8_t * data, const size_t len)
{
    memcpy(hook_frame_received_cb_data[hook_frame_received_cb_data_count].buffer, data, len);
    hook_frame_received_cb_data[hook_frame_received_cb_data_count].length = len;
    hook_frame_received_cb_data_count++;
}

static void hook_frame_received_never_called_cb(const sdll_context_id cid, const uint8_t * data, const size_t len)
{
    zassert_unreachable("frame receive callback should never be called");
}

static void hook_send_fn_never_called(const sdll_context_id cid, const uint8_t * data, const size_t len)
{
    zassert_unreachable("frame receive callback should never be called");
}

static bool hook_validation_pass_fn(const sdll_context_id cid, const uint8_t * data, const size_t len)
{
    return true;
}

static bool hook_validation_fails_fn(const sdll_context_id cid, const uint8_t * data, const size_t len)
{
    return false;
}

static bool hook_validation_never_called_fn(const sdll_context_id cid, const uint8_t * data, const size_t len)
{
    zassert_unreachable("validation function should never be called");
    return false;
}

ZTEST(sdll_init, test_success_full_configuration)
{
    int ret;

    uint8_t rxbuffer[8];
    uint8_t txbuffer[8];

    struct sdll_receiver_config rxcfg = {
        .receive_buffer = rxbuffer,
        .receive_buffer_len = sizeof(rxbuffer),
        .frame_received_cb = hook_frame_received_cb,
        .frame_check_fn = NULL,
    };

    struct sdll_transmitter_config txcfg = {
        .send_buffer = txbuffer,
        .send_buffer_len = sizeof(txbuffer),
        .frame_send_fn = hook_send_fn,
    };

    ret = sdll_init(&rxcfg, &txcfg);
    zassert_equal(ret, 0, "sdll_init failed");
    sdll_deinit(ret);
}

ZTEST(sdll_init, test_success_receiver_only)
{
    int ret;
    uint8_t rxbuffer[8];

    struct sdll_receiver_config rxcfg = {
        .receive_buffer = rxbuffer,
        .receive_buffer_len = sizeof(rxbuffer),
        .frame_received_cb = hook_frame_received_cb,
        .frame_check_fn = NULL,
    };

    ret = sdll_init(&rxcfg, NULL);
    zassert_equal(ret, 0, "sdll_init failed");
    sdll_deinit(ret);
}

ZTEST(sdll_init, test_success_transmitter_only)
{
    int ret;
    uint8_t txbuffer[8];

    struct sdll_transmitter_config txcfg = {
        .send_buffer = txbuffer,
        .send_buffer_len = sizeof(txbuffer),
        .frame_send_fn = hook_send_fn,
    };

    ret = sdll_init(NULL, &txcfg);
    zassert_equal(ret, 0, "sdll_init failed");
    sdll_deinit(ret);
}

ZTEST(sdll_init, test_fails_no_receive_cb)
{
    int ret;
    uint8_t rxbuffer[8];
    struct sdll_receiver_config rxcfg = {
        .receive_buffer = rxbuffer,
        .receive_buffer_len = sizeof(rxbuffer),
        .frame_received_cb = NULL,
        .frame_check_fn = NULL,
    };

    ret = sdll_init(&rxcfg, NULL);
    zassert_equal(ret, -EINVAL, "sdll_init succeeded when it should have failed");
}

ZTEST(sdll_init, test_fails_invalid_receive_buffer)
{
    int ret;
    struct sdll_receiver_config rxcfg = {
        .receive_buffer = NULL,
        .receive_buffer_len = 8,
        .frame_received_cb = hook_frame_received_cb,
        .frame_check_fn = NULL,
    };

    ret = sdll_init(&rxcfg, NULL);
    zassert_equal(ret, -EINVAL, "sdll_init succeeded when it should have failed");
}

ZTEST(sdll_init, test_fails_invalid_receive_buffer_len)
{
    int ret;
    uint8_t rxbuffer[8];
    struct sdll_receiver_config rxcfg = {
        .receive_buffer = rxbuffer,
        .receive_buffer_len = 0,
        .frame_received_cb = hook_frame_received_cb,
        .frame_check_fn = NULL,
    };

    ret = sdll_init(&rxcfg, NULL);
    zassert_equal(ret, -EINVAL, "sdll_init succeeded when it should have failed");
}

ZTEST(sdll_init, test_fails_no_frame_send_cb)
{
    int ret;
    uint8_t txbuffer[8];

    struct sdll_transmitter_config txcfg = {
        .send_buffer = txbuffer,
        .send_buffer_len = sizeof(txbuffer),
        .frame_send_fn = NULL,
    };

    ret = sdll_init(NULL, &txcfg);
    zassert_equal(ret, -EINVAL, "sdll_init succeeded when it should have failed");
}

ZTEST(sdll_init, test_fails_invalid_send_buffer)
{
    int ret;
    struct sdll_transmitter_config txcfg = {
        .send_buffer = NULL,
        .send_buffer_len = 8,
        .frame_send_fn = hook_send_fn,
    };

    ret = sdll_init(NULL, &txcfg);
    zassert_equal(ret, -EINVAL, "sdll_init succeeded when it should have failed");
}

ZTEST(sdll_init, test_fails_invalid_send_buffer_len)
{
    int ret;
    uint8_t txbuffer[8];

    struct sdll_transmitter_config txcfg = {
        .send_buffer = txbuffer,
        .send_buffer_len = SDLL_SEND_BUFFER_SIZE_MIN - 1,
        .frame_send_fn = hook_send_fn,
    };

    ret = sdll_init(NULL, &txcfg);
    zassert_equal(ret, -EINVAL, "sdll_init succeeded when it should have failed");
}

ZTEST(sdll_init, test_fails_no_mem)
{
    uint8_t rxbuffer[8];
    uint8_t txbuffer[8];

    struct sdll_receiver_config rxcfg = {
        .receive_buffer = rxbuffer,
        .receive_buffer_len = sizeof(rxbuffer),
        .frame_received_cb = hook_frame_received_cb,
        .frame_check_fn = NULL,
    };

    struct sdll_transmitter_config txcfg = {
        .send_buffer = txbuffer,
        .send_buffer_len = sizeof(txbuffer),
        .frame_send_fn = hook_send_fn,
    };

    const int ret1 = sdll_init(&rxcfg, NULL);
    zassert_true(ret1 >= 0, "sdll_init failed, result = %d", ret1);

    const int ret2 = sdll_init(NULL, &txcfg);
    zassert_equal(ret2, -ENOMEM, "sdll_init succeeded when it should have failed");

    zassert_equal(sdll_deinit(ret1), 0, "sdll_deinit failed");
}

ZTEST(sdll_deinit, test_success)
{
    uint8_t rxbuffer[8];
    struct sdll_receiver_config rxcfg = {
        .receive_buffer = rxbuffer,
        .receive_buffer_len = sizeof(rxbuffer),
        .frame_received_cb = hook_frame_received_cb,
        .frame_check_fn = NULL,
    };

    const int cid = sdll_init(&rxcfg, NULL);
    zassert_true(cid >= 0, "sdll_init failed, result = %d", cid);
    const int ret = sdll_deinit(cid);
    zassert_equal(ret, 0, "sdll_deinit failed");
}

ZTEST(sdll_send, test_success)
{
    const uint8_t data_to_send[] = {
        0x00,
        0x01,
        CONFIG_SDLL_ESCAPE_CHAR,
        0x02,
        0x03
    };

    const uint8_t expected_frame[] = {
        CONFIG_SDLL_BOUNDARY_CHAR,
        0x00,
        0x01,
        CONFIG_SDLL_ESCAPE_CHAR,
        (CONFIG_SDLL_ESCAPE_CHAR ^ CONFIG_SDLL_ESCAPE_MASK),
        0x02,
        0x03,
        CONFIG_SDLL_BOUNDARY_CHAR
    };

    uint8_t txbuffer[12];
    struct sdll_transmitter_config txcfg = {
        .send_buffer = txbuffer,
        .send_buffer_len = sizeof(txbuffer),
        .frame_send_fn = hook_send_fn,
    };

    hook_reset_send_fn();

    /* initialize library */

    const int cid = sdll_init(NULL, &txcfg);
    zassert_true(cid >= 0, "sdll_init failed, result = %d", cid);

    /* send data */

    const int ret = sdll_send(cid, data_to_send, sizeof(data_to_send));
    zassert_equal(ret, sizeof(data_to_send), "sdll_send failed, result = %d", ret);

    /* verify sent frame */

    hook_check_send_fn(expected_frame, sizeof(expected_frame));

    zassert_equal(sdll_deinit(cid), 0, "sdll_deinit failed");
}


ZTEST(sdll_send, test_success_polling_1_byte)
{
    const uint8_t data_to_send[] = {
        0x00,
        0x01,
        CONFIG_SDLL_ESCAPE_CHAR,
        0x02,
        0x03
    };

    const uint8_t expected_frame[] = {
        CONFIG_SDLL_BOUNDARY_CHAR,
        0x00,
        0x01,
        CONFIG_SDLL_ESCAPE_CHAR,
        (CONFIG_SDLL_ESCAPE_CHAR ^ CONFIG_SDLL_ESCAPE_MASK),
        0x02,
        0x03,
        CONFIG_SDLL_BOUNDARY_CHAR
    };

    uint8_t txbuffer[12];
    struct sdll_transmitter_config txcfg = {
        .send_buffer = txbuffer,
        .send_buffer_len = sizeof(txbuffer),
        .frame_send_fn = hook_send_1byte_fn,
    };

    hook_reset_send_fn();

    /* initialize library */

    const int cid = sdll_init(NULL, &txcfg);
    zassert_true(cid >= 0, "sdll_init failed, result = %d", cid);

    /* send data */

    const int ret = sdll_send(cid, data_to_send, sizeof(data_to_send));
    zassert_equal(ret, sizeof(data_to_send), "sdll_send failed, result = %d", ret);

    /* verify sent frame */

    hook_check_send_fn(expected_frame, sizeof(expected_frame));

    zassert_equal(sdll_deinit(cid), 0, "sdll_deinit failed");
}

ZTEST(sdll_send, test_fails_invalid_context)
{
    const uint8_t data_to_send[8] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    const int cid = -1;

    const int ret = sdll_send(cid, data_to_send, sizeof(data_to_send));
    zassert_equal(ret, -EINVAL, "sdll_send succeeded when it should have failed");
}

ZTEST(sdll_send, test_fails_invalid_buffer)
{
    const int cid = 0;

    const int ret = sdll_send(cid, NULL, 1);
    zassert_equal(ret, -EINVAL, "sdll_send succeeded when it should have failed");
}

ZTEST(sdll_send, test_fails_invalid_buffer_length)
{
    const uint8_t data_to_send[8] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    const int cid = 0;

    const int ret = sdll_send(cid, data_to_send, 0);
    zassert_equal(ret, -EINVAL, "sdll_send succeeded when it should have failed");
}

ZTEST(sdll_send, test_fails_no_memory)
{
    const uint8_t data_to_send[] = {
        0x00,
        0x01,
        CONFIG_SDLL_ESCAPE_CHAR,
        0x02,
        0x03
    };

    uint8_t txbuffer[sizeof(data_to_send) - 1];

    struct sdll_transmitter_config txcfg = {
        .send_buffer = txbuffer,
        .send_buffer_len = sizeof(txbuffer),
        .frame_send_fn = hook_send_fn,
    };

    /* initialize library */

    const int cid = sdll_init(NULL, &txcfg);
    zassert_true(cid >= 0, "sdll_init failed, result = %d", cid);

    /* send data */

    const int ret = sdll_send(cid, data_to_send, sizeof(data_to_send));
    zassert_equal(ret, -ENOBUFS, "sdll_send succeeded when it should have failed");

    zassert_equal(sdll_deinit(cid), 0, "sdll_deinit failed");
}

ZTEST(sdll_receive, test_success_receiving_1_frame_no_validation)
{
    const uint8_t data_to_receive[] = {
        CONFIG_SDLL_BOUNDARY_CHAR,
        0x00,
        0x01,
        CONFIG_SDLL_ESCAPE_CHAR,
        (CONFIG_SDLL_ESCAPE_CHAR ^ CONFIG_SDLL_ESCAPE_MASK),
        0x02,
        0x03,
        CONFIG_SDLL_BOUNDARY_CHAR
    };

    const uint8_t expected_data[] = {
        0x00,
        0x01,
        CONFIG_SDLL_ESCAPE_CHAR,
        0x02,
        0x03
    };

    uint8_t rxbuffer[12];
    struct sdll_receiver_config rxcfg = {
        .receive_buffer = rxbuffer,
        .receive_buffer_len = sizeof(rxbuffer),
        .frame_received_cb = hook_frame_received_cb,
        .frame_check_fn = NULL,
    };

    hook_reset_frame_received_cb();
    hook_add_frame_received_cb(expected_data, sizeof(expected_data));

    /* initialize library */

    const int cid = sdll_init(&rxcfg, NULL);
    zassert_true(cid >= 0, "sdll_init failed, result = %d", cid);

    /* receive data */

    const int ret = sdll_receive(cid, data_to_receive, sizeof(data_to_receive));
    zassert_equal(ret, 0, "sdll_receive failed");

    /* deinitialize library */

    zassert_equal(sdll_deinit(cid), 0, "sdll_deinit failed");
}


ZTEST(sdll_receive, test_success_receiving_and_validating_1_frame)
{
    const uint8_t data_to_receive[] = {
        CONFIG_SDLL_BOUNDARY_CHAR,
        0x00,
        0x01,
        CONFIG_SDLL_ESCAPE_CHAR,
        (CONFIG_SDLL_ESCAPE_CHAR ^ CONFIG_SDLL_ESCAPE_MASK),
        0x02,
        0x03,
        CONFIG_SDLL_BOUNDARY_CHAR
    };

    const uint8_t expected_data[] = {
        0x00,
        0x01,
        CONFIG_SDLL_ESCAPE_CHAR,
        0x02,
        0x03
    };

    uint8_t rxbuffer[12];
    struct sdll_receiver_config rxcfg = {
        .receive_buffer = rxbuffer,
        .receive_buffer_len = sizeof(rxbuffer),
        .frame_received_cb = hook_frame_received_cb,
        .frame_check_fn = hook_validation_pass_fn,
    };

    hook_reset_frame_received_cb();
    hook_add_frame_received_cb(expected_data, sizeof(expected_data));

    /* initialize library */

    const int cid = sdll_init(&rxcfg, NULL);
    zassert_true(cid >= 0, "sdll_init failed, result = %d", cid);

    /* receive data */

    const int ret = sdll_receive(cid, data_to_receive, sizeof(data_to_receive));
    zassert_equal(ret, 0, "sdll_receive failed");

    /* deinitialize library */

    zassert_equal(sdll_deinit(cid), 0, "sdll_deinit failed");
}


ZTEST(sdll_receive, test_success_receiving_2_frames_no_validation)
{
    const uint8_t data_to_receive[] = {
        CONFIG_SDLL_BOUNDARY_CHAR,
        0x00,
        0x01,
        CONFIG_SDLL_BOUNDARY_CHAR,
        CONFIG_SDLL_BOUNDARY_CHAR,
        0x02,
        0x03,
        CONFIG_SDLL_BOUNDARY_CHAR
    };

    const uint8_t expected_data_1[] = {
        0x00,
        0x01,
    };

    const uint8_t expected_data_2[] = {
        0x02,
        0x03
    };

    uint8_t rxbuffer[12];
    struct sdll_receiver_config rxcfg = {
        .receive_buffer = rxbuffer,
        .receive_buffer_len = sizeof(rxbuffer),
        .frame_received_cb = hook_frame_received_cb,
        .frame_check_fn = NULL,
    };

    /* initialize library */

    const int cid = sdll_init(&rxcfg, NULL);
    zassert_true(cid >= 0, "sdll_init failed, result = %d", cid);

    /* receive data */

    hook_reset_frame_received_cb();
    hook_add_frame_received_cb(expected_data_1, sizeof(expected_data_1));
    hook_add_frame_received_cb(expected_data_2, sizeof(expected_data_2));

    const int ret = sdll_receive(cid, data_to_receive, sizeof(data_to_receive));

    /* no more bytes in buffer, should return 0 */

    zassert_equal(ret, 0, "sdll_receive failed");

    zassert_equal(sdll_deinit(cid), 0, "sdll_deinit failed");
}

ZTEST(sdll_receive, test_hook_validation_fails_fn)
{
    const uint8_t data_to_receive[] = {
        CONFIG_SDLL_BOUNDARY_CHAR,
        0x00,
        0x01,
        0x02,
        0x03,
        CONFIG_SDLL_BOUNDARY_CHAR
    };

    const uint8_t expected_data[] = {
        0x00,
        0x01,
        0x02,
        0x03
    };

    uint8_t rxbuffer[12];
    struct sdll_receiver_config rxcfg = {
        .receive_buffer = rxbuffer,
        .receive_buffer_len = sizeof(rxbuffer),
        .frame_received_cb = hook_send_fn_never_called,
        .frame_check_fn = hook_validation_fails_fn,
    };

    hook_reset_frame_received_cb();
    hook_add_frame_received_cb(expected_data, sizeof(expected_data));

    /* initialize library */

    const int cid = sdll_init(&rxcfg, NULL);
    zassert_true(cid >= 0, "sdll_init failed, result = %d", cid);

    /* receive data */

    const int ret = sdll_receive(cid, data_to_receive, sizeof(data_to_receive));

    /* no more bytes in buffer, should return 0 */

    zassert_equal(ret, 0, "sdll_receive failed: %d", ret);

    /* deinitialize library */

    zassert_equal(sdll_deinit(cid), 0, "sdll_deinit failed");
}

ZTEST(sdll_receive, test_frame_not_finished)
{
    const uint8_t data_to_receive[] = {
        CONFIG_SDLL_BOUNDARY_CHAR,
        0x00,
        0x01,
        0x02,
    };

    const int expected_sdll_receive_result = 3;

    uint8_t rxbuffer[12];
    struct sdll_receiver_config rxcfg = {
        .receive_buffer = rxbuffer,
        .receive_buffer_len = sizeof(rxbuffer),
        .frame_received_cb = hook_frame_received_never_called_cb,
        .frame_check_fn = hook_validation_never_called_fn,
    };

    /* initialize library */

    const int cid = sdll_init(&rxcfg, NULL);
    zassert_true(cid >= 0, "sdll_init failed, result = %d", cid);

    /* receive data */

    const int ret = sdll_receive(cid, data_to_receive, sizeof(data_to_receive));

    /* should return 3 bytes pending in receive buffer */

    zassert_equal(ret, expected_sdll_receive_result, "sdll_receive failed");


    zassert_equal(sdll_deinit(cid), 0, "sdll_deinit failed");
}


ZTEST_SUITE(sdll_init, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(sdll_deinit, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(sdll_send, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(sdll_receive, NULL, NULL, NULL, NULL, NULL);