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
#include "d_smtp_server_app.hpp"
#include "d_smtp_server.hpp"
#include <gio/gunixinputstream.h>


extern "C" {
struct _DSmtpServerApp {
    GApplication parent;

    DSmtpServer* server;
};

typedef _DSmtpServerApp DSmtpServerApp;

G_DEFINE_TYPE(DSmtpServerApp,d_smtp_server_app,G_TYPE_APPLICATION);

struct _DSmtpServerAppClass {
    GApplicationClass parent;    
};

static void d_smtp_server_app_shutdown(
    GApplication* app)
{
    G_APPLICATION_CLASS(d_smtp_server_app_parent_class)->shutdown(app);
    g_message("shutdown");
    auto myapp = D_SMTP_SERVER_APP(app);
    d_smtp_server_stop(myapp->server);
}

static void d_smtp_server_app_startup(
    GApplication* app)
{
    G_APPLICATION_CLASS(d_smtp_server_app_parent_class)->startup(app);
    g_message("startup");
    auto myapp = D_SMTP_SERVER_APP(app);
}

int d_smtp_server_app_command_line(
    GApplication* app,
    GApplicationCommandLine* command_line)
{
    g_message("command-line");
    int argc{0};
    gchar** argv = g_application_command_line_get_arguments(command_line,&argc);


    int ret_val = G_APPLICATION_CLASS(d_smtp_server_app_parent_class)->command_line(app,command_line);
    g_application_activate(app);

    return ret_val;
}

static void d_smtp_server_app_activate(
    GApplication* app)
{
    g_message("activate");
    auto myapp = D_SMTP_SERVER_APP(app);

    d_smtp_server_start(myapp->server);

    g_application_hold(app);
}

static void d_smtp_server_app_finalize(
    GObject* object)
{
    g_message("finalize");
    auto myapp = D_SMTP_SERVER_APP(object);
}

static void d_smtp_server_app_init(DSmtpServerApp* app)
{
    g_message("init");
    // g_application_add_main_option_entries(G_APPLICATION(app),);
    app->server = d_smtp_server_new("127.0.0.1",8425);
}

static void d_smtp_server_app_class_init(DSmtpServerAppClass* klass)
{
    G_OBJECT_CLASS(klass)->finalize = d_smtp_server_app_finalize;
    G_APPLICATION_CLASS(klass)->activate = d_smtp_server_app_activate;
    G_APPLICATION_CLASS(klass)->command_line = d_smtp_server_app_command_line;
    G_APPLICATION_CLASS(klass)->startup = d_smtp_server_app_startup;
    G_APPLICATION_CLASS(klass)->shutdown = d_smtp_server_app_shutdown;
}
/**
 * @brief Create new instance of Mail Forward Service Application
 */
DSmtpServerApp* d_smtp_server_app_new()
{
    auto app = reinterpret_cast<DSmtpServerApp*>(
        g_object_new(D_TYPE_SMTP_SERVER_APP,
            "application-id","kz.gamma.vista.smtp.server",
            "flags",G_APPLICATION_HANDLES_COMMAND_LINE,
            NULL));

    return app;
}
}
