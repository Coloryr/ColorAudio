/**
 * @file lv_pthread.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#define __USE_GNU
#define _GNU_SOURCE
#define __GNU_SOURCE
#include "lv_os.h"

#if LV_USE_OS == LV_OS_PTHREAD

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#define gettid() syscall(SYS_gettid)

#include <sys/syscall.h>

#include "../misc/lv_log.h"

#ifndef __linux__
    #include "../misc/lv_timer.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void * generic_callback(void * user_data);


/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static int thread_bind_cpu(int target_cpu)
{
   cpu_set_t mask;
   int cpu_num = sysconf(_SC_NPROCESSORS_CONF);
   int i;

   if (target_cpu >= cpu_num)
      return -1;

   CPU_ZERO(&mask);
   CPU_SET(target_cpu, &mask);

   if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
      perror("pthread_setaffinity_np");

   if (pthread_getaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
      perror("pthread_getaffinity_np");

   printf("Thread(%ld) bound to cpu:", gettid());
   for (i = 0; i < CPU_SETSIZE; i++) {
      if (CPU_ISSET(i, &mask))
         printf(" %d", i);
   }
   printf("\n");

   return i >= cpu_num ? -1 : i;
}

static int thread_id = 0;

lv_result_t lv_thread_init(lv_thread_t * thread, const char * const name,
                           lv_thread_prio_t prio, void (*callback)(void *),
                           size_t stack_size, void * user_data)
{
    LV_UNUSED(name);
    LV_UNUSED(prio);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stack_size);
    thread->callback = callback;
    thread->user_data = user_data;
    thread->id = thread_id++;
    pthread_create(&thread->thread, &attr, generic_callback, thread);
    pthread_attr_destroy(&attr);
    return LV_RESULT_OK;
}

lv_result_t lv_thread_delete(lv_thread_t * thread)
{
    int ret = pthread_join(thread->thread, NULL);
    if(ret != 0) {
        LV_LOG_WARN("Error: %d", ret);
        return LV_RESULT_INVALID;
    }

    return LV_RESULT_OK;
}

lv_result_t lv_mutex_init(lv_mutex_t * mutex)
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    int ret = pthread_mutex_init(mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    if(ret) {
        LV_LOG_WARN("Error: %d", ret);
        return LV_RESULT_INVALID;
    }
    else {
        return LV_RESULT_OK;
    }
}

lv_result_t lv_mutex_lock(lv_mutex_t * mutex)
{
    int ret = pthread_mutex_lock(mutex);
    if(ret) {
        LV_LOG_WARN("Error: %d", ret);
        return LV_RESULT_INVALID;
    }
    else {
        return LV_RESULT_OK;
    }
}

lv_result_t lv_mutex_lock_isr(lv_mutex_t * mutex)
{
    int ret = pthread_mutex_lock(mutex);
    if(ret) {
        LV_LOG_WARN("Error: %d", ret);
        return LV_RESULT_INVALID;
    }
    else {
        return LV_RESULT_OK;
    }
}

lv_result_t lv_mutex_unlock(lv_mutex_t * mutex)
{
    int ret = pthread_mutex_unlock(mutex);
    if(ret) {
        LV_LOG_WARN("Error: %d", ret);
        return LV_RESULT_INVALID;
    }
    else {
        return LV_RESULT_OK;
    }
}

lv_result_t lv_mutex_delete(lv_mutex_t * mutex)
{
    pthread_mutex_destroy(mutex);
    return LV_RESULT_OK;
}

lv_result_t lv_thread_sync_init(lv_thread_sync_t * sync)
{
    pthread_mutex_init(&sync->mutex, 0);
    pthread_cond_init(&sync->cond, 0);
    sync->v = false;
    return LV_RESULT_OK;
}

lv_result_t lv_thread_sync_wait(lv_thread_sync_t * sync)
{
    pthread_mutex_lock(&sync->mutex);
    while(!sync->v) {
        pthread_cond_wait(&sync->cond, &sync->mutex);
    }
    sync->v = false;
    pthread_mutex_unlock(&sync->mutex);
    return LV_RESULT_OK;
}

lv_result_t lv_thread_sync_signal(lv_thread_sync_t * sync)
{
    pthread_mutex_lock(&sync->mutex);
    sync->v = true;
    pthread_cond_signal(&sync->cond);
    pthread_mutex_unlock(&sync->mutex);

    return LV_RESULT_OK;
}

lv_result_t lv_thread_sync_delete(lv_thread_sync_t * sync)
{
    pthread_mutex_destroy(&sync->mutex);
    pthread_cond_destroy(&sync->cond);
    return LV_RESULT_OK;
}

lv_result_t lv_thread_sync_signal_isr(lv_thread_sync_t * sync)
{
    LV_UNUSED(sync);
    return LV_RESULT_INVALID;
}

#ifndef __linux__
uint32_t lv_os_get_idle_percent(void)
{
    return lv_timer_get_idle();
}
#endif


/**********************
 *   STATIC FUNCTIONS
 **********************/

static void * generic_callback(void * user_data)
{
    lv_thread_t * thread = user_data;

    /* HACK, bind thread to CPU in order */
    int cpu_num = sysconf(_SC_NPROCESSORS_CONF);
    int id = thread->id;
    id %= cpu_num;
    thread_bind_cpu(id);

    thread->callback(thread->user_data);
    return NULL;
}


#endif /*LV_USE_OS == LV_OS_PTHREAD*/
