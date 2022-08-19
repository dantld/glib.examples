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
#ifndef __D__NEW__SMTP_COMMAND__HPP__
#define __D__NEW__SMTP_COMMAND__HPP__

/**
 * @brief SMTP command little object for validating SMTP command.
 */

#include <gio/gio.h>

enum SMTP_COMMAND
{
    SMTP_COMMAND_UNKNOWN,
    SMTP_COMMAND_HELO,
    SMTP_COMMAND_EHLO,
    SMTP_COMMAND_MAIL,
    SMTP_COMMAND_RCPT,
    SMTP_COMMAND_DATA,
    SMTP_COMMAND_QUIT,
};


extern "C" {
#define D_TYPE_SMTP_COMMAND (d_smtp_command_get_type())

G_DECLARE_FINAL_TYPE(DSmtpCommand,d_smtp_command,D,SMTP_COMMAND,GObject)

/**
 * @brief Set command bytes for process.
 * @param [in] smtp_command SMTP command object instance.
 * @param [in] command_bytes Input bytes with SMTP command.
 */
void d_smtp_command_set_bytes(
    DSmtpCommand* smtp_command,
    GBytes* command_bytes);

/**
 * @brief Process current command bytes.
 * @param [in] smtp_command SMTP command object instance.
 * @return In case of success function returns TRUE.
 */
gboolean d_smtp_command_process(
    DSmtpCommand* smtp_command);

/**
 * @brief Get enumeration type of command.
 * @note Function only valid after call d_smtp_command_process_command.
 * @return Return one of SMTP_COMMAND enumeration value.
 */
SMTP_COMMAND d_smtp_command_get_smtp_command(
    DSmtpCommand* command);

/**
 * @brief Get response code.
 * @note Function only valid after call d_smtp_command_process_command.
 * @return Return one of suitable for response value.
 */
gint d_smtp_command_get_response_code(
    DSmtpCommand* command);

/**
 * @brief Create new instance of SMTP command.
 */
DSmtpCommand* d_smtp_command_new();

}

#endif //#ifndef __D__NEW__SMTP_CONNECTION__HPP__
