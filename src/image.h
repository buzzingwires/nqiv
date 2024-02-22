#ifndef NQIV_IMAGE_H
#define NQIV_IMAGE_H

#include <stdbool.h>

#include <SDL2/SDL.h>
#include <MagickCore/MagickCore.h>
#include <MagickWand/MagickWand.h>

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

typedef struct nqiv_image_form
{
	bool animated;
	double frame_delta;
	bool error;
	char* path;
	FILE* file;
	MagickWand* wand;
	void* data;
	SDL_Surface* surface;
	SDL_Texture* texture;
	int height;
	int width;
} nqiv_image_form;

typedef struct nqiv_image nqiv_image;
typedef struct nqiv_image_manager nqiv_image_manager;

struct nqiv_image
{
	nqiv_image_manager* parent;
	omp_lock_t lock;
	bool thumbnail_attempted;
	nqiv_image_form image;
	nqiv_image_form thumbnail;
};

/*
nqiv_image* nqiv_image_create(nqiv_log_ctx* logger, const char* path);
bool nqiv_image_load_raw_full(nqiv_image* image);
bool nqiv_image_load_sdl_surface_full(nqiv_image* image);
bool nqiv_image_load_sdl_texture_full(nqiv_image* image);
bool nqiv_image_unload_full(nqiv_image* image);
bool nqiv_image_load_raw_thumbnail(nqiv_image* image);
bool nqiv_image_load_sdl_surface_thumbnail(nqiv_image* image);
bool nqiv_image_load_sdl_texture_thumbnail(nqiv_image* image);
bool nqiv_image_unload_thumbnail(nqiv_image* image);
bool nqiv_image_next_frame(nqiv_image* image);
void nqiv_image_destroy(nqiv_image* image);
*/


bool nqiv_image_form_first_frame(nqiv_image_form* form);
bool nqiv_image_form_next_frame(nqiv_image_form* form);

void nqiv_unload_image_form_wand(nqiv_image_form* form);
void nqiv_unload_image_form_file(nqiv_image_form* form);
void nqiv_unload_image_form_texture(nqiv_image_form* form);
void nqiv_unload_image_form_surface(nqiv_image_form* form);
void nqiv_unload_image_form_raw(nqiv_image_form* form);

bool nqiv_image_ping_wand(nqiv_image* image, nqiv_image_form* form);
bool nqiv_image_load_wand(nqiv_image* image, nqiv_image_form* form);
bool nqiv_image_upgrade_wand(nqiv_image* image, nqiv_image_form* form);
bool nqiv_image_load_texture(nqiv_image* image, nqiv_image_form* form);
bool nqiv_image_load_surface(nqiv_image* image, nqiv_image_form* form);
bool nqiv_image_load_raw(nqiv_image* image, nqiv_image_form* form);

bool nqiv_image_borrow_thumbnail_dimensions(nqiv_image* image);
void nqiv_image_rect_to_aspect_ratio(const nqiv_image* image, SDL_Rect* rect, const bool readd_zoom);

typedef struct nqiv_image_manager_thumbnail_settings
{
	char* root;
	bool load;
	bool save;
	int height;
	int width;
	FilterType interpolation;
} nqiv_image_manager_thumbnail_settings;

typedef struct nqiv_image_manager_zoom_settings
{
	double image_to_viewport_ratio;
	double viewport_horizontal_shift;
	double viewport_vertical_shift;
	double pan_left_amount;
	double pan_right_amount;
	double pan_up_amount;
	double pan_down_amount;
	double zoom_in_amount;
	double zoom_out_amount;
	int thumbnail_adjust;
} nqiv_image_manager_zoom_settings;

struct nqiv_image_manager
{
	nqiv_log_ctx* logger;
	nqiv_image_manager_thumbnail_settings thumbnail;
	nqiv_image_manager_zoom_settings zoom;
	nqiv_array* images;
	nqiv_array* extensions;
};

void nqiv_log_magick_wand_exception(nqiv_log_ctx* logger, const MagickWand* magick_wand, const char* path);

void nqiv_image_manager_destroy(nqiv_image_manager* manager);
bool nqiv_image_manager_init(nqiv_image_manager* manager, nqiv_log_ctx* logger, const int starting_length);
bool nqiv_image_manager_insert(nqiv_image_manager* manager, const char* path, const int index);
void nqiv_image_manager_remove(nqiv_image_manager* manager, const int index);
bool nqiv_image_manager_append(nqiv_image_manager* manager, const char* path);
bool nqiv_image_manager_add_extension(nqiv_image_manager* manager, char* extension);
bool nqiv_image_manager_has_path_extension(nqiv_image_manager* manager, const char* path);

void nqiv_image_manager_pan_left(nqiv_image_manager* manager);
void nqiv_image_manager_pan_right(nqiv_image_manager* manager);
void nqiv_image_manager_pan_up(nqiv_image_manager* manager);
void nqiv_image_manager_pan_down(nqiv_image_manager* manager);
void nqiv_image_manager_zoom_in(nqiv_image_manager* manager);
void nqiv_image_manager_zoom_out(nqiv_image_manager* manager);
void nqiv_image_manager_calculate_zoomrect(nqiv_image_manager* manager, const nqiv_image_form* form, SDL_Rect* rect);

void nqiv_image_manager_increment_thumbnail_size(nqiv_image_manager* manager);
void nqiv_image_manager_decrement_thumbnail_size(nqiv_image_manager* manager);
/* REMOVE? */
/* Dynamic array? */
/* convert to dynamic */
/* remove OMP preprocessor */
/* Better pointer arithmetic */
/* int types */
/* better checks */
/* assertions */
/* better naming */
/* int array ttransactons */
/* event representation */
/* pointer types */
/* clear all with memset */
/* free and clear */
/* some other generic */
/* allocator */
/* log for array */
/* rename from emlib */
/* headers */

#endif /* NQIV_IMAGE_H */
