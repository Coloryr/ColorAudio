#include "music.h"

#include <stdint.h>
#include <pthread.h>

// pthread_mutex_t play_mutex;
// pthread_cond_t play_start;



void music_test_run(music_run_type type)
{
    if (type == music_run)
    {
        return;
    }


}

void music_go_local()
{
    music_run = MUSIC_RUN_LOCAL;
}

void music_go_net()
{
    music_run = MUSIC_RUN_NET;
}

void music_init()
{
    // pthread_mutex_init(&play_mutex, NULL);
    // pthread_cond_init(&play_start, NULL);
}
