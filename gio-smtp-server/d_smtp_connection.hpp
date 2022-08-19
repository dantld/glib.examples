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
#ifndef __D__NEW__SMTP_CONNECTION__HPP__
#define __D__NEW__SMTP_CONNECTION__HPP__

#include <gio/gio.h>

extern "C" {
#define D_TYPE_SMTP_CONNECTION (d_smtp_connection_get_type())

G_DECLARE_FINAL_TYPE(DSmtpConnection,d_smtp_connection,D,SMTP_CONNECTION,GObject)

/**
 * @brief Close SMTP connection.
 */
void d_smtp_connection_close(DSmtpConnection* connection);

/**
 * @brief Get SMTP connection read operation timeout value.
 */
guint d_smtp_connection_get_read_timeout(
    DSmtpConnection* connection);

/**
 * @brief Get SMTP connection write operation timeout value.
 */
guint d_smtp_connection_get_write_timeout(
    DSmtpConnection* connection);

/**
 * @brief Get SMTP connection close operation timeout value.
 */
guint d_smtp_connection_get_close_timeout(
    DSmtpConnection* connection);

/**
 * @brief Set SMTP connection read operation timeout value.
 */
void d_smtp_connection_set_read_timeout(
    DSmtpConnection* connection,
    guint timeout_value);

/**
 * @brief Set SMTP connection write operation timeout value.
 */
void d_smtp_connection_set_write_timeout(
    DSmtpConnection* connection,
    guint timeout_value);

/**
 * @brief Set SMTP connection close operation timeout value.
 */
void d_smtp_connection_set_close_timeout(
    DSmtpConnection* connection,
    guint timeout_value);

/**
 * @brief Create new instance of SMTP connection.
 * @details Create new instance of SMTP connection and immediately
 * send the invitation with response code 220. All timeouts set to default
 * values to 10 seconds.
 * @param [in] smtp_client_socket The new listsner accepted socket.
 */
DSmtpConnection* d_smtp_connection_new(
    GSocket* smtp_client_socket);

/**
 * @brief Create new instance of SMTP connection with prediefined timeout values.
 * @details Create new instance of SMTP connection and immediately
 * send the invitation with response code 220. All timeouts set to requested
 * values.
 * @param [in] smtp_client_socket The new listsner accepted socket.
 * @param [in] read_timeout The new value for reading timeout.
 * @param [in] write_timeout The new value for write timeout.
 * @param [in] close_timeout The new value for close timeout.
 */
DSmtpConnection* d_smtp_connection_new_with_timeouts(
    GSocket* smtp_client_socket,
    guint read_timeout,
    guint write_timeout,
    guint close_timeout);
}

#endif //#ifndef __D__NEW__SMTP_CONNECTION__HPP__
