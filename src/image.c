#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <vips/vips.h>
#include <omp.h>

#include "event.h"
#include "array.h"
#include "queue.h"
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

void nqiv_unload_texture_ptr(SDL_Texture** texture, const bool destroy)
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
	nqiv_unload_image_form_vips(form);
	nqiv_unload_image_form_all_textures(form);
	nqiv_unload_image_form_surface(form);
	nqiv_unload_image_form_raw(form);
}

void nqiv_image_destroy(nqiv_image* image)
{
	assert(image != NULL);
	assert(image->parent != NULL);
	nqiv_log_write(image->parent->logger, NQIV_LOG_INFO, "Destroying image %s\n",
	               image->image.path);
	omp_destroy_lock(&image->lock);
	nqiv_unload_image_form(&image->image);
	nqiv_unload_image_form(&image->thumbnail);
	if(image->image.path != NULL) {
		free(image->image.path);
	}
	if(image->thumbnail.path != NULL) {
		free(image->thumbnail.path);
	}
	memset(image, 0, sizeof(nqiv_image));
	free(image);
}

nqiv_image* nqiv_image_create(nqiv_log_ctx* logger, const char* raw_path)
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
	nqiv_image*  image = (nqiv_image*)calloc(1, sizeof(nqiv_image));
	if(image == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to allocate memory for image at path %s",
		               path);
		return image;
	}
	image->image.path = calloc(1, path_size);
	if(image->image.path == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to allocate memory for path data %s", path);
		nqiv_image_destroy(image);
		return NULL;
	}
	omp_init_lock(&image->lock);
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
	               form == &image->image ? "image" : "thumbnail", image->image.path, error);
	g_free(error);
}

void nqiv_image_unlock(nqiv_image* image)
{
	omp_unset_lock(&image->lock);
}

void nqiv_image_lock(nqiv_image* image)
{
	omp_set_lock(&image->lock);
}

bool nqiv_image_test_lock(nqiv_image* image)
{
	if(!omp_test_lock(&image->lock)) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG,
		               "Failed to lock image %s, from thread %d.\n", image->image.path,
		               omp_get_thread_num());
		return false;
	}
	return true;
}

/* TODO step frame */
/* TODO input cleanup */
/* TODO Add twice */
/* TODO Detect change */

ptrdiff_t nqiv_find_space_delimited_idx(const char* string, const int wanted_idx)
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

bool nqiv_image_form_set_frame_delay(nqiv_image* image, nqiv_image_form* form)
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
	const long int delay_value = strtol(delay_string + idx, NULL, 10);
	g_free(delay_string);
	if(errno == ERANGE || delay_value > INT_MAX) {
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
		               form == &image->image ? "image" : "thumbnail", image->image.path);
		form->error = true;
		return false;
	}
	form->vips = vips_image_new_from_file(form->path, NULL);
	if(form->vips == NULL || !vips_colourspace_issupported(form->vips)) {
		nqiv_log_vips_exception(image->parent->logger, image, form);
		form->error = true;
		return false;
	}

	form->width = vips_image_get_width(form->vips);
	form->height = vips_image_get_height(form->vips);
	form->srcrect.x = 0;
	form->srcrect.y = 0;
	form->srcrect.w = form->width;
	form->srcrect.h = form->height;
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

	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Form %s vips for loaded for image %s\n",
	               form == &image->image ? "image" : "thumbnail", image->image.path);
	return true;
}

bool nqiv_image_load_raw(nqiv_image* image, nqiv_image_form* form)
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
			               form == &image->image ? "image" : "thumbnail", image->image.path);
			form->error = true;
			return false;
		}
		used_vips = new_vips;
		nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG,
		               "Cropped selection from %dx%d+%dx%d to %dx%d for form %s of %s\n",
		               form->srcrect.w, form->srcrect.h, form->srcrect.x, form->srcrect.y,
		               vips_image_get_width(used_vips), vips_image_get_height(used_vips),
		               form == &image->image ? "image" : "thumbnail", image->image.path);
	}

	if(form->srcrect.w > image->parent->max_texture_width
	   || form->srcrect.h > image->parent->max_texture_height) {
		const int largest_dimension =
			form->srcrect.w > form->srcrect.h ? form->srcrect.w : form->srcrect.h;
		const int smallest_texture_dimension =
			image->parent->max_texture_width > image->parent->max_texture_height
				? image->parent->max_texture_height
				: image->parent->max_texture_width;
		const double resize_ratio = (double)smallest_texture_dimension / (double)largest_dimension;
		if(vips_resize(used_vips, &new_vips, resize_ratio, NULL) == -1) {
			if(used_vips != form->vips) {
				g_object_unref(used_vips);
			}
			nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
			               "Failed to resize oversized vips region for form %s of %s",
			               form == &image->image ? "image" : "thumbnail", image->image.path);
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
		               form == &image->image ? "image" : "thumbnail", image->image.path);
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

	const VipsInterpretation interpretation = vips_image_get_interpretation(used_vips);
	if(interpretation == VIPS_INTERPRETATION_ERROR) {
		if(used_vips != form->vips) {
			g_object_unref(used_vips);
		}
		nqiv_log_vips_exception(image->parent->logger, image, form);
		form->error = true;
		return false;
	}
	if(interpretation != VIPS_INTERPRETATION_RGB && interpretation != VIPS_INTERPRETATION_sRGB) {
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
		               form == &image->image ? "image" : "thumbnail", image->image.path);
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
		               form == &image->image ? "image" : "thumbnail", image->image.path);
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
	               "delay of %d at path %s\n",
	               data_size, form == &image->image ? "image" : "thumbnail", form->animation.frame,
	               frame_offset, form->animation.delay,
	               form == &image->image ? "image" : "thumbnail", image->image.path);
	return true;
}

bool nqiv_image_load_surface(nqiv_image* image, nqiv_image_form* form)
{
	assert(image != NULL);
	assert(form != NULL);
	assert(form->data != NULL);
	assert(form->effective_width > 0);
	assert(form->effective_height > 0);
	form->surface = SDL_CreateRGBSurfaceWithFormatFrom(
		form->data, form->effective_width, form->effective_height, 4 * 8, 4 * form->effective_width,
		SDL_PIXELFORMAT_ABGR8888);
	if(form->surface == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Failed to create SDL surface for form %s of %s (%s).",
		               form == &image->image ? "image" : "thumbnail", image->image.path,
		               SDL_GetError());
		form->error = true;
		return false;
	}
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Loaded surface of form %s of image %s\n",
	               form == &image->image ? "image" : "thumbnail", image->image.path);
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
	const int width_value = nqiv_strtoi(width_string, NULL, 10);
	if(width_value <= 0 || errno == ERANGE) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Invalid width for thumbnail of %s\n", image->image.path);
		return false;
	}
	const int height_value = nqiv_strtoi(height_string, NULL, 10);
	if(height_value <= 0 || errno == ERANGE) {
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

bool nqiv_image_is_form_loaded(const nqiv_image_form* form)
{
	return form->vips != NULL || form->data != NULL || form->surface != NULL
	       || form->texture != NULL;
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

	manager->thumbnail.size = 256;

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

void nqiv_image_calculate_zoom_dimension(const double least,
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

void nqiv_image_manager_zoom_in(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(
		fabs(manager->zoom.zoom_in_amount), false, manager->zoom.actual_size_level,
		manager->zoom.image_to_viewport_ratio_max, true, &manager->zoom.image_to_viewport_ratio,
		manager->zoom.zoom_in_amount);
}

void nqiv_image_manager_zoom_out(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(
		fabs(manager->zoom.zoom_in_amount), false, manager->zoom.actual_size_level,
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
		fabs(manager->zoom.zoom_in_amount), false, manager->zoom.actual_size_level,
		manager->zoom.image_to_viewport_ratio_max, true, &manager->zoom.image_to_viewport_ratio,
		manager->zoom.zoom_in_amount_more);
}

void nqiv_image_manager_zoom_out_more(nqiv_image_manager* manager)
{
	nqiv_image_calculate_zoom_dimension(
		fabs(manager->zoom.zoom_in_amount), false, manager->zoom.actual_size_level,
		manager->zoom.image_to_viewport_ratio_max, true, &manager->zoom.image_to_viewport_ratio,
		manager->zoom.zoom_out_amount_more);
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
    Any edges of the image that overflow the canvas rect will be clipped to the size of the canvas
 rect. Take them where they are clipped, and any non-overflowing edges, as is.

    To calculate the actual destination rect, divide the height and width of the canvas rect with
 the screen size. Use these to scale the dimensions of the source rect into the screen size.
 */
void nqiv_image_manager_calculate_zoomrect(nqiv_image_manager* manager,
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

	double canvas_rect_w;
	double canvas_rect_h;
	if(srcrect->w > srcrect->h) {
		const double screen_aspect = (double)dstrect->h / (double)dstrect->w;
		canvas_rect_w = (double)srcrect->w;
		canvas_rect_h = (double)srcrect->w * screen_aspect;
	} else {
		const double screen_aspect = (double)dstrect->w / (double)dstrect->h;
		canvas_rect_w = (double)srcrect->h * screen_aspect;
		canvas_rect_h = srcrect->h;
	}

	if(do_zoom) {
		canvas_rect_w *= manager->zoom.image_to_viewport_ratio;
		canvas_rect_h *= manager->zoom.image_to_viewport_ratio;
	}

	if((double)srcrect->w > canvas_rect_w) {
		const double diff = (double)srcrect->w - canvas_rect_w;
		srcrect->w -= (int)(diff);
		srcrect->x += (int)(diff / 2.0);
	}
	if((double)srcrect->h > canvas_rect_h) {
		const double diff = (double)srcrect->h - canvas_rect_h;
		srcrect->h -= (int)(diff);
		srcrect->y += (int)(diff / 2.0);
	}

	if(do_zoom) {
		srcrect->x += (int)((double)srcrect->x * manager->zoom.viewport_horizontal_shift);
		srcrect->y += (int)((double)srcrect->y * manager->zoom.viewport_vertical_shift);
	}

	if(!do_stretch) {
		const int    display_width = dstrect->w;
		const int    display_height = dstrect->h;
		const double canvas_dst_w_ratio = (double)dstrect->w / canvas_rect_w;
		const double canvas_dst_h_ratio = (double)dstrect->h / canvas_rect_h;
		const int    new_src_w = (int)((double)srcrect->w * canvas_dst_w_ratio);
		const int    new_src_h = (int)((double)srcrect->h * canvas_dst_h_ratio);
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
	}
	if(dstrect->x < 0) {
		dstrect->x = 0;
	}
	if(dstrect->y < 0) {
		dstrect->y = 0;
	}
	if(dstrect->w <= 0) {
		dstrect->w = 1;
	}
	if(dstrect->h <= 0) {
		dstrect->h = 1;
	}
}

void nqiv_image_manager_calculate_zoom_parameters(nqiv_image_manager* manager,
                                                  const bool          tight_fit,
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
	/* Basically guestimate fit level based on biggest side of image, makingit
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
			nqiv_image_manager_calculate_zoomrect(manager, tight_fit, false, &src, &dst);
			if(dst.w < dstrect->w && dst.h < dstrect->h) {
				current_ratio = manager->zoom.image_to_viewport_ratio;
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
	if(nqiv_thumbnail_get_closest_size(manager->thumbnail.size)
	   <= nqiv_thumbnail_get_closest_size(old_size)) {
		return true;
	}
	const int    num_images = nqiv_array_get_units_count(manager->images);
	nqiv_image** images = manager->images->data;
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
			if(images[idx]->thumbnail.vips != NULL || images[idx]->thumbnail.data != NULL
			   || images[idx]->thumbnail.surface != NULL) {
				nqiv_event event = {0};
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.transaction_group = -1;
				event.options.image_load.image = images[idx];
				event.options.image_load.thumbnail_options.unload = true;
				event.options.image_load.thumbnail_options.vips =
					images[idx]->thumbnail.vips != NULL;
				event.options.image_load.thumbnail_options.raw =
					images[idx]->thumbnail.data != NULL;
				event.options.image_load.thumbnail_options.surface =
					images[idx]->thumbnail.surface != NULL;
				if(!nqiv_priority_queue_push(manager->thread_queue,
				                             NQIV_EVENT_PRIORITY_REATTEMPT_THUMBNAIL, &event)) {
					nqiv_image_unlock(images[idx]);
					return false;
				}
			}
		}
		nqiv_image_unlock(images[idx]);
	}
	return true;
}

void nqiv_image_manager_increment_thumbnail_size_base(nqiv_image_manager* manager, const int adjust)
{
	manager->thumbnail.size += adjust;
}

void nqiv_image_manager_decrement_thumbnail_size_base(nqiv_image_manager* manager, const int adjust)
{
	manager->thumbnail.size -= adjust;
	if(manager->thumbnail.size <= 0) {
		manager->thumbnail.size = adjust;
	}
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

void nqiv_image_form_delay_frame(nqiv_image_form* form)
{
	const Uint64 frame_diff = SDL_GetTicks64() - form->animation.last_frame_time;
	if(frame_diff <= form->animation.delay) {
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
	form->animation.last_frame_time = clock();
	if(!nqiv_image_form_set_frame_delay(image, form)) {
		return false;
	}
	nqiv_image_form_delay_frame(form);
	return true;
}

bool nqiv_image_form_next_frame(nqiv_image* image, nqiv_image_form* form)
{
	assert(form != NULL);
	assert(form->vips != NULL);
	if(!form->animation.exists) {
		return true;
	}
	if(form->texture != NULL && !form->animation.frame_rendered) {
		return false;
	}
	form->animation.frame += 1;
	if(form->animation.frame >= form->animation.frame_count) {
		form->animation.frame = 0;
	}
	form->animation.frame_rendered = false;
	if(!nqiv_image_form_set_frame_delay(image, form)) {
		return false;
	}
	nqiv_image_form_delay_frame(form);
	return true;
}
