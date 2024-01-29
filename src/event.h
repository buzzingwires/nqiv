#ifndef NQIV_EVENT_H
#define NQIV_EVENT_H

typedef enum nqiv_event_type
{
	NQIV_EVENT_WORKER_STOP,
	NQIV_EVENT_IMAGE_LOAD,
} nqiv_event_type;

typedef struct nqiv_event_image_load_form_options
{
	bool unload;
	bool file;
	bool wand;
	bool frame;
	bool raw;
	bool surface;
} nqiv_event_image_load_form_options;

typedef struct nqiv_event_image_load_options
{
	nqiv_image* image;
	bool create_thumbnail;
	nqiv_event_image_load_form_options image;
	nqiv_event_image_load_form_options thumbnail;
} nqiv_event_image_load_options;

typedef union nqiv_event_options
{
	nqiv_event_image_load_options image_load;
} nqiv_event_data

typedef struct nqiv_event
{
	nqiv_event_type type;
	nqiv_event_options options;
} nqiv_event;

#define NQIV_SDL_EVENT_WORKER_FINISHED 1

#endif /* NQIV_EVENT_H */
