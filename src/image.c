#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <wand/magick_wand.h>
#include <omp.h>

#include "array.h"
#include "image.h"

/* Image */
void nqiv_unload_image_form_wand(nqiv_image_form* form)
{
	assert(form != NULL);
	if(form->wand != NULL) {
		DestroyMagickWand(form->wand); /* TODO: Where should this be? */
		form->wand = NULL;
	}
}

void nqiv_unload_image_form_file(nqiv_image_form* form)
{
	assert(form != NULL);
	if(form->file != NULL) {
		fclose(form->file);
		form->file = NULL;
	}
}

void nqiv_unload_image_form_texture(nqiv_image_form* form)
{
	assert(form != NULL);
	if(form->texture != NULL) {
		SDL_DestroyTexture(form->texture);
		form->texture = NULL;
	}
}

void nqiv_unload_image_form_surface(nqiv_image_form* form)
{
	assert(form != NULL);
	if(form->surface != NULL) {
		SDL_FreeSurface(form->surface);
		form->surface = NULL;
	}
}

void nqiv_unload_image_form_raw(nqiv_image_form* form)
{
	assert(form != NULL);
	if(form->data != NULL) {
		free(form->data);
		form->data = NULL;
	}
}

void nqiv_unload_image_form(nqiv_image_form* form)
{
	assert(form != NULL);
	nqiv_unload_image_form_wand(form);
	nqiv_unload_image_form_file(form);
	nqiv_unload_image_form_texture(form);
	nqiv_unload_image_form_surface(form);
	nqiv_unload_image_form_raw(form);
}

void nqiv_image_destroy(nqiv_image* image) {
	assert(image != NULL);
	assert(image->parent != NULL);
	nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Destroying image %s.\n", image->image.path);
	omp_destroy_lock(&image->lock);
	nqiv_unload_image_form(&image->image);
	nqiv_unload_image_form(&image->thumbnail);
	if(image->image.path != NULL) {
		free(image->image.path);
	}
	if(image->thumbnail.path != NULL) {
		free(image->thumbnail.path);
	}
	memset( image, 0, sizeof(nqiv_image) );
	free(image);
}

nqiv_image* nqiv_image_create(nqiv_log_ctx* logger, const char* path)
{
	assert(logger != NULL);
	const size_t path_len = strlen(path);
	if(path_len == 0) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Cannot create image with zero-length path.");
		return NULL;
	}
	const size_t path_size = path_len + 1;
	nqiv_image* image = (nqiv_image*)calloc( 1, sizeof(nqiv_image) );
	if(image == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to allocate memory for image at path %s.", path);
		return image;
	}
	image->image.path = calloc(1, path_size);
	if(image->image.path == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to allocate memory for path data %s.", path);
		nqiv_image_destroy(image);
		return NULL;
	}
	omp_init_lock(&image->lock);
	strncpy(image->image.path, path, path_len);
	assert(strcmp(image->image.path, path) == 0);
	nqiv_log_write(logger, NQIV_LOG_DEBUG, "Created image %s.\n", image->image.path);
	return image;
}

void nqiv_log_magick_wand_exception(nqiv_log_ctx* logger, const MagickWand* magick_wand, const char* path)
{
	assert(logger != NULL);
	assert(magick_wand != NULL);
	assert(path != NULL);
	ExceptionType severity;
	char* description = MagickGetException(magick_wand, &severity);
	nqiv_log_write(logger, NQIV_LOG_ERROR, "ImageMagick exception for path %s: %s\n", path, description);
	MagickRelinquishMemory(description);
}

/* TODO to thumbnail */
/* TODO step frame */
/* TODO input cleanup */
/* TODO unload */
/* TODO Add twice */
/* TODO Detect change */

bool nqiv_image_load_wand(nqiv_image* image, nqiv_image_form* form)
{
	assert(image != NULL);
	assert(form != NULL);
	assert( (form->file == NULL && form->wand == NULL) );
	if(form->path == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "No path for form in image %s.\n", image->image.path);
		/*nqiv_set_invalid_image_form(form);*/
		form->error = true;
		return false;
	}
	form->file = fopen(form->path, "r");
	if(form->file == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to open image file at path %s.\n", form->path);
		/*nqiv_set_invalid_image_form(form);*/
		form->error = true;
		return false;
	}
	rewind(form->file);
	form->wand = NewMagickWand();
	MagickPassFail read_result;
	read_result = MagickReadImageFile(form->wand, form->file);

	if(read_result == MagickFail) {
		nqiv_log_magick_wand_exception(image->parent->logger, form->wand, form->path);
		nqiv_unload_image_form(form);
		form->error = true;
		return false;
	}
	MagickResetIterator(form->wand);
	if( MagickHasNextImage(form->wand) ) {
		form->animation.exists = true;
		form->animation.delay = (Uint32)(MagickGetImageDelay(form->wand) * 10); /* Delay is in centiseconds */
		MagickWand* final_wand = MagickCoalesceImages(form->wand);
		if(final_wand == NULL) {
			nqiv_unload_image_form(form);
			nqiv_log_magick_wand_exception(image->parent->logger, form->wand, form->path);
			form->error = true;
			return false;
		} else {
			nqiv_unload_image_form_wand(form);
		}
		form->wand = final_wand;
	}
	form->height = MagickGetImageHeight(form->wand);
	form->width = MagickGetImageWidth(form->wand);
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "%s wand for image loaded.\n", image->image.path);
	/* GIFs are 10 FPS by default. Do we need to account for other delays? */
	return true;
}

bool nqiv_image_load_raw(nqiv_image* image, nqiv_image_form* form)
{
	assert(image != NULL);
	assert(form != NULL);
	assert( (form->file != NULL && form->wand != NULL && form->data == NULL) );
	form->data = calloc( 1, strlen("RGBA") * form->height * form->width );
	if(form->data == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to allocate memory for raw image data at path %s.", form->path);
		nqiv_unload_image_form(form);
		form->error = true;
		return false;
	}
	if(MagickGetImagePixels(form->wand, 0, 0, form->width, form->height, "RGBA", CharPixel, form->data) == MagickFail) {
		nqiv_log_magick_wand_exception(image->parent->logger, form->wand, form->path);
		nqiv_unload_image_form(form);
		form->error = true;
		return false;
	}
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Loaded raw for image %s.\n", image->image.path);
	return true;
}

bool nqiv_image_load_surface(nqiv_image* image, nqiv_image_form* form)
{
	assert(image != NULL);
	assert(form != NULL);
	assert( (form->data != NULL) );
	assert(form->width > 0);
	assert(form->height > 0);
	form->surface = SDL_CreateRGBSurfaceWithFormatFrom(form->data, form->width, form->height, 4 * 8, 4 * form->width, SDL_PIXELFORMAT_ABGR8888);
	if(form->surface == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to create SDL surface for path %s (%s).", form->path, SDL_GetError() );
		nqiv_unload_image_form(form);
		form->error = true;
		return false;
	}
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Loaded surface for image %s.\n", image->image.path);
	return true;
}

bool nqiv_image_load_sdl_texture(nqiv_image* image, nqiv_image_form* form, SDL_Renderer * renderer)
{
	assert(image != NULL);
	assert(form != NULL);
	assert( (form->data != NULL) );
	assert( (form->surface != NULL) );
	form->texture = SDL_CreateTextureFromSurface(renderer, form->surface);
	if(form->texture == NULL) {
		nqiv_log_write( image->parent->logger, NQIV_LOG_ERROR, "Failed to create SDL texture for path %s (%s).", form->path, SDL_GetError() );
		nqiv_unload_image_form(form);
		form->error = true;
		return false;
	}
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Loaded texture for image %s.\n", image->image.path);
	return true;
}

bool nqiv_image_borrow_thumbnail_dimensions(nqiv_image* image)
{
	assert(image != NULL);
	if(image->thumbnail.wand == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Not borrowing dimension metadata from thumbnail because the wand is unavailable for image %s.\n", image->image.path);
		return true;
	}
	if(image->image.width != 0 || image->image.height != 0) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Not borrowing dimension metadata from thumbnail because it's already set for image %s.\n", image->image.path);
		return true;
	}
	char* width_string = MagickGetImageAttribute(image->thumbnail.wand, "Thumb::Image::Width");
	if(width_string == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING, "Failed to get width metadata from thumbnail for %s.\n", image->image.path);
		return false;
	}
	char* height_string = MagickGetImageAttribute(image->thumbnail.wand, "Thumb::Image::Height");
	if(height_string == NULL) {
		MagickRelinquishMemory(width_string);
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING, "Failed to get height metadata from thumbnail for %s.\n", image->image.path);
		return false;
	}
	const int width_value = strtol(width_string, NULL, 10);
	if(width_value == 0 || errno == ERANGE) {
		MagickRelinquishMemory(width_string);
		MagickRelinquishMemory(height_string);
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING, "Invalid width for thumbnail of '%s'.\n", image->image.path);
		return false;
	}
	const int height_value = strtol(height_string, NULL, 10);
	if(height_value == 0 || errno == ERANGE) {
		MagickRelinquishMemory(width_string);
		MagickRelinquishMemory(height_string);
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING, "Invalid height for thumbnail of '%s'.\n", image->image.path);
		return false;
	}
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Borrowing dimension metadata from thumbnail set for image %s.\n", image->image.path);
	image->image.width = width_value;
	image->image.height = height_value;
	MagickRelinquishMemory(width_string);
	MagickRelinquishMemory(height_string);
	return true;
}

/* Image manager */
void nqiv_image_manager_destroy(nqiv_image_manager* manager)
{
	if(manager == NULL) {
		return;
	}
	nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Destroying image manager.\n");

	nqiv_image* current_image;
	while(true) {
		current_image = nqiv_array_pop_ptr(manager->images);
		if(current_image == NULL) {
			break;
		}
		nqiv_image_destroy(current_image);
	}

	if(manager->images != NULL) {
		nqiv_array_destroy(manager->images);
	}
	if(manager->extensions != NULL) {
		nqiv_array_destroy(manager->extensions);
	}
	memset( manager, 0, sizeof(nqiv_image_manager) );
}

bool nqiv_image_manager_init(nqiv_image_manager* manager, nqiv_log_ctx* logger, const int starting_length)
{
	if(logger == NULL) {
		return false;
	}
	if(starting_length <= 0) {

		nqiv_log_write(logger, NQIV_LOG_ERROR, "Cannot make image manager with starting length of: %d", starting_length);
		return false;
	}
	nqiv_array* images = nqiv_array_create(starting_length);
	if(images == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Cannot make image manager images array with starting length of: %d", starting_length);
		return false;
	}
	nqiv_array* extensions = nqiv_array_create(starting_length);
	assert(images != extensions);
	if(extensions == NULL) {
		nqiv_array_destroy(images);
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Cannot make image manager extensions array with starting length of: %d", starting_length);
		return false;
	}
	nqiv_image_manager_destroy(manager);
	manager->logger = logger;
	manager->images = images;
	manager->extensions = extensions;
	manager->zoom.image_to_viewport_ratio_max = 1.0;
	manager->zoom.image_to_viewport_ratio = 1.0;
	manager->zoom.fit_level = 1.0;
	manager->zoom.actual_size_level = 1.0;

	manager->zoom.pan_left_amount = -0.05;
	manager->zoom.pan_right_amount = 0.05;
	manager->zoom.pan_up_amount = -0.05;
	manager->zoom.pan_down_amount = 0.05;
	manager->zoom.zoom_in_amount = -0.05;
	manager->zoom.zoom_out_amount = 0.05;
	manager->zoom.thumbnail_adjust = 10;
	nqiv_log_write(logger, NQIV_LOG_INFO, "Successfully made image manager with starting length of: %d", starting_length);
	return true;
}

/* HELPERS */
bool nqiv_array_push_nqiv_image_ptr(nqiv_array* array, nqiv_image* image)
{
	return nqiv_array_push_ptr( array, (void*)image );
}

bool nqiv_array_insert_nqiv_image_ptr(nqiv_array* array, nqiv_image* image, const int idx)
{
	return nqiv_array_insert_ptr(array, (void*)image, idx);
}

void nqiv_array_remove_nqiv_image_ptr(nqiv_array* array, const int idx)
{
	nqiv_array_remove_ptr(array, idx);
}

nqiv_image* nqiv_array_get_nqiv_image_ptr(nqiv_array* array, const int idx)
{
	return (nqiv_image*)nqiv_array_get_ptr(array, idx);
}
/* HELPERS END */

bool nqiv_image_manager_has_path_extension(nqiv_image_manager* manager, const char* path)
{
	assert(manager != NULL);
	assert(manager->extensions != NULL);

	const size_t path_len = strlen(path);
	const int num_extensions = manager->extensions->position / sizeof(char*);
	int idx;
	for(idx = 0; idx < num_extensions; ++idx) {
		char* ext;
		if( nqiv_array_get_bytes(manager->extensions, idx, sizeof(char*), &ext) ) {
			const size_t extlen = strlen(ext);
			if(extlen > path_len) {
				continue;
			}
			if(strcmp(path + path_len - extlen, ext) == 0) {
				return true;
			}
		}
	}
	return false;
}

bool nqiv_image_manager_insert(nqiv_image_manager* manager, const char* path, const int index)
{
	nqiv_image* image = nqiv_image_create(manager->logger, path);
	if(image == NULL) {
		// nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Failed to generate image at path '%s'. Success: %s", path, "false");
		return false;
	}
	if(!nqiv_array_insert_nqiv_image_ptr(manager->images, image, index)) {
		nqiv_image_destroy(image);
		nqiv_log_write(manager->logger, NQIV_LOG_ERROR, "Failed to add image at path '%s' to image manager at index %d.\n", path, index);
		return false;
	}
	// nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Generated image at path '%s'.", path);
	nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Added image at path '%s' to image manager at index %d.\n", path, index);
	image->parent = manager;
	return true;
}

void nqiv_image_manager_remove(nqiv_image_manager* manager, const int index)
{
	nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Adding image at index %d from image manager.\n", index);
	nqiv_image_destroy( nqiv_array_get_nqiv_image_ptr(manager->images, index) );
	nqiv_array_remove_nqiv_image_ptr(manager->images, index);
}

bool nqiv_image_manager_append(nqiv_image_manager* manager, const char* path)
{
	nqiv_image* image = nqiv_image_create(manager->logger, path);
	if(image == NULL) {
		// nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Failed to generate image at path '%s'. Success: %s", path, "false");
		return false;
	}
	if(!nqiv_array_push_nqiv_image_ptr(manager->images, image)) {
		nqiv_image_destroy(image);
		nqiv_log_write(manager->logger, NQIV_LOG_ERROR, "Failed to add image at path '%s' to image manager.", path);
		return false;
	}
	// nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Generated image at path '%s'.", path);
	nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Added image at path '%s' to image manager.\n", path);
	image->parent = manager;
	return true;
}

bool nqiv_image_manager_add_extension(nqiv_image_manager* manager, char* extension)
{
	const bool outcome = nqiv_array_push_char_ptr(manager->extensions, extension);
	nqiv_log_write(manager->logger, outcome?NQIV_LOG_INFO:NQIV_LOG_ERROR, "Adding extension '%s' to image manager. Success: %s\n", extension, outcome?"true":"false");
	return outcome;
}

void nqiv_image_calculate_zoom_dimension(const double least, const bool inclusive_least, const double catch_point, const double most, const bool inclusive_most, double* target, const double amount)
{
	double new_target = *target + amount;
	if( (*target < catch_point && new_target > catch_point) ||
		(*target > catch_point && new_target < catch_point) ) {
		new_target = catch_point;
	}
	if(inclusive_least) {
		new_target = new_target < least ? least : new_target;
	} else {
		new_target = new_target <= least ? *target : new_target;
	}
	if(inclusive_most) {
		new_target = new_target > most ? most : new_target;
	} else {
		new_target = new_target >= most ? *target : new_target;
	}
	*target = new_target;
}

void nqiv_image_manager_pan_left(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(-1.0, true, 0.0, 1.0, true, &manager->zoom.viewport_horizontal_shift, manager->zoom.pan_left_amount);
}

void nqiv_image_manager_pan_right(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(-1.0, true, 0.0, 1.0, true, &manager->zoom.viewport_horizontal_shift, manager->zoom.pan_right_amount);
}

void nqiv_image_manager_pan_up(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(-1.0, true, 0.0, 1.0, true, &manager->zoom.viewport_vertical_shift, manager->zoom.pan_up_amount);
}

void nqiv_image_manager_pan_down(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(-1.0, true, 0.0, 1.0, true, &manager->zoom.viewport_vertical_shift, manager->zoom.pan_down_amount);
}

void nqiv_image_manager_zoom_in(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(0.0, false, manager->zoom.actual_size_level, manager->zoom.image_to_viewport_ratio_max, true, &manager->zoom.image_to_viewport_ratio, manager->zoom.zoom_in_amount);
}

void nqiv_image_manager_zoom_out(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(0.0, false, manager->zoom.actual_size_level, manager->zoom.image_to_viewport_ratio_max, true, &manager->zoom.image_to_viewport_ratio, manager->zoom.zoom_out_amount);
}

/*
 *
	When zoomed all the way out, we have a rect called the 'canvas rect'.
	We imagine this rect has the same aspect ratio as the screen,
	but is big enough to accomodate the entirety of the image.
	Pick the biggest side of the image and set the corresponding side of the rect to that.
	Use that to calculate the other side of the source rect, based on aspect ratio.
	The position should be zero, since this rect corresponds to the entire screen.

	To zoom in, shrink this rect proportionally to the aspect ratio and center accordingly.

	To calculate the actual source rect of the image,
	find the center point of the image and the center point of the canvas rect.
	Move the image in the canvas rect such that it is aligned.
	Any edges of the image that overflow the canvas rect will be clipped to the size of the canvas rect.
	Take them where they are clipped, and any non-overflowing edges, as is.

	To calculate the actual destination rect, divide the height and width of the canvas rect with the screen size.
	Use these to scale the dimensions of the source rect into the screen size.
 */
void nqiv_image_manager_calculate_zoomrect(nqiv_image_manager* manager, const bool do_zoom, const bool do_stretch, SDL_Rect* srcrect, SDL_Rect* dstrect)
{
	assert(manager != NULL);
	assert(srcrect != NULL);
	assert(dstrect != NULL);
	assert(manager->zoom.image_to_viewport_ratio > 0.0);
	assert(manager->zoom.image_to_viewport_ratio <= manager->zoom.image_to_viewport_ratio_max);
	assert(manager->zoom.viewport_horizontal_shift >= -1.0);
	assert(manager->zoom.viewport_horizontal_shift <= 1.0);
	assert(manager->zoom.viewport_vertical_shift >= -1.0);
	assert(manager->zoom.viewport_vertical_shift <= 1.0);
	assert(srcrect->w > 0);
	assert(srcrect->h > 0);
	assert(srcrect->x == 0);
	assert(srcrect->y == 0);
	assert(dstrect->w > 0);
	assert(dstrect->h > 0);
	assert(dstrect->x >= 0);
	assert(dstrect->y >= 0);

	SDL_Rect canvas_rect;
	canvas_rect.x = 0;
	canvas_rect.y = 0;
	if(srcrect->w > srcrect->h) {
		const double screen_aspect = (double)dstrect->h / (double)dstrect->w;
		canvas_rect.w = srcrect->w;
		canvas_rect.h = (int)( (double)srcrect->w * screen_aspect );
	} else {
		const double screen_aspect = (double)dstrect->w / (double)dstrect->h;
		canvas_rect.w = (int)( (double)srcrect->h * screen_aspect );
		canvas_rect.h = srcrect->h;
	}
	nqiv_log_write(manager->logger, NQIV_LOG_DEBUG, "Canvas - CanvasRect: %dx%d+%dx%d SrcRect: %dx%d+%dx%d DstRect: %dx%d+%dx%d\n", canvas_rect.w, canvas_rect.h, canvas_rect.x, canvas_rect.y, srcrect->w, srcrect->h, srcrect->x, srcrect->y, dstrect->w, dstrect->h, dstrect->x, dstrect->y);

	if(do_zoom) {
		canvas_rect.w = (int)( (double)canvas_rect.w * manager->zoom.image_to_viewport_ratio );
		canvas_rect.h = (int)( (double)canvas_rect.h * manager->zoom.image_to_viewport_ratio );
		nqiv_log_write(manager->logger, NQIV_LOG_DEBUG, "Zoom - Zoom Ratio: %f\n", manager->zoom.image_to_viewport_ratio);
		nqiv_log_write(manager->logger, NQIV_LOG_DEBUG, "Zoom - CanvasRect: %dx%d+%dx%d SrcRect: %dx%d+%dx%d DstRect: %dx%d+%dx%d\n", canvas_rect.w, canvas_rect.h, canvas_rect.x, canvas_rect.y, srcrect->w, srcrect->h, srcrect->x, srcrect->y, dstrect->w, dstrect->h, dstrect->x, dstrect->y);
	}

	if(srcrect->w > canvas_rect.w) {
		const int diff = (srcrect->w - canvas_rect.w);
		srcrect->w -= diff;
		srcrect->x += diff / 2;
	}
	if(srcrect->h > canvas_rect.h) {
		const int diff = (srcrect->h - canvas_rect.h);
		srcrect->h -= diff;
		srcrect->y += diff / 2;
	}
	nqiv_log_write(manager->logger, NQIV_LOG_DEBUG, "Adjust - CanvasRect: %dx%d+%dx%d SrcRect: %dx%d+%dx%d DstRect: %dx%d+%dx%d\n", canvas_rect.w, canvas_rect.h, canvas_rect.x, canvas_rect.y, srcrect->w, srcrect->h, srcrect->x, srcrect->y, dstrect->w, dstrect->h, dstrect->x, dstrect->y);

	if(do_zoom) {
		srcrect->x += (int)( (double)srcrect->x * manager->zoom.viewport_horizontal_shift );
		srcrect->y += (int)( (double)srcrect->y * manager->zoom.viewport_vertical_shift );
		nqiv_log_write(manager->logger, NQIV_LOG_DEBUG, "Move - Horizontal Shift: %f Vertical Shift: %f\n", manager->zoom.viewport_horizontal_shift, manager->zoom.viewport_vertical_shift);
		nqiv_log_write(manager->logger, NQIV_LOG_DEBUG, "Move - CanvasRect: %dx%d+%dx%d SrcRect: %dx%d+%dx%d DstRect: %dx%d+%dx%d\n", canvas_rect.w, canvas_rect.h, canvas_rect.x, canvas_rect.y, srcrect->w, srcrect->h, srcrect->x, srcrect->y, dstrect->w, dstrect->h, dstrect->x, dstrect->y);
	}

	if(!do_stretch) {
		const int display_width = dstrect->w;
		const int display_height = dstrect->h;
		const double canvas_dst_w_ratio = (double)dstrect->w / (double)canvas_rect.w;
		const double canvas_dst_h_ratio = (double)dstrect->h / (double)canvas_rect.h;
		const int new_src_w = (int)( (double)srcrect->w * canvas_dst_w_ratio );
		const int new_src_h = (int)( (double)srcrect->h * canvas_dst_h_ratio );
		dstrect->w = new_src_w;
		dstrect->h = new_src_h;
		if(dstrect->w < display_width) {
			const int diff = display_width - dstrect->w;
			dstrect->x += diff / 2;
		}
		if(dstrect->h < display_height) {
			const int diff = display_height - dstrect->h;
			dstrect->y += diff / 2;
		}
		nqiv_log_write(manager->logger, NQIV_LOG_DEBUG, "Fit - CanvasRect: %dx%d+%dx%d SrcRect: %dx%d+%dx%d DstRect: %dx%d+%dx%d\n", canvas_rect.w, canvas_rect.h, canvas_rect.x, canvas_rect.y, srcrect->w, srcrect->h, srcrect->x, srcrect->y, dstrect->w, dstrect->h, dstrect->x, dstrect->y);
	}
	assert(dstrect->x >= 0);
	assert(dstrect->y >= 0);
	assert(dstrect->w > 0);
	assert(dstrect->h > 0);
}

void nqiv_image_manager_calculate_zoom_parameters(nqiv_image_manager* manager, SDL_Rect* srcrect, SDL_Rect* dstrect)
{
	assert(manager != NULL);
	assert(srcrect != NULL);
	assert(dstrect != NULL);
	assert(srcrect->w > 0);
	assert(srcrect->h > 0);
	assert(dstrect->w > 0);
	assert(dstrect->h > 0);
	if(srcrect->w > srcrect->h) {
		manager->zoom.fit_level = (double)dstrect->w / (double)srcrect->w;
	} else {
		manager->zoom.fit_level = (double)dstrect->w / (double)srcrect->h;
	}
	manager->zoom.actual_size_level = manager->zoom.fit_level;
	if(manager->zoom.fit_level > 1.0) {
		manager->zoom.image_to_viewport_ratio_max = manager->zoom.fit_level;
	} else {
		manager->zoom.image_to_viewport_ratio_max = 1.0;
		manager->zoom.fit_level = 1.0;
	}
	if(manager->zoom.image_to_viewport_ratio > manager->zoom.image_to_viewport_ratio_max) {
		manager->zoom.image_to_viewport_ratio = manager->zoom.image_to_viewport_ratio_max;
	}
	nqiv_log_write(manager->logger, NQIV_LOG_DEBUG, "Zoom parameters - Viewport ratio: %f/%f Fit level: %f Actual Size Level: %f\n", manager->zoom.image_to_viewport_ratio, manager->zoom.image_to_viewport_ratio_max, manager->zoom.fit_level, manager->zoom.actual_size_level);
}

void nqiv_image_manager_increment_thumbnail_size(nqiv_image_manager* manager)
{
	manager->thumbnail.height += manager->zoom.thumbnail_adjust;
	manager->thumbnail.width += manager->zoom.thumbnail_adjust;
}

void nqiv_image_manager_decrement_thumbnail_size(nqiv_image_manager* manager)
{
	manager->thumbnail.height -= manager->zoom.thumbnail_adjust;
	manager->thumbnail.width -= manager->zoom.thumbnail_adjust;
	if(manager->thumbnail.height <= 0) {
		manager->thumbnail.height = manager->zoom.thumbnail_adjust;
	}
	if(manager->thumbnail.width <= 0) {
		manager->thumbnail.width = manager->zoom.thumbnail_adjust;
	}
}

void nqiv_image_form_delay_frame(nqiv_image_form* form)
{
	const clock_t new_frame_time = clock();
	const clock_t frame_diff = (new_frame_time - form->animation.last_frame_time) / ( (clock_t)CLOCKS_PER_SEC / (clock_t)1000 );
	if(frame_diff < (clock_t)2 << (clock_t)30 && (Uint32)frame_diff <= form->animation.delay) {
		SDL_Delay(form->animation.delay - (Uint32)frame_diff);
	}
	form->animation.last_frame_time = new_frame_time;
}

bool nqiv_image_form_first_frame(nqiv_image_form* form)
{
	assert(form != NULL);
	assert(form->file != NULL);
	assert(form->wand != NULL);
	if(!form->animation.exists) {
		return true;
	}
	MagickResetIterator(form->wand);
	MagickNextImage(form->wand);
	form->animation.frame_rendered = false;
	form->animation.last_frame_time = clock();
	nqiv_image_form_delay_frame(form);
	/* GIFs are 10 FPS by default. Do we need to account for other delays? */
	return true;
}

bool nqiv_image_form_next_frame(nqiv_image_form* form)
{
	assert(form != NULL);
	assert(form->file != NULL);
	assert(form->wand != NULL);
	if(!form->animation.exists) {
		return true;
	}
	if( !MagickHasNextImage(form->wand) ) {
		MagickResetIterator(form->wand);
	}
	MagickNextImage(form->wand);
	form->animation.frame_rendered = false;
	nqiv_image_form_delay_frame(form);
	/* GIFs are 10 FPS by default. Do we need to account for other delays? */
	return true;
}
