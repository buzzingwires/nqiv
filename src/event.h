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
	bool file_soft;
	bool wand;
	bool wand_soft;
	bool first_frame;
	bool next_frame;
	bool raw;
	bool raw_soft;
	bool surface;
	bool surface_soft;
} nqiv_event_image_load_form_options;

typedef struct nqiv_event_image_load_options
{
	nqiv_image* image;
	bool set_thumbnail_path;
	bool create_thumbnail;
	nqiv_event_image_load_form_options image_options;
	nqiv_event_image_load_form_options thumbnail_options;
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

#endif /* NQIV_EVENT_H */