#include "PancakeScheduler.h"

static PancakeSchedulerEvent *events = NULL; /* waiting events */
static PancakeSchedulerEvent *unusedEvents = NULL; /* cached event instances */

PANCAKE_API PancakeSchedulerEvent *PancakeSchedule(UNative time, PancakeSchedulerEventCallback callback, void *arg) {
	PancakeSchedulerEvent *event;

	if(unusedEvents) {
		// Use cached event instance
		event = unusedEvents;

		CDL_DELETE(unusedEvents, unusedEvents);
	} else {
		// Allocate new event
		event = PancakeAllocate(sizeof(PancakeSchedulerEvent));
	}

	event->time = time;
	event->callback = callback;
	event->arg = arg;

	if(events) {
		if(events->prev->time <= time) {
			CDL_PREPEND_ELEM(events, events, event);

			events = event->next;
		} else {
			PancakeSchedulerEvent *ev;

			// Iterate through events and find place for new event
			CDL_FOREACH(events, ev) {
				if(ev->time == time) {
					CDL_PREPEND_ELEM(events, ev->next, event);
					break;
				} else if(ev->time < time && ev->next->time >= time) {
					CDL_PREPEND_ELEM(events, ev->next, event);
					break;
				}
			}
		}
	} else {
		CDL_PREPEND(events, event);
	}

	PancakeDebug {
		// Check for event order corruption
		PancakeSchedulerEvent *ev;

		CDL_FOREACH(events, ev) {
			if(ev->next != events) {
				PancakeAssert(ev->time <= ev->next->time);
			}
		}
	}

	return event;
}

PANCAKE_API inline void PancakeUnschedule(PancakeSchedulerEvent *event) {
	CDL_DELETE(events, event);
	CDL_PREPEND(unusedEvents, event);
}

PANCAKE_API inline UNative PancakeSchedulerGetNextExecutionTime() {
	if(!events) {
		return time(NULL) + 86400;
	}

	return events->time < time(NULL) ? time(NULL) : events->time;
}

PANCAKE_API inline UNative PancakeSchedulerGetNextScheduledTime() {
	if(!events) {
		return 0;
	}

	return events->time;
}

PANCAKE_API void PancakeSchedulerRun() {
	PancakeSchedulerEvent *ev, *tmp, *tmp2;
	UNative now = time(NULL);

	CDL_FOREACH_SAFE(events, ev, tmp, tmp2) {
		if(ev->time > now) {
			break;
		}

		// Run event
		ev->callback(ev->arg);

		// Cache event instance
		CDL_DELETE(events, ev);
		CDL_PREPEND(unusedEvents, ev);
	}
}

void PancakeSchedulerShutdown() {
	PancakeSchedulerEvent *ev, *tmp, *tmp2;

	CDL_FOREACH_SAFE(events, ev, tmp, tmp2) {
		// Run all events, no matter when they are scheduled
		ev->callback(ev->arg);

		CDL_DELETE(events, ev);
		PancakeFree(ev);
	}

	// Free cached events
	CDL_FOREACH_SAFE(unusedEvents, ev, tmp, tmp2) {
		CDL_DELETE(unusedEvents, ev);
		PancakeFree(ev);
	}
}
