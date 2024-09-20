#ifndef NQIV_IMAGE_H
#define NQIV_IMAGE_H

#include <stdbool.h>

#include <SDL2/SDL.h>
#include <vips/vips.h>

#include "array.h"
#include "queue.h"
#include "logging.h"

/*
 * Image data is stored and managed within an nqiv_image_manager object, which
 * also contains settings pertaining to zooming, animation, as well as thumbnail
 * behavior.
 *
 * An nqiv_image object is the representation of an image and its thumbnail,
 * which are respectively respectively represented by twin 'nqiv_image_form'
 * objects. A form is loaded in stages. First, the VIPS library representation
 * is loaded, then the raw image pixel data is extracted, then this pixel data
 * is used to back an SDL surface, and finally, the surface is loaded into a
 * texture, which is what will actually be displayed. Each stage may be
 * performed in a worker thread, except for texture loading and unloading, which
 * must be performed from the master, due to the constraints of OpenGL, which
 * backs SDL2's textures a the time of writing.
 */

typedef struct nqiv_image_form_animation
{
	bool   exists;          /* Is animation present for this form? */
	bool   frame_rendered;  /* Has the frame been rendered yet? */
	int    frame;           /* Which frame are we currently on? */
	int    frame_count;     /* How many frames are there? */
	Uint64 last_frame_time; /* SDL tick (millisecond) timestamp of when last frame was rendered. */
	Uint32 delay;           /* Delay for the current frame- this can vary. */
} nqiv_image_form_animation;

typedef struct nqiv_image_form
{
	nqiv_image_form_animation animation;
	/* Was there some kind of error with this form? */
	bool                      error;
	char*                     path;
	VipsImage*                vips;
	void*                     data;
	SDL_Surface*              surface;
	SDL_Texture*              texture;
	/* The fallback texture is used if the current texture is not available, since waiting for the
	 * texture to unload and load again may cause flickering. */
	SDL_Texture*              fallback_texture;
	/* Dimensions of whole image. */
	int                       height;
	int                       width;
	/* Sometimes we want to crop out a large image. */
	SDL_Rect                  srcrect;
	/* Dimensions of whatever crop we're using. */
	int                       effective_height;
	int                       effective_width;
	/* Master entries are record keeping reserved to the master thread only for
	 * rendering the fallback texture. They are NOT protected by the lock. */
	SDL_Rect                  master_srcrect;
	SDL_Rect                  master_dstrect;
	bool                      master_dimensions_set;
	/* Have we tried and failed to load a thumbnail? If the thumbnail is
	 * successfully created later on, this may be reset. */
	bool                      thumbnail_load_failed;
} nqiv_image_form;

typedef struct nqiv_image         nqiv_image;
typedef struct nqiv_image_manager nqiv_image_manager;

struct nqiv_image
{
	nqiv_image_manager* parent;
	omp_lock_t          lock;
	/* Have we tried to create a thumbnail, successfully or otherwise? Don't
	 * retry. */
	bool                thumbnail_attempted;
	/* Used to visually mark images and select them for certain operations. */
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
void nqiv_unload_image_form_all_textures(nqiv_image_form* form);

bool nqiv_image_load_vips(nqiv_image* image, nqiv_image_form* form);
bool nqiv_image_load_texture(nqiv_image* image, nqiv_image_form* form);
bool nqiv_image_load_surface(nqiv_image* image, nqiv_image_form* form);
bool nqiv_image_load_raw(nqiv_image* image, nqiv_image_form* form);

/* Get thumbnail metadata. */
int nqiv_lookup_vips_png_comment(gchar** values, const char* key);

/* The image thumbnail contains the actual dimensions of the image in its
 * metadata. Copy them to the image form. There are other operations that
 * benefit from knowing this, such as showing the dimensions of the actual
 * image, when only a thumbnail is loaded. */
bool nqiv_image_borrow_thumbnail_dimensions(nqiv_image* image);

bool nqiv_image_has_loaded_form(nqiv_image* image);

typedef struct nqiv_image_manager_thumbnail_settings
{
	/* Where thumbnails are stored. */
	char* root;
	/* Do we load and/or save? */
	bool  load;
	bool  save;
	/* Dimension of either side. */
	int   size;
} nqiv_image_manager_thumbnail_settings;

typedef struct nqiv_image_manager_zoom_settings
{
	/* Current zoom level. Lower values are zoomed in more. */
	double image_to_viewport_ratio;
	/* This is typically how far out one has to zoom to view the whole image,
	 * fit level for larger images, actual size for ones smaller than the
	 * screen. */
	double image_to_viewport_ratio_max;
	/* Zoom level to view whole image. */
	double fit_level;
	double actual_size_level;
	double viewport_horizontal_shift;
	double viewport_vertical_shift;
	double pan_left_amount;
	double pan_right_amount;
	double pan_up_amount;
	double pan_down_amount;
	/* Multipliers primarily used for mouse panning at this point. Larger
	 * absolute values equate to faster motion. Positive means image will move
	 * in the same direction of mouse. Negative is inverted motion. */
	double pan_coordinate_x_multiplier;
	double pan_coordinate_y_multiplier;
	double zoom_in_amount;
	double zoom_out_amount;
	/* 'Zoom' thumbnails in and out by adding or subtracting this amount of
	 * pixels from their dimension. */
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
	/* Platform depdendent and set at runtime- these control when an image must
	 * be cropped or resized for conversion into a texture. */
	int                                   max_texture_height;
	int                                   max_texture_width;
	nqiv_array*                           images;
	nqiv_priority_queue*                  thread_queue;
};

void nqiv_log_vips_exception(nqiv_log_ctx*          logger,
                             const nqiv_image*      image,
                             const nqiv_image_form* form);

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

/* Actually retrieve zoom rect. */
void nqiv_image_manager_retrieve_zoomrect(nqiv_image_manager* manager,
                                          const bool          do_zoom,
                                          const bool          do_stretch,
                                          SDL_Rect*           srcrect,
                                          SDL_Rect*           dstrect);
/* Calculate rations, actual size, fit amounts, etc' based on display and image
 * dimensions. */
void nqiv_image_manager_calculate_zoom_parameters(nqiv_image_manager* manager,
                                                  const bool          tight_fit,
                                                  const SDL_Rect*     srcrect,
                                                  const SDL_Rect*     dstrect);
int  nqiv_image_manager_get_zoom_percent(nqiv_image_manager* manager);

/* Thumbnail files are created based on the size displayed on screen, their
 * sizes determined by the Free Desktop Thumbnail Managing standard 0.9.0, which
 * can be found at:
 * https://specifications.freedesktop.org/thumbnail-spec/latest/index.html When
 * the thumbnail size changes, it is necessary to determine if a different size
 * file should be rendered by using this function. */
bool nqiv_image_manager_reattempt_thumbnails(nqiv_image_manager* manager, const int old_size);
/* 'Zoom' for thumbnail montage. */
void nqiv_image_manager_increment_thumbnail_size(nqiv_image_manager* manager);
void nqiv_image_manager_decrement_thumbnail_size(nqiv_image_manager* manager);
void nqiv_image_manager_increment_thumbnail_size_more(nqiv_image_manager* manager);
void nqiv_image_manager_decrement_thumbnail_size_more(nqiv_image_manager* manager);

#endif /* NQIV_IMAGE_H */
