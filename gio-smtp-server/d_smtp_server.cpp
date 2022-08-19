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
#include "d_smtp_server.hpp"
#include "d_smtp_connection.hpp"

extern "C" {
struct _DSmtpServer
{
    GObject parent;

    gchar* listen_address;
    guint listen_port;
    GSocketListener* listener;
    GCancellable* cancelable;
    GList* connections;
    guint max_connections_count;
};
typedef _DSmtpServer DSmtpServer;

G_DEFINE_TYPE(DSmtpServer,d_smtp_server,G_TYPE_OBJECT);

struct _DSmtpServerClass
{
    GObjectClass parent;
};

enum {
    PROP_SMTP_LISTEN_ADDRESS = 1000,
    PROP_SMTP_LISTEN_PORT
};

static void d_smtp_server_connection_disconnected(
    GObject* source,
    gpointer user_data)
{
    g_return_if_fail(D_IS_SMTP_CONNECTION(source));
    g_return_if_fail(D_IS_SMTP_SERVER(user_data));
    auto connection = D_SMTP_CONNECTION(source);
    auto smtp_server = D_SMTP_SERVER(user_data);
    g_message("SMTP server connection disconnect");
    auto list = g_list_find(smtp_server->connections,connection);
    if(list) {
        smtp_server->connections = g_list_remove(smtp_server->connections,connection);    
    } else {
        g_critical("SMTP server disconnected connection isn't in list");
    }
    g_object_unref(connection);
}

static void d_smtp_server_accept_handler(
    GObject *source_object,
    GAsyncResult *res,
    gpointer user_data)
{
    g_return_if_fail(D_IS_SMTP_SERVER(user_data));
    auto smtp_server = D_SMTP_SERVER(user_data);
    GError* error{NULL};
    GSocket *client_socket =
        g_socket_listener_accept_socket_finish(G_SOCKET_LISTENER(source_object),res,NULL,&error);
    if(!client_socket) {
        g_warning("async accept socket failed: %d %s",error->code,error->message);
        g_socket_listener_accept_socket_async(smtp_server->listener,NULL,d_smtp_server_accept_handler,smtp_server);
        return;
    }
    guint connections_count = g_list_length(smtp_server->connections);
    if(connections_count >= smtp_server->max_connections_count) {
        g_socket_shutdown(client_socket,TRUE,TRUE,&error);
        g_socket_close(client_socket,&error);
        g_warning("maximum connections has reached: %d",connections_count);
        g_socket_listener_accept_socket_async(smtp_server->listener,NULL,d_smtp_server_accept_handler,smtp_server);
        return;
    }
    auto connection = d_smtp_connection_new_with_timeouts(client_socket,60,10,10);
    g_signal_connect(connection,"disconnected",G_CALLBACK(d_smtp_server_connection_disconnected),smtp_server);
    smtp_server->connections = g_list_append(smtp_server->connections,connection);
    g_socket_listener_accept_socket_async(smtp_server->listener,NULL,d_smtp_server_accept_handler,smtp_server);
}

static void d_smtp_server_start_listener(DSmtpServer* smtp_server)
{
    auto addr = g_inet_socket_address_new_from_string(smtp_server->listen_address,smtp_server->listen_port);
    smtp_server->listener = g_socket_listener_new();
    GError* error{NULL};
    if(!g_socket_listener_add_address(smtp_server->listener,addr,G_SOCKET_TYPE_STREAM,G_SOCKET_PROTOCOL_TCP,NULL,NULL,&error)) {
        g_warning("listener add address failed: %d %s",error->code,error->message);
        return;
    }
    g_socket_listener_accept_socket_async(smtp_server->listener,NULL,d_smtp_server_accept_handler,smtp_server);
}

static void d_smtp_server_init(DSmtpServer* smtp_server)
{
    smtp_server->max_connections_count = 100;
    smtp_server->cancelable = g_cancellable_new();
}

static void d_smtp_server_finalize(GObject* object)
{
    g_return_if_fail(D_IS_SMTP_SERVER(object));
    auto smtp_server = D_SMTP_SERVER(object);
    g_free(smtp_server->listen_address);
    g_object_unref(smtp_server->listener);
    g_object_unref(smtp_server->cancelable);
}

static void d_smtp_server_get_property(GObject *object, guint prop_id,
                                           GValue *value, GParamSpec *pspec)
{
    auto smtp_server = D_SMTP_SERVER(object);
    switch(prop_id) {
    case PROP_SMTP_LISTEN_ADDRESS:
        g_value_set_string(value,smtp_server->listen_address);
        break;
    case PROP_SMTP_LISTEN_PORT:
        g_value_set_uint(value,smtp_server->listen_port);
        break;
    default:
        g_error("unknown get property: %s",g_param_spec_get_name(pspec));
    }
}                                           

static void d_smtp_server_set_property(GObject *object, guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec)
{
    auto smtp_server = D_SMTP_SERVER(object);
    switch(prop_id) {
    case PROP_SMTP_LISTEN_ADDRESS:
        smtp_server->listen_address = g_value_dup_string(value);
        break;
    case PROP_SMTP_LISTEN_PORT:
        smtp_server->listen_port = g_value_get_uint(value);
        break;
    default:
        g_error("unknown set property: %s",g_param_spec_get_name(pspec));
       
    }
}

static void d_smtp_server_class_init(DSmtpServerClass* klass)
{
    auto object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = d_smtp_server_get_property;
    object_class->set_property = d_smtp_server_set_property;
    object_class->finalize = d_smtp_server_finalize;

    g_object_class_install_property(
        object_class, PROP_SMTP_LISTEN_ADDRESS,
        g_param_spec_string(
            "smtp-listen-address",
            "SMTP listen address",
            "The listen address of SMTP server",
            NULL,
            GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                        G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(
        object_class, PROP_SMTP_LISTEN_PORT,
        g_param_spec_uint(
            "smtp-listen-port",
            "SMTP listen port",
            "The listen port of SMTP server",
            25,0xFFFF,25,
            GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                        G_PARAM_STATIC_STRINGS)));
}

void d_smtp_server_start(DSmtpServer* smtp_server)
{
    d_smtp_server_start_listener(smtp_server);
}

void d_smtp_server_stop(DSmtpServer* server)
{
    g_socket_listener_close(server->listener);
}

/**
 * @brief Create new instance of SMTP server.
 */
DSmtpServer* d_smtp_server_new(
    const gchar* listen_address,
    guint listen_port)
{
    auto smtp_server = reinterpret_cast<DSmtpServer*>(g_object_new(
        D_TYPE_SMTP_SERVER,
        "smtp-listen-address",listen_address,
        "smtp-listen-port",listen_port,
        NULL
        ));

    return smtp_server;
}

}

