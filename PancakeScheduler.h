#ifndef _PANCAKE_SCHEDULER_H
#define _PANCAKE_SCHEDULER_H

#include "Pancake.h"

typedef void (*PancakeSchedulerEventCallback)(void *arg);

typedef struct _PancakeSchedulerEvent {
	UNative time;
	PancakeSchedulerEventCallback callback;
	void *arg;

	struct _PancakeSchedulerEvent *prev;
	struct _PancakeSchedulerEvent *next;
} PancakeSchedulerEvent;

PANCAKE_API PancakeSchedulerEvent *PancakeSchedule(UNative time, PancakeSchedulerEventCallback callback, void *arg);
PANCAKE_API void PancakeUnschedule(PancakeSchedulerEvent *event);
PANCAKE_API UNative PancakeSchedulerGetNextExecutionTime(); /* returns >= time() */
PANCAKE_API UNative PancakeSchedulerGetNextExecutionTimeOffset(); /* return >= 0 */
PANCAKE_API UNative PancakeSchedulerGetNextScheduledTime(); /* returns actual scheduled time (can be < time()) */
PANCAKE_API void PancakeSchedulerRun();

void PancakeSchedulerShutdown();

#endif
