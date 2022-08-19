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
#ifndef __D__NEW__SMTP_STATE__HPP__
#define __D__NEW__SMTP_STATE__HPP__
/**
 * @brief SMTP state little object for processing SMTP FSM.
 */

#include <gio/gio.h>
#include "d_smtp_command.hpp"

enum SMTP_STATE
{
    SMTP_STATE_ERROR,
    SMTP_STATE_GREETING_SENDING,
    SMTP_STATE_GREETING_SENT,
    SMTP_STATE_HELO_RECEIVED,
    SMTP_STATE_HELO_ACCEPTED,
    SMTP_STATE_EHLO_RECEIVED,
    SMTP_STATE_EHLO_ACCEPTED,
    SMTP_STATE_MAIL_RECEIVED,
    SMTP_STATE_MAIL_ACCEPTED,
    SMTP_STATE_RCPT_RECEIVED,
    SMTP_STATE_RCPT_ACCEPTED,
    SMTP_STATE_DATA_RECEIVED,
    SMTP_STATE_DATA_ACCEPTED,
    SMTP_STATE_DATA_ENDED,
    SMTP_STATE_QUIT_RECEIVED,
    SMTP_STATE_QUIT_ACCEPTED,
    SMTP_STATE_CLOSE
};

extern "C" {
#define D_TYPE_SMTP_STATE (d_smtp_state_get_type())

G_DECLARE_FINAL_TYPE(DSmtpState,d_smtp_state,D,SMTP_STATE,GObject)

/**
 * @brief Get current state.
 * @return Return one of SMTP_STATE enumeration value.
 */
SMTP_STATE d_smtp_state_get_current_state(
    DSmtpState* smtp_state);

/**
 * @brief @brief Get current state as text string.
 */
const gchar* d_smtp_state_get_current_state_text(
    DSmtpState* smtp_state);

/**
 * @brief Switch to the new state by the write bytes complete.
 * @return Function returns TRUE if state successfully changed.
 */
gboolean d_smtp_state_next_by_write_complete(
    DSmtpState* smtp_state);

/**
 * @brief Switch to the new state by the command.
 * @return Function returns TRUE if state successfully changed.
 */
gboolean d_smtp_state_next_state_by_command(
    DSmtpState* smtp_state,
    SMTP_COMMAND command);

/**
 * @brief Switch to the new state by the new state.
 * @return Function returns TRUE if state successfully changed.
 */
gboolean d_smtp_state_set_next_state(
    DSmtpState* smtp_state,
    SMTP_STATE state);

/**
 * @brief Create new instance of SMTP FSM.
 */
DSmtpState* d_smtp_state_new();

}

#endif //#ifndef __D__NEW__SMTP_STATE__HPP__
