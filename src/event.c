#include "typedefs.h"

#include <inttypes.h>
#include <string.h>
#include <assert.h>

#include "event.h"

const char* const nqiv_event_priority_names[] = {
	"animation",
	"unload",
	"prune",
	"image",
	"thumbnail_load",
	"thumbnail_save",
	"preload_image",
	"preload_thumbnail_load",
	"preload_thumbnail_save",
};

const char* const nqiv_event_priority_descriptions[] = {
	"Load frames of an animation.",
	"Request unloading of images.",
	"Automatically prune images.",
	"Load a displayed image.",
	"Load a montage image.",
	"Save a montage thumbnail.",
	"Preload image not currently displayed.",
	"Preload montage image not currently displayed.",
	"Save montage thumbnail not currently displayed.",
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
