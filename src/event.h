#ifndef NQIV_EVENT_H
#define NQIV_EVENT_H

#include <stdbool.h>
#include <stdint.h>

#include "image.h"

typedef enum nqiv_event_type
{
	NQIV_EVENT_WORKER_STOP,
	NQIV_EVENT_IMAGE_LOAD,
} nqiv_event_type;

typedef struct nqiv_event_image_load_form_options
{
	bool clear_error;
	bool unload;
	bool vips;
	/* If unload is set, soft operations will only work if the texture of the given form exists. If
	 * it is not set, it will avoid reloading a form data that is already loaded. */
	bool vips_soft;
	bool first_frame;
	bool next_frame;
	bool surface;
	bool surface_soft;
} nqiv_event_image_load_form_options;

typedef struct nqiv_event_image_load_options
{
	nqiv_image*                        image;
	/* Calculate the thumbnail path, but don't create it. */
	bool                               set_thumbnail_path;
	bool                               create_thumbnail;
	/* If it exists, prematurely copy the known dimensions of the image from the thumbnail. There
	 * are other operations that benefit from knowing this, such as showing the dimensions of the
	 * actual image, when only a thumbnail is loaded. */
	bool                               borrow_thumbnail_dimension_metadata;
	nqiv_event_image_load_form_options image_options;
	nqiv_event_image_load_form_options thumbnail_options;
} nqiv_event_image_load_options;

typedef union nqiv_event_options
{
	nqiv_event_image_load_options image_load;
} nqiv_event_options;

typedef struct nqiv_event
{
	nqiv_event_type    type;
	/* The transaction group is compared against a current number tracked by the threads. If the
	 * event is less than the current number, it is considered out of date and discarded. An event
	 * with a transaction group of -1 is never out of date. This feature primarily exists to solve
	 * the problem of events still being queued for images that are no longer visible. */
	int64_t            transaction_group;
	nqiv_event_options options;
} nqiv_event;

#endif /* NQIV_EVENT_H */
