#ifndef _MUSIC_H_
#define _MUSIC_H_

typedef enum {
    MUSIC_RUN_LOCAL = 0,
    MUSIC_RUN_NET,
    MUSIC_RUN_UNKNOW = -1
} music_run_type;

extern music_run_type music_run;

void music_test_run(music_run_type type);
void music_go_local();
void music_go_net();

#endif