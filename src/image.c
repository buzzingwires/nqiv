#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <errno.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <vips/vips.h>

#include "event.h"
#include "array.h"
#include "queue.h"
#include "helpers.h"
#include "image.h"
#include "thumbnail.h"
#include "state.h"

/* Image */
void nqiv_unload_image_form_vips(nqiv_image_form* form)
{
	assert(form != NULL);
	if(form->vips != NULL) {
		g_object_unref(form->vips);
		form->vips = NULL;
	}
}

static void nqiv_unload_texture_ptr(SDL_Texture** texture, const bool destroy)
{
	assert(texture != NULL);
	if(*texture != NULL) {
		if(destroy) {
			SDL_DestroyTexture(*texture);
		}
		*texture = NULL;
	}
}

void nqiv_unload_image_form_texture(nqiv_image_form* form)
{
	nqiv_unload_texture_ptr(&form->texture, form->fallback_texture != form->texture);
}

void nqiv_unload_image_form_fallback_texture(nqiv_image_form* form)
{
	assert(form->texture == NULL || form->texture != form->fallback_texture);
	nqiv_unload_texture_ptr(&form->fallback_texture, true);
}

void nqiv_unload_image_form_all_textures(nqiv_image_form* form)
{
	/* Texture is only destroyed if it does not match fallback texture (since
	 * the pointer to the old texture is copied to its position). Otherwise, it
	 * is just unloaded (pointer set to NULL). And, texture must be different
	 * (or NULL) when destroying fallback. Do this to make absolutely sure both
	 * textures are unloaded. */
	nqiv_unload_image_form_texture(form);
	nqiv_unload_image_form_fallback_texture(form);
	nqiv_unload_image_form_texture(form);
	assert(form->texture == NULL);
	assert(form->fallback_texture == NULL);
}

void nqiv_unload_image_form_surface(nqiv_image_form* form)
{
	assert(form != NULL);
	if(form->surface != NULL) {
		SDL_FreeSurface(form->surface);
		form->surface = NULL;
		assert(form->data != NULL);
		free(form->data);
		form->data = NULL;
	}
}

static void nqiv_unload_image_form(nqiv_image_form* form)
{
	assert(form != NULL);
	nqiv_unload_image_form_vips(form);
	nqiv_unload_image_form_all_textures(form);
	nqiv_unload_image_form_surface(form);
}

static void nqiv_image_destroy(nqiv_image* image)
{
	assert(image != NULL);
	assert(image->parent != NULL);
	assert(image->image.path != NULL);
	nqiv_log_write(image->parent->logger, NQIV_LOG_INFO, "Destroying image %s\n",
	               image->image.path);
	SDL_DestroyMutex(image->lock);
	nqiv_unload_image_form(&image->image);
	nqiv_unload_image_form(&image->thumbnail);
	memset(image->image.path, 0, strlen(image->image.path));
	if(image->thumbnail.path != NULL) {
		memset(image->thumbnail.path, 0, strlen(image->thumbnail.path));
		free(image->thumbnail.path);
	}
	memset(image, 0, sizeof(nqiv_image));
	free(image);
}

static nqiv_image* nqiv_image_create(nqiv_log_ctx* logger, const char* raw_path)
{
	assert(logger != NULL);

	char path[PATH_MAX + 1] = {0};
	if(!nqiv_expand_path(path, PATH_MAX, raw_path)) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Could not expand path for image: %s", raw_path);
		return NULL;
	}

	const size_t path_len = strlen(path);
	if(path_len == 0) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Cannot create image with zero-length path.");
		return NULL;
	}
	const size_t path_size = path_len + 1;
	nqiv_image*  image = (nqiv_image*)calloc(1, sizeof(nqiv_image) + path_size);
	if(image == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to allocate memory for image at path %s",
		               path);
		return image;
	}
	image->lock = SDL_CreateMutex();
	if(image->lock == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to create mutex for image %s (%s)", path,
		               SDL_GetError());
		free(image);
		return NULL;
	}
	image->image.path = ((char*)image) + sizeof(nqiv_image);
	memcpy(image->image.path, path, path_len);
	assert(strcmp(image->image.path, path) == 0);
	nqiv_log_write(logger, NQIV_LOG_DEBUG, "Created image %s\n", image->image.path);
	return image;
}

void nqiv_log_vips_exception(nqiv_log_ctx*          logger,
                             const nqiv_image*      image,
                             const nqiv_image_form* form)
{
	char* error = vips_error_buffer_copy();
	nqiv_log_write(logger, NQIV_LOG_WARNING, "Vips exception for form %s of path %s (%s)\n",
	               NQIV_SAYFORM(image, form), image->image.path, error);
	g_free(error);
}

void nqiv_image_unlock(nqiv_image* image)
{
	SDL_UnlockMutex(image->lock);
}

void nqiv_image_lock(nqiv_image* image)
{
	SDL_LockMutex(image->lock);
}

bool nqiv_image_test_lock(nqiv_image* image)
{
	if(SDL_TryLockMutex(image->lock) != 0) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG,
		               "Failed to lock image %s, from thread %lu.\n", image->image.path,
		               SDL_ThreadID());
		return false;
	}
	return true;
}

/* TODO step frame */
/* TODO input cleanup */
/* TODO Add twice */
/* TODO Detect change */

static ptrdiff_t nqiv_find_space_delimited_idx(const char* string, const int wanted_idx)
{
	const size_t len = strlen(string);
	if(len > PTRDIFF_MAX) {
		return -1;
	}
	int       i_idx = -1;
	ptrdiff_t c_idx;
	bool      in_section = false;
	bool      found = false;
	for(c_idx = 0; (size_t)c_idx < len; ++c_idx) {
		const char c = string[c_idx];
		if(!in_section) {
			if(c != ' ') {
				in_section = true;
				++i_idx;
				if(i_idx == wanted_idx) {
					found = true;
					break;
				}
			}
		} else if(c == ' ') {
			in_section = false;
		}
	}
	return found ? c_idx : -1;
}

static bool nqiv_image_form_set_frame_delay(nqiv_image* image, nqiv_image_form* form)
{
	char* delay_string;
	if(vips_image_get_as_string(form->vips, "delay", &delay_string) == -1) {
		nqiv_log_vips_exception(image->parent->logger, image, form);
		form->error = true;
		return false;
	}
	const ptrdiff_t idx = nqiv_find_space_delimited_idx(delay_string, form->animation.frame);
	if(idx == -1) {
		g_free(delay_string);
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Unable to find delay for frame %d of %s\n", form->animation.frame,
		               image->image.path);
		form->error = true;
		return false;
	}
	assert(idx >= 0);
	char*          end = NULL;
	const long int delay_value = strtol(delay_string + idx, &end, 10);
	g_free(delay_string);
	if(errno == ERANGE || delay_value > INT_MAX || end == NULL || delay_string + idx == end) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Delay for frame %d of '%s' is not a valid integer.\n",
		               form->animation.frame, image->image.path);
		form->error = true;
		return false;
	}
	if(delay_value < 0) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Invalid delay of %d for frame %d of %s\n", delay_value,
		               form->animation.frame, image->image.path);
		form->error = true;
		return false;
	}
	if(delay_value == 0) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG,
		               "Delay for frame %d of '%s' is zero. Setting to default of %d\n",
		               form->animation.frame, image->image.path, image->parent->default_frame_time);
		form->animation.delay = image->parent->default_frame_time;
	} else {
		form->animation.delay = delay_value; /* Delay is in milliseconds for vips */
	}
	return true;
}

bool nqiv_image_load_vips(nqiv_image* image, nqiv_image_form* form)
{
	assert(image != NULL);
	assert(form != NULL);
	assert(form->vips == NULL);
	if(form->path == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING, "No path for %s form in image %s\n",
		               NQIV_SAYFORM(image, form), image->image.path);
		form->error = true;
		return false;
	}
	form->vips = vips_image_new_from_file(form->path, NULL);
	if(form->vips == NULL || !vips_colourspace_issupported(form->vips)) {
		nqiv_log_vips_exception(image->parent->logger, image, form);
		form->error = true;
		return false;
	}

	const int old_width = form->width;
	const int old_height = form->height;
	form->width = vips_image_get_width(form->vips);
	form->height = vips_image_get_height(form->vips);
	if(old_width != form->width || old_height != form->height) {
		form->srcrect.x = 0;
		form->srcrect.y = 0;
		form->srcrect.w = form->width;
		form->srcrect.h = form->height;
	}

	form->animation.frame_count = vips_image_get_n_pages(form->vips);
	form->animation.frame = 0;
	form->animation.exists = false;

	if(form->animation.frame_count > 1) {
		form->animation.exists = true;
		if(!nqiv_image_form_set_frame_delay(image, form)) {
			nqiv_log_vips_exception(image->parent->logger, image, form);
			form->error = true;
			return false;
		}
		g_object_unref(form->vips);
		form->vips = vips_image_new_from_file(form->path, "n", form->animation.frame_count, NULL);
		if(form->vips == NULL) {
			nqiv_log_vips_exception(image->parent->logger, image, form);
			form->error = true;
			return false;
		}
	}

	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Form %s vips loaded for image %s\n",
	               NQIV_SAYFORM(image, form), image->image.path);
	return true;
}

static bool nqiv_image_load_raw(nqiv_image* image, nqiv_image_form* form)
{
	assert(image != NULL);
	assert(form != NULL);
	assert(form->vips != NULL);
	assert(form->data == NULL);

	const int frame_offset = form->height * (form->animation.exists ? form->animation.frame : 0);

	VipsImage* used_vips = form->vips;
	VipsImage* new_vips;
	if(form->srcrect.x != 0 || form->srcrect.y + frame_offset != 0
	   || form->srcrect.w != vips_image_get_width(used_vips)
	   || form->srcrect.h != vips_image_get_height(used_vips)) {
		if(vips_crop(used_vips, &new_vips, form->srcrect.x, form->srcrect.y + frame_offset,
		             form->srcrect.w, form->srcrect.h, NULL)
		   == -1) {
			nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
			               "Failed to crop out oversized vips region to resize of form %s of %s\n",
			               NQIV_SAYFORM(image, form), image->image.path);
			form->error = true;
			return false;
		}
		used_vips = new_vips;
		nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG,
		               "Cropped selection from %dx%d+%dx%d to %dx%d for form %s of %s\n",
		               form->srcrect.w, form->srcrect.h, form->srcrect.x, form->srcrect.y,
		               vips_image_get_width(used_vips), vips_image_get_height(used_vips),
		               NQIV_SAYFORM(image, form), image->image.path);
	}

	if(form->srcrect.w > image->parent->max_texture_width
	   || form->srcrect.h > image->parent->max_texture_height) {
		const int largest_dimension = NQIV_MAX(form->srcrect.w, form->srcrect.h);
		const int smallest_texture_dimension =
			NQIV_MIN(image->parent->max_texture_height, image->parent->max_texture_width);
		const double resize_ratio = (double)smallest_texture_dimension / (double)largest_dimension;
		if(vips_resize(used_vips, &new_vips, resize_ratio, NULL) == -1) {
			if(used_vips != form->vips) {
				g_object_unref(used_vips);
			}
			nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
			               "Failed to resize oversized vips region for form %s of %s",
			               NQIV_SAYFORM(image, form), image->image.path);
			form->error = true;
			return false;
		}
		if(used_vips != form->vips) {
			g_object_unref(used_vips);
		}
		used_vips = new_vips;
		nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG,
		               "Resized oversized selection %dx%d+%dx%d to %dx%d for form %s of %s\n",
		               form->srcrect.w, form->srcrect.h, form->srcrect.x, form->srcrect.y,
		               vips_image_get_width(used_vips), vips_image_get_height(used_vips),
		               NQIV_SAYFORM(image, form), image->image.path);
	}

	const VipsBandFormat band_format = vips_image_get_format(used_vips);
	if(band_format == VIPS_FORMAT_NOTSET) {
		if(used_vips != form->vips) {
			g_object_unref(used_vips);
		}
		nqiv_log_vips_exception(image->parent->logger, image, form);
		form->error = true;
		return false;
	}

	const VipsInterpretation interpretation = vips_image_get_interpretation(used_vips);
	if(interpretation == VIPS_INTERPRETATION_ERROR) {
		if(used_vips != form->vips) {
			g_object_unref(used_vips);
		}
		nqiv_log_vips_exception(image->parent->logger, image, form);
		form->error = true;
		return false;
	}

	if(vips_icc_present() != 0) {
		if(vips_icc_transform(used_vips, &new_vips, "srgb", "intent", VIPS_INTENT_PERCEPTUAL,
		                      "embedded", TRUE, NULL)
		   == -1) {
			if(used_vips != form->vips) {
				g_object_unref(used_vips);
			}
			nqiv_log_vips_exception(image->parent->logger, image, form);
			form->error = true;
			return false;
		}
		if(used_vips != form->vips) {
			g_object_unref(used_vips);
		}
		used_vips = new_vips;
	} else if(interpretation != VIPS_INTERPRETATION_sRGB) {
		if(vips_colourspace(used_vips, &new_vips, VIPS_INTERPRETATION_sRGB, NULL) == -1) {
			if(used_vips != form->vips) {
				g_object_unref(used_vips);
			}
			nqiv_log_vips_exception(image->parent->logger, image, form);
			form->error = true;
			return false;
		}
		if(used_vips != form->vips) {
			g_object_unref(used_vips);
		}
		used_vips = new_vips;
	}

	if(band_format != VIPS_FORMAT_UCHAR) {
		if(vips_cast(used_vips, &new_vips, VIPS_FORMAT_UCHAR, "shift", TRUE, NULL) == -1) {
			if(used_vips != form->vips) {
				g_object_unref(used_vips);
			}
			nqiv_log_vips_exception(image->parent->logger, image, form);
			form->error = true;
			return false;
		}
		if(used_vips != form->vips) {
			g_object_unref(used_vips);
		}
		used_vips = new_vips;
	}

	if(!vips_image_hasalpha(used_vips)) {
		if(vips_addalpha(used_vips, &new_vips, NULL) == -1) {
			if(used_vips != form->vips) {
				g_object_unref(used_vips);
			}
			nqiv_log_vips_exception(image->parent->logger, image, form);
			form->error = true;
			return false;
		}
		if(used_vips != form->vips) {
			g_object_unref(used_vips);
		}
		used_vips = new_vips;
	}

	const void* extracted = vips_image_get_data(used_vips);
	if(extracted == NULL) {
		if(used_vips != form->vips) {
			g_object_unref(used_vips);
		}
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Failed to extract raw image data for form %s of %s\n",
		               NQIV_SAYFORM(image, form), image->image.path);
		form->error = true;
		return false;
	}

	const size_t data_size = VIPS_IMAGE_SIZEOF_PEL(used_vips) * VIPS_IMAGE_N_PELS(used_vips);

	form->data = calloc(1, data_size);
	if(form->data == NULL) {
		if(used_vips != form->vips) {
			g_object_unref(used_vips);
		}
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Failed to allocate memory for raw image data for form %s of %s\n",
		               NQIV_SAYFORM(image, form), image->image.path);
		form->error = true;
		return false;
	}

	memcpy(form->data, extracted, data_size);
	form->effective_width = vips_image_get_width(used_vips);
	form->effective_height = vips_image_get_height(used_vips);
	if(used_vips != form->vips) {
		g_object_unref(used_vips);
	}
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG,
	               "Loaded raw of size %zu for image form %s frame %d with pixel offset %d at "
	               "delay of %d for image path %s\n",
	               data_size, NQIV_SAYFORM(image, form), form->animation.frame, frame_offset,
	               form->animation.delay, image->image.path);
	return true;
}

bool nqiv_image_load_surface(nqiv_image* image, nqiv_image_form* form)
{
	assert(image != NULL);
	assert(form != NULL);

	if(!nqiv_image_load_raw(image, form)) {
		return false;
	}

	assert(form->effective_width > 0);
	assert(form->effective_height > 0);

	form->surface = SDL_CreateRGBSurfaceWithFormatFrom(
		form->data, form->effective_width, form->effective_height, 4 * 8, 4 * form->effective_width,
		SDL_PIXELFORMAT_ABGR8888);
	if(form->surface == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Failed to create SDL surface for form %s of %s (%s).",
		               NQIV_SAYFORM(image, form), image->image.path, SDL_GetError());
		form->error = true;
		return false;
	}
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Loaded surface of form %s of image %s\n",
	               NQIV_SAYFORM(image, form), image->image.path);
	return true;
}

int nqiv_lookup_vips_png_comment(gchar** values, const char* key)
{
	const size_t keylen = strlen(key);
	int          result = -1;
	int          idx = 0;
	while(values[idx] != NULL) {
		const size_t valuelen = strlen(values[idx]);
		if(keylen > valuelen) {
			/* NOOP */
		} else if(strncmp(values[idx] + valuelen - keylen, key, keylen) == 0) {
			result = idx;
			break;
		}
		++idx;
	}
	return result;
}

bool nqiv_image_borrow_thumbnail_dimensions(nqiv_image* image)
{
	assert(image != NULL);
	if(image->thumbnail.vips == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG,
		               "Not borrowing dimension metadata from thumbnail because the vips image is "
		               "unavailable for image %s\n",
		               image->image.path);
		return true;
	}
	if(image->image.width != 0 || image->image.height != 0) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG,
		               "Not borrowing dimension metadata from thumbnail because it's already set "
		               "for image %s\n",
		               image->image.path);
		return true;
	}
	gchar** header_field_names = vips_image_get_fields(image->thumbnail.vips);
	if(header_field_names == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Failed to get vips header field names for image %s\n", image->image.path);
		return false;
	}
	const int width_string_idx =
		nqiv_lookup_vips_png_comment(header_field_names, "Thumb::Image::Width");
	if(width_string_idx == -1) {
		g_strfreev(header_field_names);
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Failed to lookup width metadata from thumbnail for %s\n",
		               image->image.path);
		return false;
	}
	const int height_string_idx =
		nqiv_lookup_vips_png_comment(header_field_names, "Thumb::Image::Height");
	if(height_string_idx == -1) {
		g_strfreev(header_field_names);
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Failed to lookup height metadata from thumbnail for %s\n",
		               image->image.path);
		return false;
	}
	const char* width_string;
	if(vips_image_get_string(image->thumbnail.vips, header_field_names[width_string_idx],
	                         &width_string)
	   == -1) {
		g_strfreev(header_field_names);
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Failed to get width metadata from thumbnail for %s\n", image->image.path);
		return false;
	}
	const char* height_string;
	if(vips_image_get_string(image->thumbnail.vips, header_field_names[height_string_idx],
	                         &height_string)
	   == -1) {
		g_strfreev(header_field_names);
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Failed to get height metadata from thumbnail for %s\n", image->image.path);
		return false;
	}
	g_strfreev(header_field_names);
	char*     end = NULL;
	const int width_value = nqiv_strtoi(width_string, &end, 10);
	if(width_value <= 0 || errno == ERANGE || end == NULL || width_string == end) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Invalid width for thumbnail of %s\n", image->image.path);
		return false;
	}
	end = NULL;
	const int height_value = nqiv_strtoi(height_string, &end, 10);
	if(height_value <= 0 || errno == ERANGE || end == NULL || height_string == end) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Invalid height for thumbnail of %s\n", image->image.path);
		return false;
	}
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG,
	               "Borrowing dimension metadata from thumbnail set for image %s\n",
	               image->image.path);
	image->image.width = width_value;
	image->image.height = height_value;
	return true;
}

static bool nqiv_image_is_form_loaded(const nqiv_image_form* form)
{
	assert((form->data == NULL && form->surface == NULL)
	       || (form->data != NULL && form->surface != NULL));
	return form->vips != NULL || form->surface != NULL || form->texture != NULL;
}

bool nqiv_image_has_loaded_form(nqiv_image* image)
{
	return nqiv_image_is_form_loaded(&image->thumbnail) || nqiv_image_is_form_loaded(&image->image);
}

/* Image manager */
void nqiv_image_manager_destroy(nqiv_image_manager* manager)
{
	if(manager == NULL) {
		return;
	}
	nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Destroying image manager.\n");

	if(manager->images != NULL) {
		const int    num_images = nqiv_array_get_units_count(manager->images);
		nqiv_image** images = manager->images->data;
		int          idx;
		for(idx = 0; idx < num_images; ++idx) {
			nqiv_image_destroy(images[idx]);
		}
		nqiv_array_destroy(manager->images);
	}
	if(manager->thumbnail.root != NULL) {
		free(manager->thumbnail.root);
	}
	memset(manager, 0, sizeof(nqiv_image_manager));
}

bool nqiv_image_manager_init(nqiv_image_manager* manager,
                             nqiv_log_ctx*       logger,
                             const int           starting_length)
{
	if(logger == NULL) {
		return false;
	}
	if(starting_length <= 0) {
		nqiv_log_write(logger, NQIV_LOG_ERROR,
		               "Cannot make image manager with starting length of: %d", starting_length);
		return false;
	}
	nqiv_array* images = nqiv_array_create(sizeof(nqiv_array*), starting_length);
	if(images == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR,
		               "Cannot make image manager images array with starting length of: %d",
		               starting_length);
		return false;
	}
	nqiv_array_unlimit_data(images);
	images->min_add_count = STARTING_QUEUE_LENGTH;
	nqiv_image_manager_destroy(manager);
	manager->logger = logger;
	manager->images = images;
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

	manager->zoom.pan_left_amount_more = -0.2;
	manager->zoom.pan_right_amount_more = 0.2;
	manager->zoom.pan_up_amount_more = -0.2;
	manager->zoom.pan_down_amount_more = 0.2;
	manager->zoom.pan_coordinate_x_multiplier = -2.0;
	manager->zoom.pan_coordinate_y_multiplier = -2.0;
	manager->zoom.zoom_in_amount_more = -0.2;
	manager->zoom.zoom_out_amount_more = 0.2;
	manager->zoom.thumbnail_adjust_more = 50;

	SDL_AtomicSet(&manager->thumbnail.size, 256);

	manager->default_frame_time = 100;

	nqiv_log_write(logger, NQIV_LOG_INFO,
	               "Successfully made image manager with starting length of: %d\n",
	               starting_length);
	return true;
}

bool nqiv_image_manager_insert(nqiv_image_manager* manager, const char* path, const int index)
{
	nqiv_image* image = nqiv_image_create(manager->logger, path);
	if(image == NULL) {
		return false;
	}
	const int images_length = nqiv_array_get_units_count(manager->images);
	if(index > images_length) {
		nqiv_image_destroy(image);
		nqiv_log_write(manager->logger, NQIV_LOG_ERROR,
		               "Cannot insert image from path '%s' at index %d, greater than the length of "
		               "the current images array %d.\n",
		               path, index, images_length);
		return false;
	}
	if(!nqiv_array_insert(manager->images, &image, index)) {
		nqiv_image_destroy(image);
		nqiv_log_write(manager->logger, NQIV_LOG_ERROR,
		               "Failed to add image at path '%s' to image manager at index %d.\n", path,
		               index);
		return false;
	}
	nqiv_log_write(manager->logger, NQIV_LOG_INFO,
	               "Added image at path '%s' to image manager at index %d.\n", path, index);
	image->parent = manager;
	return true;
}

bool nqiv_image_manager_remove(nqiv_image_manager* manager, const int index)
{
	nqiv_log_write(manager->logger, NQIV_LOG_INFO,
	               "Removing image from index %d from image manager.\n", index);
	const int images_length = nqiv_array_get_units_count(manager->images);
	if(index >= images_length) {
		nqiv_log_write(manager->logger, NQIV_LOG_ERROR,
		               "Cannot remove image at index %d, greater than the length of the current "
		               "images array %d.\n",
		               index, images_length);
		return false;
	}
	nqiv_image* image = NULL;
	nqiv_array_get(manager->images, index, &image);
	nqiv_image_destroy(image);
	nqiv_array_remove(manager->images, index);
	return true;
}

bool nqiv_image_manager_append(nqiv_image_manager* manager, const char* path)
{
	nqiv_image* image = nqiv_image_create(manager->logger, path);
	if(image == NULL) {
		return false;
	}
	if(!nqiv_array_push(manager->images, &image)) {
		nqiv_image_destroy(image);
		nqiv_log_write(manager->logger, NQIV_LOG_ERROR,
		               "Failed to add image at path '%s' to image manager.", path);
		return false;
	}
	nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Added image at path '%s' to image manager.\n",
	               path);
	image->parent = manager;
	return true;
}

bool nqiv_image_manager_set_thumbnail_root(nqiv_image_manager* manager, const char* path)
{
	if(manager->thumbnail.root != NULL) {
		free(manager->thumbnail.root);
		manager->thumbnail.root = NULL;
	}
	if(strlen(path) == 0) {
		return true;
	}
	const size_t path_len = strlen(path);
	char*        path_ptr = (char*)calloc(1, path_len + 1);
	if(path_ptr == NULL) {
		nqiv_log_write(manager->logger, NQIV_LOG_ERROR,
		               "Failed to create buffer to store thumbnail root of %s", path);
		return false;
	}
	manager->thumbnail.root = path_ptr;
	memcpy(manager->thumbnail.root, path, path_len);
	return true;
}

static void nqiv_image_calculate_zoom_dimension(const double least,
                                                const bool   inclusive_least,
                                                const double catch_point,
                                                const double most,
                                                const bool   inclusive_most,
                                                double*      target,
                                                const double amount)
{
	double new_target = *target + amount;
	if((*target < catch_point && new_target > catch_point)
	   || (*target > catch_point && new_target < catch_point)) {
		new_target = catch_point;
	}
	if(inclusive_least) {
		new_target = NQIV_MAX(new_target, least);
	} else {
		new_target = new_target <= least ? *target : new_target;
	}
	if(inclusive_most) {
		new_target = NQIV_MIN(new_target, most);
	} else {
		new_target = new_target >= most ? *target : new_target;
	}
	*target = new_target;
}

void nqiv_image_manager_pan_left(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(-1.0, true, 0.0, 1.0, true,
	                                    &manager->zoom.viewport_horizontal_shift,
	                                    manager->zoom.pan_left_amount);
}

void nqiv_image_manager_pan_right(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(-1.0, true, 0.0, 1.0, true,
	                                    &manager->zoom.viewport_horizontal_shift,
	                                    manager->zoom.pan_right_amount);
}

void nqiv_image_manager_pan_up(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(-1.0, true, 0.0, 1.0, true,
	                                    &manager->zoom.viewport_vertical_shift,
	                                    manager->zoom.pan_up_amount);
}

void nqiv_image_manager_pan_down(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(-1.0, true, 0.0, 1.0, true,
	                                    &manager->zoom.viewport_vertical_shift,
	                                    manager->zoom.pan_down_amount);
}

void nqiv_image_manager_pan_coordinates(nqiv_image_manager* manager, const SDL_Rect* coordinates)
{
	nqiv_image_calculate_zoom_dimension(-1.0, true, 0.0, 1.0, true,
	                                    &manager->zoom.viewport_horizontal_shift,
	                                    ((double)coordinates->x / (double)coordinates->w)
	                                        * manager->zoom.pan_coordinate_x_multiplier);
	nqiv_image_calculate_zoom_dimension(-1.0, true, 0.0, 1.0, true,
	                                    &manager->zoom.viewport_vertical_shift,
	                                    ((double)coordinates->y / (double)coordinates->h)
	                                        * manager->zoom.pan_coordinate_y_multiplier);
}

static double nqiv_image_managet_get_smallest_zoom(nqiv_image_manager* manager)
{
	return NQIV_MIN(fabs(manager->zoom.zoom_in_amount), manager->zoom.actual_size_level);
}

void nqiv_image_manager_zoom_in(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(
		nqiv_image_managet_get_smallest_zoom(manager), true, manager->zoom.actual_size_level,
		manager->zoom.image_to_viewport_ratio_max, true, &manager->zoom.image_to_viewport_ratio,
		manager->zoom.zoom_in_amount);
}

void nqiv_image_manager_zoom_out(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(
		nqiv_image_managet_get_smallest_zoom(manager), true, manager->zoom.actual_size_level,
		manager->zoom.image_to_viewport_ratio_max, true, &manager->zoom.image_to_viewport_ratio,
		manager->zoom.zoom_out_amount);
}

void nqiv_image_manager_pan_center(nqiv_image_manager* manager)
{
	manager->zoom.viewport_horizontal_shift = 0.0;
	manager->zoom.viewport_vertical_shift = 0.0;
}

void nqiv_image_manager_pan_left_more(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(-1.0, true, 0.0, 1.0, true,
	                                    &manager->zoom.viewport_horizontal_shift,
	                                    manager->zoom.pan_left_amount_more);
}

void nqiv_image_manager_pan_right_more(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(-1.0, true, 0.0, 1.0, true,
	                                    &manager->zoom.viewport_horizontal_shift,
	                                    manager->zoom.pan_right_amount_more);
}

void nqiv_image_manager_pan_up_more(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(-1.0, true, 0.0, 1.0, true,
	                                    &manager->zoom.viewport_vertical_shift,
	                                    manager->zoom.pan_up_amount_more);
}

void nqiv_image_manager_pan_down_more(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(-1.0, true, 0.0, 1.0, true,
	                                    &manager->zoom.viewport_vertical_shift,
	                                    manager->zoom.pan_down_amount_more);
}

void nqiv_image_manager_zoom_in_more(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(
		nqiv_image_managet_get_smallest_zoom(manager), true, manager->zoom.actual_size_level,
		manager->zoom.image_to_viewport_ratio_max, true, &manager->zoom.image_to_viewport_ratio,
		manager->zoom.zoom_in_amount_more);
}

void nqiv_image_manager_zoom_out_more(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(
		nqiv_image_managet_get_smallest_zoom(manager), true, manager->zoom.actual_size_level,
		manager->zoom.image_to_viewport_ratio_max, true, &manager->zoom.image_to_viewport_ratio,
		manager->zoom.zoom_out_amount_more);
}

static void nqiv_image_manager_calculate_zoomrect(nqiv_image_manager* manager,
                                                  const bool          do_zoom,
                                                  const bool          do_stretch,
                                                  SDL_Rect*           srcrect,
                                                  SDL_Rect*           dstrect)
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

	double srcrect_w = (double)srcrect->w;
	double srcrect_h = (double)srcrect->h;
	double srcrect_x = (double)srcrect->x;
	double srcrect_y = (double)srcrect->y;
	double dstrect_w = (double)dstrect->w;
	double dstrect_h = (double)dstrect->h;
	double dstrect_x = (double)dstrect->x;
	double dstrect_y = (double)dstrect->y;

	/* We fit the sample area (srcrect) for the image inside of a canvas made using its largest side
	 * matching, and the smaller side changed so that the aspect ratio matches that of the viewport
	 * (dstrect). */
	double canvas_rect_w = srcrect_w;
	double canvas_rect_h = srcrect_h;
	if(srcrect_w > srcrect_h) {
		canvas_rect_h = srcrect_w * (dstrect_h / dstrect_w);
	} else {
		canvas_rect_w = srcrect_h * (dstrect_w / dstrect_h);
	}

	/* Apply basic zooming to the canvas. */
	if(do_zoom) {
		canvas_rect_w *= manager->zoom.image_to_viewport_ratio;
		canvas_rect_h *= manager->zoom.image_to_viewport_ratio;
	}

	/* Align the sample area with the canvas and clip off overflowing edges.  */
	if(srcrect_w > canvas_rect_w) {
		const double diff = srcrect_w - canvas_rect_w;
		srcrect_w -= diff;
		srcrect_x += (diff / 2.0);
	}
	if(srcrect_h > canvas_rect_h) {
		const double diff = srcrect_h - canvas_rect_h;
		srcrect_h -= diff;
		srcrect_y += (diff / 2.0);
	}

	/* Apply basic panning to the canvas. */
	if(do_zoom) {
		srcrect_x += srcrect_x * manager->zoom.viewport_horizontal_shift;
		srcrect_y += srcrect_y * manager->zoom.viewport_vertical_shift;
	}

	const double display_width = dstrect_w;
	const double display_height = dstrect_h;
	if(!do_stretch) {
		/* Get proportion of the viewport to the canvas and use that to scale the viewport to the
		 * sample area. */
		dstrect_w = srcrect_w * (dstrect_w / canvas_rect_w);
		dstrect_h = srcrect_h * (dstrect_h / canvas_rect_h);
		/* Clip the edges of the viewport to match the display, if it's larger, otherwise center it
		 * in the display.  */
		if(dstrect_w > display_width) {
			dstrect_w = display_width;
			dstrect_h = dstrect_h * (display_width / dstrect_w);
		} else if(dstrect_w < display_width) {
			dstrect_x += (display_width - dstrect_w) / 2;
		}
		if(dstrect_h > display_height) {
			dstrect_h = display_height;
			dstrect_w = dstrect_w * (display_height / dstrect_h);
		} else if(dstrect_h < display_height) {
			dstrect_y += (display_height - dstrect_h) / 2;
		}
	}

	/* Round dimensions up to prepare our integer values. */
	srcrect_w = ceil(srcrect_w);
	srcrect_h = ceil(srcrect_h);
	srcrect_x = ceil(srcrect_x);
	srcrect_y = ceil(srcrect_y);
	dstrect_w = ceil(dstrect_w);
	dstrect_h = ceil(dstrect_h);
	dstrect_x = ceil(dstrect_x);
	dstrect_y = ceil(dstrect_y);

	if(!do_stretch) {
		/* Here, we fix rounding errors by calculating the difference between the aspect ratios of
		 * our viewport and sample area. The goal is to get the dimensions to match as closely as
		 * possible.
		 */
		double ratio_diff = dstrect_w / dstrect_h - srcrect_w / srcrect_h;
		bool   ratio_diff_positive = ratio_diff > 0.0;
		while(ratio_diff != 0.0) {
			const double old_w = dstrect_w;
			const double old_h = dstrect_h;
			const double old_x = dstrect_x;
			const double old_y = dstrect_y;
			/* If the aspect ratio of viewport is wider than sample area, we will first aim to
			 * shrink its width, then to grow its height, until we completely run out of room. */
			if(ratio_diff_positive) {
				if(dstrect_w > 0) {
					dstrect_w -= 1.0;
					/* Keep centered. */
					if(display_width - (dstrect_x + dstrect_w) > dstrect_x + 1.0) {
						dstrect_x += 1.0;
					}
				} else if(dstrect_h < display_height) {
					dstrect_h += 1.0;
					if((display_height - (dstrect_y + dstrect_h)) + 1.0 < dstrect_y) {
						dstrect_y -= 1.0;
					}
				}
			} else {
				/* If narrower, do the inverse. */
				if(dstrect_w < display_width) {
					dstrect_w += 1.0;
					if((display_width - (dstrect_x + dstrect_w)) + 1.0 < dstrect_x) {
						dstrect_x -= 1.0;
					}
				} else if(dstrect_h > 0) {
					dstrect_h -= 1.0;
					if(display_height - (dstrect_y + dstrect_h) > dstrect_y + 1.0) {
						dstrect_y += 1.0;
					}
				}
			}
			/* Check the new difference, if it is bigger than before, we know we have fit as tightly
			 * as possible. Restore previous values and break. */
			const double new_ratio_diff = dstrect_w / dstrect_h - srcrect_w / srcrect_h;
			if(fabs(new_ratio_diff) > fabs(ratio_diff)) {
				dstrect_w = old_w;
				dstrect_h = old_h;
				dstrect_x = old_x;
				dstrect_y = old_y;
				break;
			}
			ratio_diff = new_ratio_diff;
		}
	}

	srcrect->w = (int)srcrect_w;
	srcrect->h = (int)srcrect_h;
	srcrect->x = (int)srcrect_x;
	srcrect->y = (int)srcrect_y;
	dstrect->w = (int)dstrect_w;
	dstrect->h = (int)dstrect_h;
	dstrect->x = (int)dstrect_x;
	dstrect->y = (int)dstrect_y;

	assert(dstrect->x >= 0);
	assert(dstrect->y >= 0);
	assert(dstrect->w >= 1);
	assert(dstrect->h >= 1);
	assert(dstrect->h <= (int)display_height);
	assert(dstrect->w <= (int)display_width);
}

void nqiv_image_manager_calculate_zoom_parameters(nqiv_image_manager* manager,
                                                  const SDL_Rect*     srcrect,
                                                  const SDL_Rect*     dstrect)
{
	assert(manager != NULL);
	assert(srcrect != NULL);
	assert(dstrect != NULL);
	assert(srcrect->w > 0);
	assert(srcrect->h > 0);
	assert(dstrect->w > 0);
	assert(dstrect->h > 0);
	double src_aspect;
	double dst_aspect;
	/* Basically guestimate fit level based on biggest side of image, making it
	 * proportional to the ratio between it and the screen's corresponding
	 * side. */
	if(srcrect->w > srcrect->h) {
		manager->zoom.fit_level = (double)dstrect->w / (double)srcrect->w;
		src_aspect = (double)srcrect->w / (double)srcrect->h;
	} else {
		src_aspect = (double)srcrect->h / (double)srcrect->w;
		manager->zoom.fit_level = (double)dstrect->h / (double)srcrect->h;
	}
	manager->zoom.actual_size_level = manager->zoom.fit_level;
	if(dstrect->w > dstrect->h) {
		dst_aspect = (double)dstrect->w / (double)dstrect->h;
	} else {
		dst_aspect = (double)dstrect->h / (double)dstrect->w;
	}
	/* If image has a bigger side than display, set fit level to be greater
	 * than 1.0 + (biggest ratio - smallest ratio) */
	if(srcrect->w > dstrect->w || srcrect->h > dstrect->h) {
		if(src_aspect > dst_aspect) {
			manager->zoom.fit_level = 1.0 + (src_aspect - dst_aspect);
		} else {
			manager->zoom.fit_level = 1.0 + (dst_aspect - src_aspect);
		}
	}
	const double original_ratio = manager->zoom.image_to_viewport_ratio;
	manager->zoom.image_to_viewport_ratio = manager->zoom.fit_level;
	manager->zoom.image_to_viewport_ratio_max = manager->zoom.fit_level;
	double current_ratio = manager->zoom.image_to_viewport_ratio;
	bool   ever_set = false;
	/* Get a precisely calculated zoom level by repeatedly calculating the real
	 * zoomrect until the entire image isn't in view. */
	while(true) {
		if(manager->zoom.image_to_viewport_ratio > 0.0) {
			SDL_Rect src = {0};
			src.w = srcrect->w;
			src.h = srcrect->h;
			SDL_Rect dst = {0};
			dst.w = dstrect->w;
			dst.h = dstrect->h;
			nqiv_image_manager_calculate_zoomrect(manager, true, false, &src, &dst);
			if(dst.w <= dstrect->w && dst.h <= dstrect->h) {
				current_ratio = manager->zoom.image_to_viewport_ratio;
				if(dst.w == dstrect->w || dst.h == dstrect->h) {
					if(ever_set) {
						manager->zoom.fit_level = current_ratio;
					}
					break;
				}
			} else {
				if(ever_set) {
					manager->zoom.fit_level = current_ratio;
				}
				break;
			}
		} else {
			if(ever_set) {
				manager->zoom.fit_level = current_ratio;
			}
			break;
		}
		manager->zoom.image_to_viewport_ratio += manager->zoom.zoom_in_amount;
		manager->zoom.image_to_viewport_ratio_max = manager->zoom.image_to_viewport_ratio;
		ever_set = true;
	}
	manager->zoom.image_to_viewport_ratio = original_ratio;
	/* If the image is smaller than the screen, make sure we can zoom out
	 * enough to see its actual size. */
	if(manager->zoom.actual_size_level > manager->zoom.fit_level) {
		manager->zoom.image_to_viewport_ratio_max = manager->zoom.actual_size_level;
	} else {
		manager->zoom.image_to_viewport_ratio_max = manager->zoom.fit_level;
	}
	/* Clamp max ratio at 1.0 */
	if(manager->zoom.image_to_viewport_ratio_max < 1.0) {
		manager->zoom.image_to_viewport_ratio_max = 1.0;
		if(manager->zoom.fit_level < 1.0) {
			manager->zoom.fit_level = 1.0;
		}
	}
	assert(manager->zoom.image_to_viewport_ratio_max >= 1.0);
	nqiv_log_write(manager->logger, NQIV_LOG_DEBUG,
	               "Zoom parameters - Viewport ratio: %f/%f Fit level: %f Actual Size Level: %f\n",
	               manager->zoom.image_to_viewport_ratio, manager->zoom.image_to_viewport_ratio_max,
	               manager->zoom.fit_level, manager->zoom.actual_size_level);
	if(manager->zoom.image_to_viewport_ratio > manager->zoom.image_to_viewport_ratio_max) {
		manager->zoom.image_to_viewport_ratio = manager->zoom.image_to_viewport_ratio_max;
	}
}

void nqiv_image_manager_retrieve_zoomrect(nqiv_image_manager* manager,
                                          const bool          do_zoom,
                                          const bool          do_stretch,
                                          SDL_Rect*           srcrect,
                                          SDL_Rect*           dstrect)
{
	nqiv_image_manager_calculate_zoomrect(manager, do_zoom, do_stretch, srcrect, dstrect);
	nqiv_log_write(manager->logger, NQIV_LOG_DEBUG,
	               "Zoomrect - SrcRect: %dx%d+%dx%d DstRect: %dx%d+%dx%d\n", srcrect->w, srcrect->h,
	               srcrect->x, srcrect->y, dstrect->w, dstrect->h, dstrect->x, dstrect->y);
}

int nqiv_image_manager_get_zoom_percent(nqiv_image_manager* manager)
{
	return (int)((manager->zoom.actual_size_level / manager->zoom.image_to_viewport_ratio) * 100.0);
}

bool nqiv_image_manager_reattempt_thumbnails(nqiv_image_manager* manager, const int old_size)
{
	if(nqiv_thumbnail_get_closest_size(SDL_AtomicGet(&manager->thumbnail.size))
	   <= nqiv_thumbnail_get_closest_size(old_size)) {
		return true;
	}
	const int    num_images = nqiv_array_get_units_count(manager->images);
	nqiv_image** images = manager->images->data;
	bool         wake = false;
	int          idx;
	for(idx = 0; idx < num_images; ++idx) {
		nqiv_image_lock(images[idx]);
		images[idx]->thumbnail_attempted = false;
		if(nqiv_image_is_form_loaded(&(images[idx]->thumbnail))) {
			if(images[idx]->thumbnail.texture != NULL
			   || images[idx]->thumbnail.fallback_texture != NULL) {
				nqiv_unload_image_form_all_textures(&images[idx]->thumbnail);
			}
			if(images[idx]->thumbnail.path != NULL) {
				free(images[idx]->thumbnail.path);
				images[idx]->thumbnail.path = NULL;
			}
			if(images[idx]->thumbnail.vips != NULL || images[idx]->thumbnail.surface != NULL) {
				nqiv_event event = {0};
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.transaction_group = -1;
				event.options.image_load.image = images[idx];
				event.options.image_load.thumbnail_options.unload = true;
				event.options.image_load.thumbnail_options.vips =
					images[idx]->thumbnail.vips != NULL;
				event.options.image_load.thumbnail_options.surface =
					images[idx]->thumbnail.surface != NULL;
				if(!nqiv_priority_queue_push(manager->thread_queue,
				                             NQIV_EVENT_PRIORITY_REATTEMPT_THUMBNAIL, &event)) {
					nqiv_image_unlock(images[idx]);
					return false;
				}
				wake = true;
			}
		}
		nqiv_image_unlock(images[idx]);
	}
	if(wake) {
		nqiv_cond_wake_all(manager->thread_wakeup_signaler);
	}
	return true;
}

static void nqiv_image_manager_increment_thumbnail_size_base(nqiv_image_manager* manager,
                                                             const int           adjust)
{
	SDL_AtomicAdd(&manager->thumbnail.size, adjust);
}

static void nqiv_image_manager_decrement_thumbnail_size_base(nqiv_image_manager* manager,
                                                             const int           adjust)
{
	/* This should not be done by a thread. Only master can modify to guarantee value. */
	const int new_size = SDL_AtomicGet(&manager->thumbnail.size) - adjust;
	SDL_AtomicSet(&manager->thumbnail.size, NQIV_MAX(new_size, adjust));
}

void nqiv_image_manager_increment_thumbnail_size(nqiv_image_manager* manager)
{
	nqiv_image_manager_increment_thumbnail_size_base(manager, manager->zoom.thumbnail_adjust);
}

void nqiv_image_manager_decrement_thumbnail_size(nqiv_image_manager* manager)
{
	nqiv_image_manager_decrement_thumbnail_size_base(manager, manager->zoom.thumbnail_adjust);
}

void nqiv_image_manager_increment_thumbnail_size_more(nqiv_image_manager* manager)
{
	nqiv_image_manager_increment_thumbnail_size_base(manager, manager->zoom.thumbnail_adjust_more);
}

void nqiv_image_manager_decrement_thumbnail_size_more(nqiv_image_manager* manager)
{
	nqiv_image_manager_decrement_thumbnail_size_base(manager, manager->zoom.thumbnail_adjust_more);
}

static void nqiv_image_form_delay_frame(nqiv_image_form* form)
{
	const Uint64 frame_diff = SDL_GetTicks64() - form->animation.last_frame_time;
	if(frame_diff < form->animation.delay) {
		SDL_Delay((Uint32)(form->animation.delay - frame_diff));
	}
	form->animation.last_frame_time = SDL_GetTicks64();
}

bool nqiv_image_form_first_frame(nqiv_image* image, nqiv_image_form* form)
{
	assert(form != NULL);
	assert(form->vips != NULL);
	if(!form->animation.exists) {
		return true;
	}
	form->animation.frame = 0;
	form->animation.frame_rendered = false;
	form->animation.last_frame_time = SDL_GetTicks64();
	if(!nqiv_image_form_set_frame_delay(image, form)) {
		return false;
	}
	return true;
}

bool nqiv_image_form_next_frame(nqiv_image* image, nqiv_image_form* form)
{
	assert(form != NULL);
	assert(form->vips != NULL);
	if(!form->animation.exists) {
		return true;
	}
	if(!form->animation.frame_rendered) {
		return false;
	}
	nqiv_image_form_delay_frame(form);
	form->animation.frame += 1;
	if(form->animation.frame >= form->animation.frame_count) {
		form->animation.frame = 0;
	}
	form->animation.frame_rendered = false;
	if(!nqiv_image_form_set_frame_delay(image, form)) {
		return false;
	}
	return true;
}
