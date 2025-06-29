#include "rime_input.h"

#include "lvgl/src/misc/lv_log.h"

#include <rime_api.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define RIME_DATA_DIR "data"
#define RIME_LOG_DIR "log"
#define RIME_BUILD_DIR "build"

#define RIME_APP_NAME "ColorAudio"

bool rime_run = false;

static RimeApi *rime_api = NULL;
static RimeSessionId rime_session = 0;

static void check_dir(uint8_t *data)
{
    if (NULL == opendir(data))
    {
        mkdir(data, 0775);
    }
}

static void rime_handel(void *context_object, RimeSessionId session_id,
                        const char *message_type, const char *message_value)
{
    LV_LOG_USER("rime message: [%lx] [%s] [%s]", session_id, message_type, message_value);
}

static void *rime_init_run(void *arg)
{
    if (rime_api->start_maintenance(true))
    {
        rime_api->join_maintenance_thread();
    }

    rime_run = true;
}

void rime_init()
{
    check_dir(RIME_DATA_DIR);
    check_dir(RIME_LOG_DIR);
    check_dir(RIME_BUILD_DIR);

    RIME_STRUCT(RimeTraits, traits);

    traits.app_name = RIME_APP_NAME;
    traits.min_log_level = 0;
    traits.shared_data_dir = RIME_DATA_DIR;
    traits.user_data_dir = RIME_DATA_DIR;
    traits.prebuilt_data_dir = RIME_BUILD_DIR;
    traits.staging_dir = RIME_BUILD_DIR;
    traits.log_dir = RIME_LOG_DIR;

    rime_api = rime_get_api();

    rime_api->setup(&traits);
    rime_api->set_notification_handler(rime_handel, NULL);
    rime_api->initialize(&traits);

    pthread_t sid;
    int res = pthread_create(&sid, NULL, rime_init_run, NULL);
    if (res)
    {
        LV_LOG_ERROR("Rime init thread run fail: %d", res);
    }
}

void rime_start_session()
{
    if (rime_session != 0)
    {
        rime_close_session();
    }
    rime_session = rime_api->create_session();
}

bool rime_put_key(uint8_t *key)
{
    return rime_api->simulate_key_sequence(rime_session, key);
}

bool rime_get_commit(RimeCommit *commit)
{
    return rime_api->get_commit(rime_session, commit);
}

void rime_close_commit(RimeCommit *commit)
{
    rime_api->free_commit(commit);
}

bool rime_get_status(RimeStatus *status)
{
    return rime_api->get_status(rime_session, status);
}

void rime_close_status(RimeStatus *status)
{
    rime_api->free_status(status);
}

bool rime_get_context(RimeContext *context)
{
    return rime_api->get_context(rime_session, context);
}

void rime_close_context(RimeContext *context)
{
    rime_api->free_context(context);
}

void rime_highlight_candidate(int index)
{
    rime_api->highlight_candidate_on_current_page(rime_session, index);
}

void rime_close_session()
{
    rime_api->destroy_session(rime_session);
    rime_session = 0;
}

bool rime_change_page(bool backward)
{  
    return rime_api->change_page(rime_session, backward);
}