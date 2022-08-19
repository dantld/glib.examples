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
#include "d_smtp_command.hpp"

#define CRLF "\r\n"

extern "C" {

struct _DSmtpCommand
{
    GObject parent;

    SMTP_COMMAND command;
    guint response_code;

    GBytes* command_bytes;    
};
typedef _DSmtpCommand DSmtpCommand;

G_DEFINE_TYPE(DSmtpCommand,d_smtp_command,G_TYPE_OBJECT)

struct _DSmtpCommandClass
{
    GObjectClass parent;
};

/**
 * @brief Eat token at the begin of the sequence.
 * @details Eat token from the begin but until the end.
 * @param [in] begin The begin of sequence.
 * @param [in] end The end of sequence.
 * @param [in] word The token to find.
 * @return In case of succes the function returns the pointer after the word.
 * If word not found return NULL.
 */
static const gchar* eat_tok(const gchar* begin,const gchar* end,const gchar* word)
{
    if(!begin) return nullptr;
    gsize length = strlen(word);
    gsize index = 0;
    for(; index < length; index++) {
        if(*begin != word[index]) break;
        if(begin == end) break;
        begin++;
    }
    return index == length ? begin : nullptr;
}
/**
 * @brief Find the token by scaning the sequence from the begin until end.
 * @param [in] begin The begin of sequence.
 * @param [in] end The end of sequence.
 * @param [in] word The token to find.
 */
static const gchar* find_until(const gchar* begin,const gchar* end,const gchar* word)
{
    for(; *begin != *word && begin != end; begin++);
    if(begin == end) return nullptr;
    return eat_tok(begin,end,word);
}
/**
 * @brief Find the token between two sentry tokens.
 * @details Try to find the first - wordb, then second - worde locations.
 * After success search of two pointers retrieve the contents between them.
 * @param [in] begin The begin of sequence.
 * @param [in] end The end of sequence.
 * @param [in] wordb The begin sentry token to find.
 * @param [in] worde The end sentry token to find.
 * @param [out] token The token between sentries. Caller should free parameter.
 * @returns In case of success function return non-NULL pointer which points to
 * the next symbol after the worde. Token will be allocated to the new string
 * with contents between last symbol of wordb and the first symbol of worde.
 * If none of wordb or worde hasn't found the function returns with NULL pointer
 * and output parameter token stay unallocated.
 */
static const gchar* get_tok(const gchar* begin,const gchar* end,const gchar* wordb,const gchar* worde,gchar** token)
{
    if(!begin) return nullptr;
    const gchar* p1 = eat_tok(begin,end,wordb);
    if(!p1) return nullptr;
    const gchar* p2 = find_until(p1,end,worde);
    if(!p2) return nullptr;
    p2--;
    auto token_string = g_string_new_len(p1,p2-p1);
    *token = g_string_free(token_string,FALSE);
    return p2;
}

static gboolean d_smtp_command_process_command_helo(
    DSmtpCommand* smtp_command,
    SMTP_COMMAND command)
{
    gsize count{0};
    auto line = reinterpret_cast<const gchar*>(g_bytes_get_data(smtp_command->command_bytes,&count));
    auto end = strstr(line,CRLF);
    const gchar* p = line + 4;
    for(; *p == ' '; p++);
    gsize l = end - p;
    auto str = g_string_new_len(p,end-p);
    g_autofree gchar* domain = g_string_free(str,FALSE);
    g_message("HELO from \"%s\"",domain);
    smtp_command->command = SMTP_COMMAND_HELO;
    smtp_command->response_code = 250;
    return TRUE;
}

static gboolean d_smtp_command_process_command_ehlo(
    DSmtpCommand* smtp_command,
    SMTP_COMMAND command)
{
    gsize count{0};
    reinterpret_cast<const gchar*>(g_bytes_get_data(smtp_command->command_bytes,&count));
    return FALSE;
}

static gboolean d_smtp_command_process_command_mail(
    DSmtpCommand* smtp_command,
    SMTP_COMMAND command)
{
    gsize count{0};
    auto line = reinterpret_cast<const gchar *>(
        g_bytes_get_data(smtp_command->command_bytes, &count));
    auto end = strstr(line,CRLF);
    const gchar* begin = line;

    gchar* token{nullptr};
    begin = eat_tok(begin,end,"MAIL");
    begin = eat_tok(begin,end," ");
    begin = eat_tok(begin,end,"FROM");
    begin = eat_tok(begin,end,":");
    begin = get_tok(begin,end,"<",">",&token);

    if(begin) {
        smtp_command->command = SMTP_COMMAND_MAIL;
        smtp_command->response_code = 250;
        g_message("mail from token \"%s\"",token);
        g_free(token);
        return TRUE;
    }
    return FALSE;
}

static gboolean d_smtp_command_process_command_rcpt(
    DSmtpCommand* smtp_command,
    SMTP_COMMAND command)
{
    gsize count{0};
    auto line = reinterpret_cast<const gchar *>(
        g_bytes_get_data(smtp_command->command_bytes, &count));
    auto end = strstr(line,CRLF);
    const gchar* begin = line;

    gchar* token{nullptr};
    begin = eat_tok(begin,end,"RCPT");
    begin = eat_tok(begin,end," ");
    begin = eat_tok(begin,end,"TO");
    begin = eat_tok(begin,end,":");
    begin = get_tok(begin,end,"<",">",&token);

    if(begin) {
        smtp_command->command = SMTP_COMMAND_RCPT;
        smtp_command->response_code = 250;
        g_message("mail from token \"%s\"",token);
        g_free(token);
        return TRUE;
    }
    return FALSE;
}

static gboolean d_smtp_command_process_command_data(
    DSmtpCommand* smtp_command,
    SMTP_COMMAND command)
{
    gsize count{0};
    auto line = reinterpret_cast<const gchar *>(
        g_bytes_get_data(smtp_command->command_bytes, &count));
    auto end = strstr(line,CRLF);
    const gchar* begin = line;

    gchar* token{nullptr};
    begin = eat_tok(begin,end,"DATA");
    if(begin) {
        smtp_command->command = SMTP_COMMAND_DATA;
        smtp_command->response_code = 354;
        return TRUE;
    }
    return FALSE;
}

static gboolean d_smtp_command_process_command_quit(
    DSmtpCommand* smtp_command,
    SMTP_COMMAND command)
{
    gsize count{0};
    auto line = reinterpret_cast<const gchar *>(
        g_bytes_get_data(smtp_command->command_bytes, &count));
    auto end = strstr(line,CRLF);
    const gchar* begin = line;

    gchar* token{nullptr};
    begin = eat_tok(begin,end,"QUIT");
    if(begin) {
        smtp_command->command = SMTP_COMMAND_QUIT;
        smtp_command->response_code = 221;
        return TRUE;
    }
    return FALSE;
}
/**
 * @brief SMTP command process command.
 */
static gboolean d_smtp_command_process_command(
    DSmtpCommand* smtp_command,
    SMTP_COMMAND command)
{
    switch(command) {
    case SMTP_COMMAND_HELO:
        return d_smtp_command_process_command_helo(smtp_command,command);
    case SMTP_COMMAND_EHLO:
        return d_smtp_command_process_command_ehlo(smtp_command,command);
    case SMTP_COMMAND_MAIL:
        return d_smtp_command_process_command_mail(smtp_command,command);
    case SMTP_COMMAND_RCPT:
        return d_smtp_command_process_command_rcpt(smtp_command,command);
    case SMTP_COMMAND_DATA:
        return d_smtp_command_process_command_data(smtp_command,command);
    case SMTP_COMMAND_QUIT:
        return d_smtp_command_process_command_quit(smtp_command,command);
    }
    return FALSE;
}

void d_smtp_command_set_bytes(
    DSmtpCommand* smtp_command,
    GBytes* command_bytes)
{
    if(smtp_command->command_bytes) {
        g_bytes_unref(smtp_command->command_bytes);
    }
    smtp_command->command_bytes = command_bytes;
    g_bytes_ref(smtp_command->command_bytes);
}

gboolean d_smtp_command_process(
    DSmtpCommand* smtp_command)
{
    if(!smtp_command->command_bytes) {
        g_warning("SMTP command: process failed no command bytes");
        return FALSE;
    }
    gsize count{0};
    auto buffer = reinterpret_cast<const char *>(
        g_bytes_get_data(smtp_command->command_bytes, &count));
    g_message("PROCESSING: [%.*s]",(int)count,buffer);
    // Test for minimal and maximum command length requirements.
    if(count < 4 || count > 1024) {
        g_warning("command length less than four or more than 1024 symbols");
        return FALSE;
    }
    // Test for valid commands.
    gchar cmd[5] = {0};
    strncpy(cmd,buffer,4)[4] = 0;
    SMTP_COMMAND command = SMTP_COMMAND_UNKNOWN;
    if(g_str_equal(cmd,"QUIT")) {
        command = SMTP_COMMAND_QUIT;
    } else if(g_str_equal(cmd,"HELO")) {
        command = SMTP_COMMAND_HELO;
    } else if(g_str_equal(cmd,"EHLO")) {
        command = SMTP_COMMAND_EHLO;
    } else if(g_str_equal(cmd,"MAIL")) {
        command = SMTP_COMMAND_MAIL;
    } else if(g_str_equal(cmd,"RCPT")) {
        command = SMTP_COMMAND_RCPT;
    } else if(g_str_equal(cmd,"DATA")) {
        command = SMTP_COMMAND_DATA;
    } else {
        g_warning("unknown command %s",cmd);
        return FALSE;
    }

    return d_smtp_command_process_command(smtp_command,command);
}

SMTP_COMMAND d_smtp_command_get_smtp_command(
    DSmtpCommand* smtp_command)
{
    return smtp_command->command;
}

gint d_smtp_command_get_response_code(
    DSmtpCommand* smtp_command)
{
    return smtp_command->response_code;
}

static void d_smtp_command_finalize(GObject* object)
{
    auto smtp_command = D_SMTP_COMMAND(object);
    if(smtp_command->command_bytes) {
        g_bytes_unref(smtp_command->command_bytes);
    }
    G_OBJECT_CLASS(d_smtp_command_parent_class)->finalize(object);
}

static void d_smtp_command_init(DSmtpCommand* smtp_command)
{
    smtp_command->command = SMTP_COMMAND_UNKNOWN;
}

static void d_smtp_command_class_init(DSmtpCommandClass* klass)
{
    auto object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = d_smtp_command_finalize;
}

/**
 * @brief Create new instance of SMTP command.
 * @param [in] command_bytes Input bytes with SMTP command.
 */
DSmtpCommand* d_smtp_command_new()
{
    auto smtp_command = reinterpret_cast<DSmtpCommand*>(
        g_object_new(
            D_TYPE_SMTP_COMMAND,
            NULL
        ));
    return smtp_command;
}

}
