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
/**
 * @brief SMTP state little object for processing SMTP FSM.
 */

#include "d_smtp_state.hpp"

extern "C" {

struct _DSmtpState
{
    GObject parent;
    SMTP_STATE state;
};
typedef _DSmtpState DSmtpState;

G_DEFINE_TYPE(DSmtpState,d_smtp_state,G_TYPE_OBJECT)

struct _DSmtpStateClass
{
    GObjectClass parent_class;
};


/**
 * @brief Get current state.
 * @return Return one of SMTP_STATE enumeration value.
 */
SMTP_STATE d_smtp_state_get_current_state(
    DSmtpState* smtp_state)
{
    return smtp_state->state;
}

static const gchar* smtp_state_to_text(SMTP_STATE state);

const gchar* d_smtp_state_get_current_state_text(
    DSmtpState* smtp_state)
{
    return smtp_state_to_text(smtp_state->state);
}

/**
 * @brief Switch to the new state by the write bytes complete.
 * @return Function returns TRUE if state successfully changed.
 */
gboolean d_smtp_state_next_by_write_complete(
    DSmtpState* smtp_state)
{
    g_message("current state %s",smtp_state_to_text(smtp_state->state));
    SMTP_STATE new_state = SMTP_STATE_ERROR;
    switch(smtp_state->state) {
    case SMTP_STATE_GREETING_SENDING:
        new_state = SMTP_STATE_GREETING_SENT;
        break;
    case SMTP_STATE_HELO_RECEIVED:
        new_state = SMTP_STATE_HELO_ACCEPTED;
        break;
    case SMTP_STATE_EHLO_RECEIVED:
        new_state = SMTP_STATE_EHLO_ACCEPTED;
        break;
    case SMTP_STATE_MAIL_RECEIVED:
        new_state = SMTP_STATE_MAIL_ACCEPTED;
        break;
    case SMTP_STATE_RCPT_RECEIVED:
        new_state = SMTP_STATE_RCPT_ACCEPTED;
        break;
    case SMTP_STATE_DATA_RECEIVED:
        new_state = SMTP_STATE_DATA_ACCEPTED;
        break;
    case SMTP_STATE_DATA_ENDED:
        new_state = SMTP_STATE_DATA_ENDED;
        break;
    case SMTP_STATE_QUIT_RECEIVED:
        new_state = SMTP_STATE_QUIT_ACCEPTED;
        break;
    case SMTP_STATE_QUIT_ACCEPTED:
        new_state = SMTP_STATE_CLOSE;
        break;
    default:
        g_warning("smtp state: unknown current state");
    }
    smtp_state->state = new_state;
    g_message("new state by write complete: %s",smtp_state_to_text(smtp_state->state));
    return smtp_state->state != SMTP_STATE_ERROR;
}
/**
 * @brief Switch to the new state by the command.
 * @return Function returns TRUE if state successfully changed.
 */
gboolean d_smtp_state_next_state_by_command(
    DSmtpState* smtp_state,
    SMTP_COMMAND command)
{
    g_message("old state: %s",smtp_state_to_text(smtp_state->state));
    SMTP_STATE new_state = SMTP_STATE_ERROR;
    switch(smtp_state->state) {
    case SMTP_STATE_GREETING_SENDING:
    case SMTP_STATE_GREETING_SENT:
        if(command == SMTP_COMMAND_HELO) new_state = SMTP_STATE_HELO_RECEIVED;
        else if(command == SMTP_COMMAND_EHLO) new_state = SMTP_STATE_EHLO_RECEIVED;
        break;
    case SMTP_STATE_HELO_ACCEPTED:
        if(command == SMTP_COMMAND_MAIL) new_state = SMTP_STATE_MAIL_RECEIVED;
        else if(command == SMTP_COMMAND_QUIT) new_state = SMTP_STATE_QUIT_ACCEPTED;
        break;
    case SMTP_STATE_EHLO_RECEIVED:
        if(command == SMTP_COMMAND_MAIL) new_state = SMTP_STATE_MAIL_RECEIVED;
        else if(command == SMTP_COMMAND_QUIT) new_state = SMTP_STATE_QUIT_ACCEPTED;
        break;
    case SMTP_STATE_MAIL_ACCEPTED:
        if(command == SMTP_COMMAND_RCPT) new_state = SMTP_STATE_RCPT_RECEIVED;
        else if(command == SMTP_COMMAND_QUIT) new_state = SMTP_STATE_QUIT_ACCEPTED;
        break;
    case SMTP_STATE_RCPT_ACCEPTED:
        if(command == SMTP_COMMAND_RCPT) new_state = SMTP_STATE_RCPT_RECEIVED;
        else if(command == SMTP_COMMAND_QUIT) new_state = SMTP_STATE_QUIT_ACCEPTED;
        else if(command == SMTP_COMMAND_DATA) new_state = SMTP_STATE_DATA_RECEIVED;
        break;
    case SMTP_STATE_DATA_ACCEPTED:
        break;
    case SMTP_STATE_DATA_ENDED:
        if(command == SMTP_COMMAND_QUIT) new_state = SMTP_STATE_QUIT_ACCEPTED;
        break;
    default:
        g_warning("smtp unknown state");
    }
    smtp_state->state = new_state;
    g_message("new state by command: %s",smtp_state_to_text(smtp_state->state));
    return smtp_state->state != SMTP_STATE_ERROR;
}

gboolean d_smtp_state_set_next_state(
    DSmtpState* smtp_state,
    SMTP_STATE state)
{
    smtp_state->state = state;
    return TRUE;
}

static const gchar* smtp_state_to_text(SMTP_STATE state)
{
    switch(state) {
    case SMTP_STATE_ERROR: return "ERROR";
    case SMTP_STATE_GREETING_SENDING: return "GREETING_SENDING";
    case SMTP_STATE_GREETING_SENT: return "GREETING_SENT";
    case SMTP_STATE_HELO_RECEIVED: return "HELO_RECEIVED";
    case SMTP_STATE_HELO_ACCEPTED: return "HELO_ACCEPTED";
    case SMTP_STATE_EHLO_RECEIVED: return "EHLO_RECEIVED";
    case SMTP_STATE_EHLO_ACCEPTED: return "EHLO_ACCEPTED";
    case SMTP_STATE_MAIL_RECEIVED: return "MAIL_RECEIVED";
    case SMTP_STATE_MAIL_ACCEPTED: return "MAIL_ACCEPTED";
    case SMTP_STATE_RCPT_RECEIVED: return "RCPT_RECEIVED";
    case SMTP_STATE_RCPT_ACCEPTED: return "RCPT_ACCEPTED";
    case SMTP_STATE_DATA_RECEIVED: return "DATA_RECEIVED";
    case SMTP_STATE_DATA_ACCEPTED: return "DATA_ACCEPTED";
    case SMTP_STATE_DATA_ENDED: return "DATA_ENDED";
    case SMTP_STATE_QUIT_RECEIVED: return "QUIT_RECEIVED";
    case SMTP_STATE_QUIT_ACCEPTED: return "QUIT_ACCEPTED";
    case SMTP_STATE_CLOSE: return "CLOSE";
    default: return "UNKNOWN";
    }
}

static void d_smtp_state_init(DSmtpState* smtp_state)
{
    smtp_state->state = SMTP_STATE_ERROR;
}

static void d_smtp_state_class_init(DSmtpStateClass* klass)
{
}

/**
 * @brief Create new instance of SMTP FSM.
 */
DSmtpState* d_smtp_state_new()
{
    auto smtp_state = reinterpret_cast<DSmtpState*>(
        g_object_new(
            D_TYPE_SMTP_STATE,
            NULL));

    return smtp_state;
}

}
