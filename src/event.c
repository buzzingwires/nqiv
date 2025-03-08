#include "typedefs.h"

#include <inttypes.h>
#include <string.h>

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

bool nqiv_shared_var_init(nqiv_shared_var* var)
{
	memset(var, 0, sizeof(nqiv_shared_var));
	var->lock = SDL_CreateMutex();
	return var->lock != NULL;
}

void nqiv_shared_var_destroy(nqiv_shared_var* var)
{
	if(var->lock != NULL) {
		SDL_DestroyMutex(var->lock);
	}
	memset(var, 0, sizeof(nqiv_shared_var));
}

void nqiv_shared_var_lock(nqiv_shared_var* var)
{
	SDL_LockMutex(var->lock);
}

void nqiv_shared_var_unlock(nqiv_shared_var* var)
{
	SDL_UnlockMutex(var->lock);
}

void nqiv_shared_var_set_op_result(nqiv_shared_var* var, const nqiv_op_result value)
{
	nqiv_shared_var_lock(var);
	var->data.as_op_result = value;
	nqiv_shared_var_unlock(var);
}

nqiv_op_result nqiv_shared_var_get_op_result(nqiv_shared_var* var)
{
	nqiv_shared_var_lock(var);
	const nqiv_op_result result = var->data.as_op_result;
	nqiv_shared_var_unlock(var);
	return result;
}

void nqiv_shared_var_inc_int(nqiv_shared_var* var)
{
	nqiv_shared_var_lock(var);
	var->data.as_int += 1;
	nqiv_shared_var_unlock(var);
}

void nqiv_shared_var_dec_int(nqiv_shared_var* var)
{
	nqiv_shared_var_lock(var);
	var->data.as_int -= 1;
	nqiv_shared_var_unlock(var);
}

int64_t nqiv_shared_var_get_int(nqiv_shared_var* var)
{
	nqiv_shared_var_lock(var);
	const int64_t result = var->data.as_int;
	nqiv_shared_var_unlock(var);
	return result;
}

void nqiv_shared_var_set_int(nqiv_shared_var* var, const int64_t value)
{
	nqiv_shared_var_lock(var);
	var->data.as_int = value;
	nqiv_shared_var_unlock(var);
}
