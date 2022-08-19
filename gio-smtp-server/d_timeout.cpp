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
#include "d_timeout.hpp"

/**
 * @brief Common object for processing cancleable timeouts.
 * @details Object dedicated for processing timeouts for GIO
 * asyncrhonous operations. The asynchrounous operations
 * will canceled by timeout expire event.
 * Currently object deal with three timeout values: read, write and close.
 */

extern "C" {

struct _DTimeout
{
    GObject parent;

    GCancellable* cancelable;
    gulong cancelable_id;

    TIMEOUT_OPERATION current_timeout_operation;

    gint read_timout_value;
    gint write_timout_value;
    gint close_timout_value;

    guint current_timeout_source_id;
};
typedef _DTimeout DTimeout;

G_DEFINE_TYPE(DTimeout,d_timeout,G_TYPE_OBJECT)

struct _DTimeoutClass {
    GObjectClass parenet_class;
};

/**
 * @brief Get cancelable instance used by timeout object.
 * @details Get the cacnelable object instance for using in GIO 
 * asynchrounous operations.
 */
GCancellable* d_timeout_get_cancelable(
    DTimeout* timeout)
{
    return timeout->cancelable;
}

/**
 * @brief Get current value for the specific operation timeout.
 * @details Get the current timeout value for specific
 * operation type enumeration value.
 * @param [in] timeout Timeout object instance.
 * @param [in] timeout_type The timeout type for which the value is requested.
 */
guint d_timeout_get_value(
    DTimeout* timeout,
    TIMEOUT_OPERATION timeout_type)
{
    switch(timeout_type) {
    case TIMEOUT_OPERATION_READ: return timeout->read_timout_value;
    case TIMEOUT_OPERATION_WRITE: return timeout->write_timout_value;
    case TIMEOUT_OPERATION_CLOSE: return timeout->close_timout_value;
    default: g_warning("unknown timeout operation type");
    }
    return 0;
}

/**
 * @brief Set current value for the specific operation timeout.
 * @param [in] timeout Timeout object instance.
 * @param [in] timeout_type The timeout type for which the value is set.
 * @param [in] timeout_value value of read timeout in seconds.
 */
void d_timeout_set_value(
    DTimeout* timeout,
    TIMEOUT_OPERATION timeout_type,
    guint timeout_value)
{
    switch(timeout_type) {
    case TIMEOUT_OPERATION_READ: 
        timeout->read_timout_value = timeout_value;
        break;
    case TIMEOUT_OPERATION_WRITE:
        timeout->write_timout_value = timeout_value;
        break;
    case TIMEOUT_OPERATION_CLOSE:
        timeout->close_timout_value = timeout_value;
        break;
    default: g_warning("unknown timeout operation type");
    }
}
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
    gpointer user_data)
{
    // Test if we already connected.
    if(timeout->cancelable_id) {
        // Disconnect previous handler from cancelable object.
        g_cancellable_disconnect(timeout->cancelable, timeout->cancelable_id);
        timeout->cancelable_id = 0;
    }
    // Connect out handler to the cancelabel object.
    timeout->cancelable_id = g_cancellable_connect(
        timeout->cancelable, handler,
        user_data, NULL);
}    

static gboolean internal_timeout_function(gpointer user_data)
{
    // We are called because specific amount of time expired.
    g_return_val_if_fail(D_IS_TIMEOUT(user_data),G_SOURCE_REMOVE);
    auto timeout = D_TIMEOUT(user_data);
    // Cancel the cancelable object.
    g_cancellable_cancel(timeout->cancelable);
    // Reset current source id for timeout.
    timeout->current_timeout_source_id = 0;
    // Request to remove this source from futher execution.
    return G_SOURCE_REMOVE;
}

/**
 * @brief Start specific timeout.
 */
void d_timeout_start(
    DTimeout* timeout,
    TIMEOUT_OPERATION timeout_type)
{
    // Reset, stop, previous timeout, if it started.
    d_timeout_stop(timeout,timeout_type);
    // Set the current timeout operation type.
    timeout->current_timeout_operation = timeout_type;
    // Get the specific timeout value.
    guint timeout_value = d_timeout_get_value(timeout,timeout_type);
    g_message("start timeout type:%d value:%d",timeout_type,timeout_value);
    // Reset cancelable object.
    if(g_cancellable_is_cancelled(timeout->cancelable)) {
        g_warning("timeout: cancelable object already was canceled, reset it");
        g_cancellable_reset(timeout->cancelable);
    }
    timeout->current_timeout_source_id = g_timeout_add_seconds(
        timeout_value, internal_timeout_function, timeout);
}

/**
 * @brief Stop specific timeout.
 */
void d_timeout_stop(
    DTimeout* timeout,
    TIMEOUT_OPERATION timeout_type)
{
    if(timeout->current_timeout_source_id) {
        g_source_remove(timeout->current_timeout_source_id);
        timeout->current_timeout_source_id = 0;
    }
}

static void d_timeout_init(DTimeout* timeout)
{
    // Create the SMTP connection cancelable object.
    timeout->cancelable = g_cancellable_new();
    // Setup default value for timeouts.
    timeout->read_timout_value = 10;
    timeout->write_timout_value = 10;
    timeout->close_timout_value = 10;
}

static void d_timeout_finalize(GObject* object)
{
    g_return_if_fail(D_IS_TIMEOUT(object));
    auto timeout = D_TIMEOUT(object);
    if(timeout->current_timeout_source_id) {
        g_source_remove(timeout->current_timeout_source_id);
        timeout->current_timeout_source_id = 0;
    }
    g_cancellable_disconnect(timeout->cancelable, timeout->cancelable_id);
    g_object_unref(timeout->cancelable);
    G_OBJECT_CLASS(d_timeout_parent_class)->finalize(object);
}

static void d_timeout_class_init(DTimeoutClass* klass)
{
    auto object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = d_timeout_finalize;
}

/**
 * @brief Create new instance of timeout object.
 */
DTimeout* d_timeout_new()
{
    auto timeout = reinterpret_cast<DTimeout*>(
        g_object_new(
            D_TYPE_TIMEOUT,
            NULL));

    return timeout;
}

}
