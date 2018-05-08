/*
 * Copyright 2018 The Fuchsia Authors.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <stdlib.h>

#include <zircon/listnode.h>
#include <zircon/status.h>

#include "htc.h"
#include "htt.h"
#include "wmi.h"
#include "wmi-tlv.h"

// #define DEBUG_MSG_BUF 1

// Each of the modules that generate or parse messages should instantiate its
// top-level macro (e.g., WMI_MSGS) with a comma-delimited list of messages,
// defined as MSG(name, base-name, type). The top-level macros will be used
// whenever a complete list of messages will be needed, with MSG defined to
// match the context where it is being used.
#define MSG(type, base, hdr) type
enum ath10k_msg_type {
    ATH10K_MSG_TYPE_BASE,

    // Instantiated from htc.h
    HTC_MSGS,

    // Instantiated from wmi.h
    WMI_MSGS,

    // Instantiated from wmi-tlv.h
    WMI_TLV_MSGS,

    // Instantiated from htt.h
    HTT_MSGS,

    // Must be last
    ATH10K_MSG_TYPE_COUNT
};
#undef MSG

enum ath10k_tx_flags {
    ATH10K_TX_BUF_NO_HWCRYPT = BIT(0),
    ATH10K_TX_BUF_DTIM_ZERO = BIT(1),
    ATH10K_TX_BUF_DELIVER_CAB = BIT(2),
    ATH10K_TX_BUF_MGMT = BIT(3),
    ATH10K_TX_BUF_QOS = BIT(4),
};

struct ath10k_msg_buf {
    struct ath10k_msg_buf_state* state;
    enum ath10k_msg_type type;
    list_node_t listnode;
    io_buffer_t buf;
    void* vaddr;
    zx_paddr_t paddr;
    size_t capacity;
    size_t used;
    union {
        struct {
            size_t frame_offset;
            size_t frame_size;
        } rx;
        struct {
            uint32_t flags;
            wlan_tx_info_t tx_info;
        } tx;
    };

    const char* alloc_file_name;
    size_t alloc_line_num;
    list_node_t debug_listnode;
};

struct ath10k_msg_buf_state {
    struct ath10k* ar;
    mtx_t lock;

    // Lists of previously-allocated buffers
    list_node_t buf_pool;

    size_t bufs_allocated;
    list_node_t bufs_in_use;
    zx_paddr_t highest_addr_allocated;
};

// Initialize the module
zx_status_t ath10k_msg_bufs_init(struct ath10k* ar);

// Allocate a new buffer of the specified type, plus any extra space requested
zx_status_t ath10k_msg_buf_alloc_real(struct ath10k* ar,
                                      struct ath10k_msg_buf** msg_buf_ptr,
                                      enum ath10k_msg_type type,
                                      size_t extra_bytes,
                                      bool force_new,
                                      const char* filename,
                                      size_t line_num);

#define ath10k_msg_buf_alloc(ar, ptr, type, bytes) \
        ath10k_msg_buf_alloc_real(ar, ptr, type, bytes, false, __FILE__, __LINE__)

void* ath10k_msg_buf_get_header(struct ath10k_msg_buf* msg_buf,
                                enum ath10k_msg_type msg_type);

void* ath10k_msg_buf_get_payload(struct ath10k_msg_buf* msg_buf);

size_t ath10k_msg_buf_get_payload_len(struct ath10k_msg_buf* msg_buf,
                                      enum ath10k_msg_type msg_type);

size_t ath10k_msg_buf_get_offset(enum ath10k_msg_type msg_type);

size_t ath10k_msg_buf_get_payload_offset(enum ath10k_msg_type msg_type);

void ath10k_msg_buf_free(struct ath10k_msg_buf* msg_buf);

void ath10k_msg_buf_dump_stats(struct ath10k* ar);

void ath10k_msg_buf_dump(struct ath10k_msg_buf* msg_buf, const char* prefix);
