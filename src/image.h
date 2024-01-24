#ifndef NQIV_IMAGE_H
#define NQIV_IMAGE_H

#include <stdbool.h>

#include <SDL.h>
#include <wand/MagickWand.h>

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
	bool error;
	char* path;
	FILE* file;
	magick_wand* wand;
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

typedef struct nqiv_image_manager_thumbnail_settings
{
	char* root;
	bool load;
	bool save;
	int height;
	int width;
	PixelInterpolateMethod interpolation;
} nqiv_image_manager_thumbnail_settings;

typedef struct nqiv_image_manager_zoom_settings
{
	double image_to_viewport_ratio;
	double viewport_horizontal_shift;
	double viewport_vertical_shift;
} nqiv_image_manager_zoom_settings;

struct nqiv_image_manager
{
	nqiv_log_ctx* logger;
	nqiv_image_manager_thumbnail_settings thumbnail;
	nqiv_image_manager_zoom_settings zoom;
	nqiv_array* images;
	nqiv_array* extensions;
};

/*
void nqiv_image_manager_destroy(nqiv_image_manager* manager);
bool nqiv_image_manager_init(nqiv_image_manager* manager, nqiv_log_ctx* logger, const int starting_length);
bool nqiv_image_manager_insert(nqiv_image_manager* manager, const char* path, const int index);
void nqiv_image_manager_remove(nqiv_image_manager* manager, const int index);
bool nqiv_image_manager_append(nqiv_image_manager* manager, const char* path);
bool nqiv_image_manager_add_extension(nqiv_image_manager* manager, const char* extension);
*/
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
