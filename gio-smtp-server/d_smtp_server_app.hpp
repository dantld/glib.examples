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
#ifndef __D__NEW__SMTP_SERVER__APPLICATION__HPP__
#define __D__NEW__SMTP_SERVER__APPLICATION__HPP__

#include <gio/gio.h>

extern "C" {
#define D_TYPE_SMTP_SERVER_APP (d_smtp_server_app_get_type())

G_DECLARE_FINAL_TYPE(DSmtpServerApp,d_smtp_server_app,D,SMTP_SERVER_APP,GApplication)

/**
 * @brief Create new instance of SMTP server application.
 */
DSmtpServerApp* d_smtp_server_app_new();

}

#endif //#ifndef __D__NEW__MAIL_FORWARD_SERVICE__APPLICATION__HPP__
