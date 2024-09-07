#ifndef NQIV_IMAGE_H
#define NQIV_IMAGE_H

#include <stdbool.h>

#include <SDL2/SDL.h>
#include <vips/vips.h>

#include "array.h"
#include "logging.h"

/*
 * thumbnail data
 * image data
 * mode sdl texture | sdl surface | raw data
 * load thumbnail (from disk)
 * save thumbnail (to disk)
 * thumbnail path
 * image path
 * size
 * other metadata
 * instead of size readahead and behind like vimiv?
 */

typedef struct nqiv_image_form_animation
{
	bool   exists;
	bool   frame_rendered;
	int    frame;
	int    frame_count;
	Uint64 last_frame_time;
	Uint32 delay;
} nqiv_image_form_animation;

typedef struct nqiv_image_form
{
	nqiv_image_form_animation animation;
	bool                      error;
	char*                     path;
	VipsImage*                vips;
	void*                     data;
	SDL_Surface*              surface;
	SDL_Texture*              texture;
	SDL_Texture*              fallback_texture;
	int                       height;
	int                       width;
	SDL_Rect                  srcrect;
	int                       effective_height;
	int                       effective_width;
	SDL_Rect                  master_srcrect;
	SDL_Rect                  master_dstrect;
	bool                      master_dimensions_set;
	bool                      thumbnail_load_failed;
} nqiv_image_form;

typedef struct nqiv_image         nqiv_image;
typedef struct nqiv_image_manager nqiv_image_manager;

struct nqiv_image
{
	nqiv_image_manager* parent;
	omp_lock_t          lock;
	bool                thumbnail_attempted;
	bool                marked;
	nqiv_image_form     image;
	nqiv_image_form     thumbnail;
};

bool nqiv_image_form_first_frame(nqiv_image* image, nqiv_image_form* form);
bool nqiv_image_form_next_frame(nqiv_image* image, nqiv_image_form* form);

void nqiv_unload_image_form_vips(nqiv_image_form* form);
void nqiv_unload_image_form_texture(nqiv_image_form* form);
void nqiv_unload_image_form_surface(nqiv_image_form* form);
void nqiv_unload_image_form_raw(nqiv_image_form* form);
void nqiv_unload_image_form_fallback_texture(nqiv_image_form* form);

bool nqiv_image_load_vips(nqiv_image* image, nqiv_image_form* form);
bool nqiv_image_load_texture(nqiv_image* image, nqiv_image_form* form);
bool nqiv_image_load_surface(nqiv_image* image, nqiv_image_form* form);
bool nqiv_image_load_raw(nqiv_image* image, nqiv_image_form* form);

int  nqiv_lookup_vips_png_comment(gchar** values, const char* key);
bool nqiv_image_borrow_thumbnail_dimensions(nqiv_image* image);

bool nqiv_image_has_loaded_form(nqiv_image* image);

typedef struct nqiv_image_manager_thumbnail_settings
{
	char* root;
	bool  load;
	bool  save;
	int   size;
} nqiv_image_manager_thumbnail_settings;

typedef struct nqiv_image_manager_zoom_settings
{
	double image_to_viewport_ratio;
	double image_to_viewport_ratio_max;
	double fit_level;
	double actual_size_level;
	double viewport_horizontal_shift;
	double viewport_vertical_shift;
	double pan_left_amount;
	double pan_right_amount;
	double pan_up_amount;
	double pan_down_amount;
	double pan_coordinate_x_multiplier;
	double pan_coordinate_y_multiplier;
	double zoom_in_amount;
	double zoom_out_amount;
	int    thumbnail_adjust;
	double pan_left_amount_more;
	double pan_right_amount_more;
	double pan_up_amount_more;
	double pan_down_amount_more;
	double zoom_in_amount_more;
	double zoom_out_amount_more;
	int    thumbnail_adjust_more;
} nqiv_image_manager_zoom_settings;

struct nqiv_image_manager
{
	nqiv_log_ctx*                         logger;
	nqiv_image_manager_thumbnail_settings thumbnail;
	nqiv_image_manager_zoom_settings      zoom;
	Uint32                                default_frame_time;
	nqiv_array*                           images;
};

void nqiv_log_vips_exception(nqiv_log_ctx* logger, nqiv_image* image, nqiv_image_form* form);

void nqiv_image_unlock(nqiv_image* image);
void nqiv_image_lock(nqiv_image* image);
bool nqiv_image_test_lock(nqiv_image* image);

void nqiv_image_manager_destroy(nqiv_image_manager* manager);
bool nqiv_image_manager_init(nqiv_image_manager* manager,
                             nqiv_log_ctx*       logger,
                             const int           starting_length);
bool nqiv_image_manager_insert(nqiv_image_manager* manager, const char* path, const int index);
bool nqiv_image_manager_remove(nqiv_image_manager* manager, const int index);
bool nqiv_image_manager_append(nqiv_image_manager* manager, const char* path);
bool nqiv_image_manager_set_thumbnail_root(nqiv_image_manager* manager, const char* path);

void nqiv_image_manager_pan_left(nqiv_image_manager* manager);
void nqiv_image_manager_pan_right(nqiv_image_manager* manager);
void nqiv_image_manager_pan_up(nqiv_image_manager* manager);
void nqiv_image_manager_pan_down(nqiv_image_manager* manager);
void nqiv_image_manager_pan_coordinates(nqiv_image_manager* manager, const SDL_Rect* coordinates);
void nqiv_image_manager_zoom_in(nqiv_image_manager* manager);
void nqiv_image_manager_zoom_out(nqiv_image_manager* manager);
void nqiv_image_manager_pan_center(nqiv_image_manager* manager);

void nqiv_image_manager_pan_left_more(nqiv_image_manager* manager);
void nqiv_image_manager_pan_right_more(nqiv_image_manager* manager);
void nqiv_image_manager_pan_up_more(nqiv_image_manager* manager);
void nqiv_image_manager_pan_down_more(nqiv_image_manager* manager);
void nqiv_image_manager_zoom_in_more(nqiv_image_manager* manager);
void nqiv_image_manager_zoom_out_more(nqiv_image_manager* manager);

void nqiv_image_manager_retrieve_zoomrect(nqiv_image_manager* manager,
                                          const bool          do_zoom,
                                          const bool          do_stretch,
                                          SDL_Rect*           srcrect,
                                          SDL_Rect*           dstrect);
void nqiv_image_manager_calculate_zoom_parameters(nqiv_image_manager* manager,
                                                  const bool          tight_fit,
                                                  const SDL_Rect*     srcrect,
                                                  const SDL_Rect*     dstrect);
int  nqiv_image_manager_get_zoom_percent(nqiv_image_manager* manager);

void nqiv_image_manager_reattempt_thumbnails(nqiv_image_manager* manager, const int old_size);
void nqiv_image_manager_increment_thumbnail_size(nqiv_image_manager* manager);
void nqiv_image_manager_decrement_thumbnail_size(nqiv_image_manager* manager);
void nqiv_image_manager_increment_thumbnail_size_more(nqiv_image_manager* manager);
void nqiv_image_manager_decrement_thumbnail_size_more(nqiv_image_manager* manager);

#endif /* NQIV_IMAGE_H */
