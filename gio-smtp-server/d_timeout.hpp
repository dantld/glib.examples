/*
 * Copyright (c) 2022 Daniyar Tleulin <daniyar.tleulin@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef __D__NEW__CANCELABLE_TIMEOUT__HPP__
#define __D__NEW__CANCELABLE_TIMEOUT__HPP__
/**
 * @brief Common object for processing cancleable timeouts.
 * @details Object dedicated for processing timeouts for GIO
 * asyncrhonous operations. The asynchrounous operations
 * will canceled by timeout expire event.
 * Currently object deal with three timeout values: read, write and close.
 */

#include <gio/gio.h>

enum TIMEOUT_OPERATION {
    TIMEOUT_OPERATION_READ,
    TIMEOUT_OPERATION_WRITE,
    TIMEOUT_OPERATION_CLOSE,
};

extern "C" {
#define D_TYPE_TIMEOUT (d_timeout_get_type())

G_DECLARE_FINAL_TYPE(DTimeout,d_timeout,D,TIMEOUT,GObject)

/**
 * @brief Get cancelable instance used by timeout object.
 * @details Get the cacnelable object instance for using in GIO 
 * asynchrounous operations.
 */
GCancellable* d_timeout_get_cancelable(
    DTimeout* timeout);

/**
 * @brief Get current value for the specific operation timeout.
 * @details Get the current timeout value for specific
 * operation type enumeration value.
 * @param [in] timeout Timeout object instance.
 * @param [in] timeout_type The timeout type for which the value is requested.
 */
guint d_timeout_get_value(
    DTimeout* timeout,
    TIMEOUT_OPERATION timeout_type);

/**
 * @brief Set current value for the specific operation timeout.
 * @param [in] timeout Timeout object instance.
 * @param [in] timeout_type The timeout type for which the value is set.
 * @param [in] timeout_value value of read timeout in seconds.
 */
void d_timeout_set_value(
    DTimeout* timeout,
    TIMEOUT_OPERATION timeout_type,
    guint timeout_value);

/**
 * @brief Connect cancel handler to the internal timeout canceleable object.
 * @details Function connect user provided cancel handler to the internal
 * timeout cancelable object. Next call of this function disconnect previous
 * handler. Pass NULL value as handler for disconnect current handler from
 * cancelable object.
 * @param [in] timeout Timeout object instance.
 * @param [in] handler The cancelable handler callback.
 */
void d_timeout_connect(
    DTimeout* timeout,
    GCallback handler,
    gpointer user_data);

/**
 * @brief Start specific timeout.
 */
void d_timeout_start(
    DTimeout* timeout,
    TIMEOUT_OPERATION timeout_type);

/**
 * @brief Stop specific timeout.
 */
void d_timeout_stop(
    DTimeout* timeout,
    TIMEOUT_OPERATION timeout_type);

/**
 * @brief Create new instance of timeout object.
 */
DTimeout* d_timeout_new();

}

#endif //#ifndef __D__NEW__CANCELABLE_TIMEOUT__HPP__
