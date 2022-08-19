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
#ifndef __D__NEW__SMTP_SERVER__HPP__
#define __D__NEW__SMTP_SERVER__HPP__

#include <gio/gio.h>

extern "C" {
#define D_TYPE_SMTP_SERVER (d_smtp_server_get_type())

G_DECLARE_FINAL_TYPE(DSmtpServer,d_smtp_server,D,SMTP_SERVER,GObject)

void d_smtp_server_start(DSmtpServer*);

void d_smtp_server_stop(DSmtpServer*);

/**
 * @brief Create new instance of SMTP server.
 */
DSmtpServer* d_smtp_server_new(
    const gchar* listen_address,
    guint listen_port);

}

#endif //#ifndef __D__NEW__SMTP_SERVER__HPP__
