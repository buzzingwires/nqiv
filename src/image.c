#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <MagickCore/MagickCore.h>
#include <MagickWand/MagickWand.h>
#include <omp.h>

#include "array.h"
#include "image.h"

/* Image */
void nqiv_unload_image_form_wand(nqiv_image_form* form)
{
	assert(form != NULL);
	assert(form->file != NULL);
	if(form->wand != NULL) {
		DestroyMagickWand(form->wand); /* TODO: Where should this be? */
		form->wand = NULL;
	}
}

void nqiv_unload_image_form_file(nqiv_image_form* form)
{
	assert(form != NULL);
	assert(form->wand == NULL);
	if(form->file != NULL) {
		fclose(form->file);
		form->file = NULL;
	}
}

void nqiv_unload_image_form_texture(nqiv_image_form* form)
{
	if(form->texture != NULL) {
		SDL_DestroyTexture( (SDL_Texture *)form->data );
		form->texture = NULL;
	}
}

void nqiv_unload_image_form_surface(nqiv_image_form* form)
{
	assert(form != NULL);
	assert(form->data != NULL);
	if(form->surface != NULL) {
		SDL_FreeSurface(form->surface);
		form->surface = NULL;
	}
}

void nqiv_unload_image_form_raw(nqiv_image_form* form)
{
	assert(form != NULL);
	assert(form->surface == NULL);
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
	nqiv_image* image = (nqiv_image*)calloc( 1, sizeof(nqiv_image) );
	if(image == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to allocate memory for image at path %s.", path);
		return image;
	}
	image->image.path = calloc(1, path_len);
	if(image->image.path == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to allocate memory for path data %s.", path);
		nqiv_image_destroy(image);
		return NULL;
	}
	omp_init_lock(&image->lock);
	strncpy(image->image.path, path, path_len + 1);
	assert(strcmp(image->image.path, path) == 0);
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Created image %s.\n", image->image.path);
	return image;
}

void nqiv_log_magick_wand_exception(nqiv_log_ctx* logger, const MagickWand* magick_wand, const char* path)
{
	assert(logger != NULL);
	assert(magick_wand != NULL);
	assert(path != NULL);
	ExceptionType severity;
	char* description = MagickGetException(magick_wand, &severity);
	nqiv_log_write(logger, NQIV_LOG_ERROR, "ImageMagick exception for path %s: %s %s\n", path, GetMagickModule(), description);
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
	form->file = fopen(form->path, "r");
	if(form->file == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to open image file at path %s.", form->path);
		/*nqiv_set_invalid_image_form(form);*/
		form->error = true;
		return false;
	}
	rewind(form->file);
	form->wand = NewMagickWand();
	if(MagickReadImageFile(form->wand, form->file) == MagickFalse) {
		nqiv_log_magick_wand_exception(image->parent->logger, form->wand, form->path);
		nqiv_unload_image_form(form);
		form->error = true;
		return false;
	}
	if( MagickHasNextImage(form->wand) ) {
		form->animated = true;
		form->frame_delta = 0.0;
	}
	form->height = MagickGetImageHeight(form->wand);
	form->width = MagickGetImageWidth(form->wand);
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Loaded wand for image %s.\n", image->image.path);
	/* GIFs are 10 FPS by default. Do we need to account for other delays? */
	return true;
}

bool nqiv_image_load_raw(nqiv_image* image, nqiv_image_form* form)
{
	assert(image != NULL);
	assert(form != NULL);
	assert( (form->file != NULL && form->wand != NULL && form->data == NULL) );
	assert(sizeof(CharPixel) == 1);
	form->data = calloc( 1, strlen("RGBA") * sizeof(CharPixel) * form->height * form->width );
	if(form->data == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to allocate memory for raw image data at path %s.", form->path);
		nqiv_unload_image_form(form);
		form->error = true;
		return false;
	}
	if(MagickExportImagePixels(form->wand, 0, 0, form->width, form->height, "RGBA", CharPixel, form->data) == MagickFalse) {
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
	form->surface = SDL_CreateRGBSurfaceWithFormatFrom(form->data, form->width, form->height, 4 * 8, 4 * form->width, SDL_PIXELFORMAT_RGBA8888);
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

/* Image manager */
void nqiv_image_manager_destroy(nqiv_image_manager* manager)
{
	if(manager == NULL) {
		return;
	}
	nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Destroying image manager.");
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
		nqiv_log_write(manager->logger, NQIV_LOG_ERROR, "Failed to add image at path '%s' to image manager at index %d.", path, index);
		return false;
	}
	// nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Generated image at path '%s'.", path);
	nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Added image at path '%s' to image manager at index %d.", path, index);
	image->parent = manager;
	return true;
}

void nqiv_image_manager_remove(nqiv_image_manager* manager, const int index)
{
	nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Adding image at index %d from image manager.", index);
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
	nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Added image at path '%s' to image manager.", path);
	image->parent = manager;
	return true;
}

bool nqiv_image_manager_add_extension(nqiv_image_manager* manager, char* extension)
{
	const bool outcome = nqiv_array_push_char_ptr(manager->extensions, extension);
	nqiv_log_write(manager->logger, outcome?NQIV_LOG_INFO:NQIV_LOG_ERROR, "Adding extension '%s' to image manager. Success: %s", extension, outcome?"true":"false");
	return outcome;
}

void nqiv_image_manager_calculate_zoomrect(nqiv_image_manager* manager, const nqiv_image_form* form, SDL_Window* window, SDL_Rect* rect)
{
	int window_width;
	int window_height;
	SDL_GetWindowSizeInPixels(window, &window_width, &window_height);
	assert(manager != NULL);
	assert(form != NULL);
	assert(rect != NULL);
	assert(form->height >= 1);
	assert(form->width >= 1);
	assert(window_width > 0);
	assert(window_height > 0);
	assert(manager->zoom.image_to_viewport_ratio > 0.0);
	assert(manager->zoom.image_to_viewport_ratio <= 1.0);
	assert( isnormal(manager->zoom.viewport_vertical_shift) );
	assert( isnormal(manager->zoom.viewport_horizontal_shift) );
	const double height_diff = ( (double)form->height - ( (double)form->height * manager->zoom.image_to_viewport_ratio ) ) / 2;
	const double width_diff = ( (double)form->width - ( (double)form->width * manager->zoom.image_to_viewport_ratio ) ) / 2;
	const double vertical_shift_amount = (double)window_height * manager->zoom.viewport_vertical_shift;
	const double horizontal_shift_amount = (double)window_width * manager->zoom.viewport_horizontal_shift;
	rect->y = (int)(height_diff + vertical_shift_amount);
	rect->x = (int)(width_diff + horizontal_shift_amount);
	rect->y = rect->y >= 0 ? rect->y : 0;
	rect->x = rect->x >= 0 ? rect->x : 0;
	rect->h = (int)(form->height - height_diff * 2 + vertical_shift_amount);
	rect->w = (int)(form->width - width_diff * 2 + horizontal_shift_amount);
	rect->h = rect->h <= form->height ? rect->y : form->height;
	rect->w = rect->w <= form->width ? rect->w : form->width;
	assert(rect->h > rect->y);
	assert(rect->w > rect->x);
}

bool nqiv_image_form_first_frame(nqiv_image_form* form)
{
	assert(form != NULL);
	assert(form->file != NULL);
	assert(form->wand != NULL);
	if(!form->animated) {
		return true;
	}
	MagickResetIterator(form->wand);
	MagickNextImage(form->wand);
	/* GIFs are 10 FPS by default. Do we need to account for other delays? */
	return true;
}

bool nqiv_image_form_next_frame(nqiv_image_form* form)
{
	assert(form != NULL);
	assert(form->file != NULL);
	assert(form->wand != NULL);
	if(!form->animated) {
		return true;
	}
	if( !MagickHasNextImage(form->wand) ) {
		MagickResetIterator(form->wand);
	}
	MagickNextImage(form->wand);
	/* GIFs are 10 FPS by default. Do we need to account for other delays? */
	return true;
}
