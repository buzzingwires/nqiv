#include "typedefs.h"

#include <inttypes.h>
#include <string.h>
#include <assert.h>

#include "event.h"

const char* const nqiv_event_priority_names[] = {
	"quit",
	"image_load_animation",
	"reattempt_thumbnail",
	"prune",
	"image_load",
	"thumbnail_load_ephemeral",
	"thumbnail_load",
	"thumbnail_save_load_fail",
	"thumbnail_save_load_no",
	"preload_image_load",
	"preload_thumbnail_load_ephemeral",
	"preload_thumbnail_load",
	"preload_thumbnail_save_load_fail",
	"preload_thumbnail_save_load_no",
};

const char* const nqiv_event_priority_descriptions[] = {
	"Stop the thread.",
	"Load frames of an animation.",
	"Reload thumbnails- usually if the size changes or something.",
	"Prune images.",
	"Load a displayed image.",
	"Load thumbnail from existing image data.",
	"Load thumbnail from disc.",
	"Create thumbnail after failing to load it from disc.",
	"Create thumbnail we don't actually intend to view.",
	"Lower priority variant of 'image_load' for preloading.",
	"Lower priority variant of 'thumbnail_load_ephemeral' for preloading.",
	"Lower priority variant of 'thumbnail_load' for preloading.",
	"Lower priority variant of 'thumbnail_save_load_fail' for preloading.",
	"Lower priority variant of 'thumbnail_save_load_no' for preloading.",
};

nqiv_event_priority nqiv_text_to_event_priority(const char* text, const int length)
{
	nqiv_event_priority priority = NQIV_EVENT_PRIORITY_UNKNOWN;
	for(priority = NQIV_EVENT_PRIORITY_FIRST; priority <= NQIV_EVENT_PRIORITY_LAST; ++priority) {
		if(strncmp(text, nqiv_event_priority_names[priority], length) == 0
		   && (size_t)length == strlen(nqiv_event_priority_names[priority])) {
			return priority;
		}
	}
	return NQIV_EVENT_PRIORITY_UNKNOWN;
}

void nqiv_cond_destroy(nqiv_cond* cond)
{
	assert(cond->cond != NULL);
	assert(cond->lock != NULL);
	SDL_DestroyCond(cond->cond);
	SDL_DestroyMutex(cond->lock);
	memset(cond, 0, sizeof(nqiv_cond));
}

bool nqiv_cond_init(nqiv_cond* cond)
{
	assert(cond->cond == NULL);
	assert(cond->lock == NULL);
	cond->cond = SDL_CreateCond();
	if(cond->cond == NULL) {
		return false;
	}
	cond->lock = SDL_CreateMutex();
	if(cond->lock == NULL) {
		SDL_DestroyCond(cond->cond);
		memset(cond, 0, sizeof(nqiv_cond));
		return false;
	}
	return true;
}

void nqiv_cond_wait(nqiv_cond* cond)
{
	assert(cond->cond != NULL);
	assert(cond->lock != NULL);
	SDL_LockMutex(cond->lock);
	SDL_CondWait(cond->cond, cond->lock);
	SDL_UnlockMutex(cond->lock);
}

void nqiv_cond_wake_all(nqiv_cond* cond)
{
	assert(cond->cond != NULL);
	assert(cond->lock != NULL);
	SDL_CondBroadcast(cond->cond);
}
