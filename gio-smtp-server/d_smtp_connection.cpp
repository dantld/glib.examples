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
#include "d_smtp_connection.hpp"
#include "d_smtp_command.hpp"
#include "d_smtp_state.hpp"
#include "d_timeout.hpp"

#define DATA_END ".\r\n"

extern "C" {

struct _DSmtpConnection
{
    GObject parent;
    GSocketConnection* socket_connection;

    gchar* my_host_name;
    // Read, write and close operations timeout processor.
    DTimeout* timeout;
    /// @brief Current connection state.
    DSmtpState* state;
    /// @brief Transient write buffer.
    GBytes* writing_bytes;
};
typedef _DSmtpConnection DSmtpConnection;

G_DEFINE_TYPE(DSmtpConnection,d_smtp_connection,G_TYPE_OBJECT)

struct _DSmtpConnectionClass
{
    GObjectClass parent;
};

enum {
    PROP_READ_TIMEOUT = 2000,
    PROP_WRITE_TIMEOUT,
    PROP_CLOSE_TIMEOUT
};

enum {
    SIGNAL_DISCONNECTED,
    NR_SIGNALS
};
static guint d_smtp_connection_signals[NR_SIGNALS];

static void d_smtp_connection_canceled(
    GObject* source,
    gpointer user_data)
{
    g_message("canceled!!!");
    /// TODO: Perform actions that need to be executed in case of operation has been canceled.
}

/**
 * @brief Setup socket.
 */
static void d_smtp_connection_set_socket(DSmtpConnection* connection,GSocket* socket)
{
    GError *error{nullptr};
    auto remote = g_socket_get_remote_address(socket,&error);
    if(remote) {
        auto raddr = g_inet_socket_address_get_address(G_INET_SOCKET_ADDRESS(remote));
        g_autofree gchar* addr_str = g_inet_address_to_string(raddr);
        g_message("new remote connection from: %s:%d", addr_str, g_inet_socket_address_get_port(G_INET_SOCKET_ADDRESS(remote)));
        g_object_unref(remote);
    }
    connection->socket_connection = g_socket_connection_factory_create_connection(socket);
    d_smtp_state_set_next_state(connection->state,SMTP_STATE_GREETING_SENDING);
}

static void d_smtp_connection_send_response_text(
    DSmtpConnection* connection,
    const gchar* response_text);

static void d_smtp_connection_send_response_code(
    DSmtpConnection* connection,
    guint response_code);

/**
 * @brief Test for valid command input
 */
static gboolean d_smtp_connection_test_input(
    DSmtpConnection* connection,
    GBytes* bytes)
{
    auto smtp_command = d_smtp_command_new();
    d_smtp_command_set_bytes(smtp_command,bytes);
    if(!d_smtp_command_process(smtp_command)) {
        g_object_unref(smtp_command);
        return FALSE;
    }
    SMTP_COMMAND command = d_smtp_command_get_smtp_command(smtp_command);
    if(command == SMTP_COMMAND_UNKNOWN) {
        g_object_unref(smtp_command);
        return FALSE;
    }

    if(!d_smtp_state_next_state_by_command(connection->state,command)) {
        return FALSE;
    }

    guint response_code = d_smtp_command_get_response_code(smtp_command);
    g_object_unref(smtp_command);
    d_smtp_connection_send_response_code(connection,response_code);

    return TRUE;
}
/**
 * @brief Completion handler for async read.
 */
static void d_smtp_connection_read_handle(
    GObject *source_object,
	GAsyncResult *res,
	gpointer user_data)
{
    auto connection = D_SMTP_CONNECTION(user_data);
    d_timeout_stop(connection->timeout,TIMEOUT_OPERATION_READ);
    GError *error{NULL};
    auto bytes = g_input_stream_read_bytes_finish(G_INPUT_STREAM(source_object),res,&error);
    if(!bytes) {
        g_warning("read bytes finish failed: %d %s",error->code,error->message);
        if(error->code == G_IO_ERROR_CANCELLED) {
        // Process some extra in case of operation has been canceled.
            g_message("connection read opertion was canceled");
        }
        // In most cases we couldn't (wantn't?) to continue in case async operation was failed.
        // So just clos the connection.
        d_smtp_connection_close(connection);
        return;
    }
    // Client sending the RAW data until the end of sequence marker.
    SMTP_STATE state = d_smtp_state_get_current_state(connection->state);
    if(state == SMTP_STATE_DATA_ACCEPTED) {
        gsize count{0};
        auto line = reinterpret_cast<const gchar*>(g_bytes_get_data(bytes,&count));
        g_print("DATA:  [%.*s]\n",(int)count,line);
        if(g_strstr_len(line,count,DATA_END)) {
            g_print("DATA END detected\n");
            d_smtp_state_set_next_state(connection->state, SMTP_STATE_DATA_ENDED);
            d_smtp_connection_send_response_code(connection,250);
        } else {
            // Continue to read the client data.
            auto is = g_io_stream_get_input_stream(G_IO_STREAM(connection->socket_connection));
            g_input_stream_read_bytes_async(is, 2048, G_PRIORITY_DEFAULT,
                                            d_timeout_get_cancelable(connection->timeout),
                                            d_smtp_connection_read_handle, connection);
        }
    } else {
        if(!d_smtp_connection_test_input(connection,bytes)) {
            d_smtp_connection_close(connection);
            g_bytes_unref(bytes);
            return;        
        }
    }
    g_bytes_unref(bytes);
}
/**
 * @brief Completion handler for async write.
 */
static void d_smtp_connection_write_handle(
    GObject *source_object,
	GAsyncResult *res,
	gpointer user_data)
{
    auto connection = D_SMTP_CONNECTION(user_data);
    d_timeout_stop(connection->timeout,TIMEOUT_OPERATION_WRITE);
    gsize bytes_written{0};
    GError *error{NULL};
    if(!g_output_stream_write_all_finish(G_OUTPUT_STREAM(source_object),res,&bytes_written,&error)) {
        g_bytes_unref(connection->writing_bytes);
        g_warning("write all bytes finish failed: %d %s",error->code,error->message);
        d_smtp_connection_close(connection);
        return;
    }
    auto expected_bytes = g_bytes_get_size(connection->writing_bytes);
    if(bytes_written != expected_bytes) {
        g_bytes_unref(connection->writing_bytes);
        g_warning("write all bytes finish unexpected bytes wrriten: %ld != %ld",bytes_written,expected_bytes);
        d_smtp_connection_close(connection);
        return;
    }
    // Try to get the new SMTP state based on write completed.
    // Basically we sent some response code and bytes are written.
    if(!d_smtp_state_next_by_write_complete(connection->state)) {
        g_bytes_unref(connection->writing_bytes);
        g_warning("write all bytes finish unexpected state");
        d_smtp_connection_close(connection);
        return;
    }

    SMTP_STATE state = d_smtp_state_get_current_state(connection->state);
    if(state == SMTP_STATE_CLOSE) {
        g_bytes_unref(connection->writing_bytes);
        g_message("write all bytes finish client quit requested");
        d_smtp_connection_close(connection);
        return;
    }
    g_bytes_unref(connection->writing_bytes);
    // Set the read timeout.
    d_timeout_start(connection->timeout,TIMEOUT_OPERATION_READ);
    // Switch to reading.
    auto is = g_io_stream_get_input_stream(G_IO_STREAM(connection->socket_connection));
    g_input_stream_read_bytes_async(is, 2048, G_PRIORITY_DEFAULT,
                                    d_timeout_get_cancelable(connection->timeout),
                                    d_smtp_connection_read_handle, connection);
}
/**
 * @brief Start async operation for writing bytes.
 */
static void d_smtp_connection_write_bytes(
    DSmtpConnection* connection,
    GBytes* bytes
    )
{
    auto os = g_io_stream_get_output_stream(G_IO_STREAM(connection->socket_connection));
    gsize count{0};
    auto buffer = g_bytes_get_data(bytes,&count);
    g_message("sending %ld bytes",count);
    // Set timeout.
    d_timeout_start(connection->timeout,TIMEOUT_OPERATION_WRITE);
    // Switch to write.
    g_output_stream_write_all_async(os, buffer, count, G_PRIORITY_DEFAULT,
                                    d_timeout_get_cancelable(connection->timeout),
                                    d_smtp_connection_write_handle, connection);
    connection->writing_bytes = bytes;
    g_bytes_ref(bytes);
}

static void d_smtp_connection_send_response_text(
    DSmtpConnection* connection,
    const gchar* response_text)
{
    gsize count = strlen(response_text);
    d_smtp_connection_write_bytes(connection,g_bytes_new(response_text,count));
}

static void d_smtp_connection_send_response_code(
    DSmtpConnection* connection,
    guint response_code)
{
    g_autofree gchar* response_text = g_strdup_printf("%d %s\r\n",response_code,connection->my_host_name);
    d_smtp_connection_send_response_text(connection,response_text);
}


/**
 * @brief Completion handler for async close.
 */
static void d_smtp_connection_close_handle(
    GObject *source_object,
	GAsyncResult *res,
	gpointer user_data)
{
    auto connection = D_SMTP_CONNECTION(user_data);
    d_timeout_stop(connection->timeout,TIMEOUT_OPERATION_CLOSE);
    g_signal_emit(connection,d_smtp_connection_signals[SIGNAL_DISCONNECTED],0,NULL);
    GError *error{NULL};
    if(!g_io_stream_close_finish(G_IO_STREAM(source_object),res,&error)) {
        g_warning("close async failed: %d %s",error->code,error->message);
        if(error->code == G_IO_ERROR_CANCELLED) {
            // Canceled can be initiated by the close timeout.
            // We need close socket by the hand in case of connection still connected.
            if(g_socket_connection_is_connected(connection->socket_connection)) {
                GSocket* socket = g_socket_connection_get_socket(connection->socket_connection);
                if(!g_socket_shutdown(socket,TRUE,TRUE,&error)) {
                    g_warning("shutdown socket failed: %d %s",error->code,error->message);
                    return;
                }
                if(!g_socket_close(socket,&error)) {
                    g_warning("close socket failed: %d %s",error->code,error->message);
                    return;
                }
            }
        }
        return;
    }
    g_message("connection closed\n");
}

void d_smtp_connection_close(DSmtpConnection* connection)
{
    // Set close operation timeout.
    d_timeout_start(connection->timeout,TIMEOUT_OPERATION_CLOSE);
    // Initiate the asynchrnous close socket operation.
    g_io_stream_close_async(
        G_IO_STREAM(connection->socket_connection),
        G_PRIORITY_DEFAULT,
        d_timeout_get_cancelable(connection->timeout),
        d_smtp_connection_close_handle,
        connection);
}


static void d_smtp_connection_init(DSmtpConnection* connection)
{
    // Create new timeout processing object.
    connection->timeout = d_timeout_new();
    // Create new instance of SMTP FSM.
    connection->state = d_smtp_state_new();
    /// TODO: place host name to the object properties.
    connection->my_host_name = g_strdup("localhost");
    // Connect out handler to the cancelabel object.
    d_timeout_connect(connection->timeout,G_CALLBACK(d_smtp_connection_canceled),connection);
}

static void d_smtp_connection_get_property(GObject *object, guint prop_id,
                                               GValue *value,
                                               GParamSpec *pspec)
{
    g_return_if_fail(D_IS_SMTP_CONNECTION(object));
    auto connection = D_SMTP_CONNECTION(object);
    switch(prop_id) {
    case PROP_READ_TIMEOUT:
        g_value_set_uint(value,d_timeout_get_value(connection->timeout,TIMEOUT_OPERATION_READ));
        break;
    case PROP_WRITE_TIMEOUT:
        g_value_set_uint(value,d_timeout_get_value(connection->timeout,TIMEOUT_OPERATION_WRITE));
        break;
    case PROP_CLOSE_TIMEOUT:
        g_value_set_uint(value,d_timeout_get_value(connection->timeout,TIMEOUT_OPERATION_CLOSE));
        break;
    }
}

static void d_smtp_connection_set_property(GObject *object, guint prop_id,
                                               const GValue *value,
                                               GParamSpec *pspec)
{
    g_return_if_fail(D_IS_SMTP_CONNECTION(object));
    auto connection = D_SMTP_CONNECTION(object);
    switch(prop_id) {
    case PROP_READ_TIMEOUT:
        d_smtp_connection_set_read_timeout(connection,g_value_get_uint(value));
        break;
    case PROP_WRITE_TIMEOUT:
        d_smtp_connection_set_write_timeout(connection,g_value_get_uint(value));
        break;
    case PROP_CLOSE_TIMEOUT:
        d_smtp_connection_set_close_timeout(connection,g_value_get_uint(value));
        break;
    }

}

static void d_smtp_connection_finalize(GObject* object)
{
    g_message("d_smtp_connection_finalize");
    g_return_if_fail(D_IS_SMTP_CONNECTION(object));
    auto connection = D_SMTP_CONNECTION(object);
    g_object_unref(connection->timeout);
    g_free(connection->my_host_name);
    G_OBJECT_CLASS(d_smtp_connection_parent_class)->finalize(object);
}

static void d_smtp_connection_class_disconnected_handler(
    GObject* source)
{
    g_message("class disconnected handler");
}

static void d_smtp_connection_class_init(DSmtpConnectionClass* klass)
{
    auto object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = d_smtp_connection_finalize;
    object_class->get_property = d_smtp_connection_get_property;
    object_class->set_property = d_smtp_connection_set_property;
    // Register signal "disconnected". Signal is called after the
    // socket connection will be closed or canceled.
    d_smtp_connection_signals[SIGNAL_DISCONNECTED] =
        g_signal_new_class_handler("disconnected",
                  D_TYPE_SMTP_CONNECTION,
                  GSignalFlags(G_SIGNAL_RUN_LAST|G_SIGNAL_MUST_COLLECT),
                  G_CALLBACK(d_smtp_connection_class_disconnected_handler),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

    g_object_class_install_property(
        object_class, PROP_READ_TIMEOUT,
        g_param_spec_uint(
            "read-timeout",
            "read timeout",
            "The maximum amount of time in seconds until the read bytes will be available",
            1,240,10,
            GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                        G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(
        object_class, PROP_WRITE_TIMEOUT,
        g_param_spec_uint(
            "write-timeout",
            "write timeout",
            "The maximum amount of time in seconds until the write operation has completed",
            1,240,10,
            GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                        G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(
        object_class, PROP_CLOSE_TIMEOUT,
        g_param_spec_uint(
            "close-timeout",
            "close timeout",
            "The maximum amount of time in seconds until the close socket operation has completed",
            1,240,10,
            GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                        G_PARAM_STATIC_STRINGS)));
}

guint d_smtp_connection_get_read_timeout(
    DSmtpConnection* connection)
{
    g_return_val_if_fail(D_IS_SMTP_CONNECTION,0);
    return d_timeout_get_value(connection->timeout,TIMEOUT_OPERATION_READ);
}

guint d_smtp_connection_get_write_timeout(
    DSmtpConnection* connection)
{
    g_return_val_if_fail(D_IS_SMTP_CONNECTION,0);
    return d_timeout_get_value(connection->timeout,TIMEOUT_OPERATION_CLOSE);
}

guint d_smtp_connection_get_close_timeout(
    DSmtpConnection* connection)
{
    g_return_val_if_fail(D_IS_SMTP_CONNECTION,0);
    return d_timeout_get_value(connection->timeout,TIMEOUT_OPERATION_CLOSE);
}

void d_smtp_connection_set_read_timeout(
    DSmtpConnection* connection,
    guint timeout_value)
{
    g_return_if_fail(D_IS_SMTP_CONNECTION);
    d_timeout_set_value(connection->timeout,TIMEOUT_OPERATION_READ,timeout_value);
    g_object_notify(G_OBJECT(connection),"read-timeout");
}

void d_smtp_connection_set_write_timeout(
    DSmtpConnection* connection,
    guint timeout_value)
{
    g_return_if_fail(D_IS_SMTP_CONNECTION);
    d_timeout_set_value(connection->timeout,TIMEOUT_OPERATION_WRITE,timeout_value);
    g_object_notify(G_OBJECT(connection),"write-timeout");
}

void d_smtp_connection_set_close_timeout(
    DSmtpConnection* connection,
    guint timeout_value)
{
    g_return_if_fail(D_IS_SMTP_CONNECTION);
    d_timeout_set_value(connection->timeout,TIMEOUT_OPERATION_CLOSE,timeout_value);
    g_object_notify(G_OBJECT(connection),"close-timeout");
}

/**
 * @brief Create new instance of SMTP connection.
 */
DSmtpConnection* d_smtp_connection_new(
    GSocket* smtp_client_socket)
{
    auto connection = reinterpret_cast<DSmtpConnection*>(
        g_object_new(
            D_TYPE_SMTP_CONNECTION,
            NULL
        ));

    d_smtp_connection_set_socket(connection,smtp_client_socket);
    g_autofree gchar* response = g_strdup_printf("220 %s SMTP example mail server\r\n",connection->my_host_name);
    d_smtp_connection_send_response_text(connection,response);
    d_smtp_state_set_next_state(connection->state, SMTP_STATE_GREETING_SENDING);

    return connection;
}

DSmtpConnection* d_smtp_connection_new_with_timeouts(
    GSocket* smtp_client_socket,
    guint read_timeout,
    guint write_timeout,
    guint close_timeout)
{
    auto connection = reinterpret_cast<DSmtpConnection*>(
        g_object_new(
            D_TYPE_SMTP_CONNECTION,
            "read-timeout",read_timeout,
            "write-timeout",write_timeout,
            "close-timeout",close_timeout,
            NULL
        ));

    d_smtp_connection_set_socket(connection,smtp_client_socket);
    g_autofree gchar* response = g_strdup_printf("220 %s SMTP example mail server\r\n",connection->my_host_name);
    d_smtp_connection_send_response_text(connection,response);
    d_smtp_state_set_next_state(connection->state, SMTP_STATE_GREETING_SENDING);

    return connection;
}

}
