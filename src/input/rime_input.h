#ifndef _LIBRIME_INPUT_H_
#define _LIBRIME_INPUT_H_

#include <rime_api.h>
#include <stdbool.h>

void rime_init();
void rime_start_session();
bool rime_put_key(uint8_t *key);
bool rime_get_commit(RimeCommit *commit);
void rime_close_commit(RimeCommit *commit);
bool rime_get_status(RimeStatus *status);
void rime_close_status(RimeStatus *status);
bool rime_get_context(RimeContext *context);
void rime_close_context(RimeContext *context);
void rime_highlight_candidate(int index);
void rime_close_session();
bool rime_change_page(bool backward);

#endif