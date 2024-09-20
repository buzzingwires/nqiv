#include "platform.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>
#include <limits.h>
#include <inttypes.h>
#include <errno.h>

#include <vips/vips.h>

#include "image.h"
#include "thumbnail.h"

int nqiv_thumbnail_get_closest_size(const int size)
{
	if(size <= 128) {
		return 128;
	} else if(size <= 256) {
		return 256;
	} else if(size <= 512) {
		return 512;
	} else {
		return 1024;
	}
}

#define NQIV_URI_LEN (PATH_MAX + 7)
bool nqiv_thumbnail_render_uri(const nqiv_image* image, char* uri)
{
	char abspath[PATH_MAX + 1] = {0};
	if(nqiv_realpath(image->image.path, abspath) == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR,
		               "Failed to calculate absolute path of %s\n", image->image.path);
		return false;
	}

	gchar* gcanon = g_canonicalize_filename(abspath, "/");
	if(gcanon == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to canonicalize path of %s\n",
		               image->image.path);
		return false;
	}

	gchar* guri = g_filename_to_uri(gcanon, NULL, NULL);
	g_free(gcanon);
	if(guri == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR,
		               "Failed to generate URI from path of %s\n", image->image.path);
		return false;
	}
	if(strlen(guri) > NQIV_URI_LEN) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR,
		               "Generated URI exceeds length of %d for path %s\n", NQIV_URI_LEN,
		               image->image.path);
		g_free(guri);
		return false;
	}

	memset(uri, 0, NQIV_URI_LEN);
	memcpy(uri, guri, strlen(guri));
	g_free(guri);

	return true;
}

bool nqiv_thumbnail_digest_to_builder(nqiv_array* builder, const nqiv_image* image)
{
	char actualpath[NQIV_URI_LEN + 1];
	if(!nqiv_thumbnail_render_uri(image, actualpath)) {
		return false;
	}
	gchar* md5string = g_compute_checksum_for_string(G_CHECKSUM_MD5, actualpath, -1);
	if(md5string == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Failed to compute checksum for thumbnail URI '%s' of %s\n", actualpath,
		               image->image.path);
		return false;
	}
	const bool result = nqiv_array_push_str(builder, md5string);
	g_free(md5string);
	return result;
}

bool nqiv_thumbnail_get_type(const nqiv_image_manager* images,
                             const bool                failed,
                             nqiv_array*               builder)
{
	if(failed) {
		return nqiv_array_push_str(builder, "fail/");
	} else if(images->thumbnail.size <= 128) {
		return nqiv_array_push_str(builder, "normal/");
	} else if(images->thumbnail.size <= 256) {
		return nqiv_array_push_str(builder, "large/");
	} else if(images->thumbnail.size <= 512) {
		return nqiv_array_push_str(builder, "x-large/");
	} else {
		return nqiv_array_push_str(builder, "xx-large/");
	}
}

bool nqiv_thumbnail_create_dirs(nqiv_image_manager* images, const bool failed)
{
	assert(images != NULL);
	assert(images->thumbnail.size > 0);
	assert(images->thumbnail.root != NULL);

	nqiv_array builder;
	char       fullpath[PATH_MAX + 1] = {0};
	nqiv_array_inherit(&builder, fullpath, sizeof(char), PATH_MAX);

	bool result = true;
	result = result && nqiv_array_push_str(&builder, images->thumbnail.root);
	result = result && nqiv_array_push_str(&builder, "/thumbnails/");
	if(!result || !nqiv_mkdir(fullpath)) {
		return false;
	}
	result = result && nqiv_thumbnail_get_type(images, failed, &builder);
	if(!result || !nqiv_mkdir(fullpath)) {
		return false;
	}
	return true;
}

bool nqiv_thumbnail_calculate_path(const nqiv_image* image, char** pathptr_store, const bool failed)
{
	assert(image != NULL);
	assert(image->parent != NULL);
	assert(image->parent->thumbnail.size > 0);
	assert(image->parent->thumbnail.root != NULL);

	nqiv_array builder;
	/* XXX: We may or may not make a .tmp file with this path, so there should be room for that. */
	char       fullpath[PATH_MAX - 4 + 1] = {0};
	nqiv_array_inherit(&builder, fullpath, sizeof(char), PATH_MAX);

	const int raw_rootlen = nqiv_strlen(image->parent->thumbnail.root);
	assert(raw_rootlen >= 1);
	const int rootlen =
		image->parent->thumbnail.root[raw_rootlen - 1] == '/' ? raw_rootlen - 1 : raw_rootlen;
	if(strncmp(image->image.path, image->parent->thumbnail.root, rootlen) == 0) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Image path '%s' matches thumbnail path starting at '%s'. Avoiding "
		               "recreating thumbnail.\n",
		               image->image.path, image->parent->thumbnail.root);
		return false;
	}

	bool result = true;
	result = result && nqiv_array_push_str_count(&builder, image->parent->thumbnail.root, rootlen);
	result = result && nqiv_array_push_str(&builder, "/thumbnails/");
	result = result && nqiv_thumbnail_get_type(image->parent, failed, &builder);
	result = result && nqiv_thumbnail_digest_to_builder(&builder, image);
	result = result && nqiv_array_push_str(&builder, ".png");
	if(!result) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR,
		               "Thumbnail name string too long for image %s\n", image->image.path);
		return false;
	}

	char* pathptr = calloc(builder.unit_length, nqiv_array_get_units_count(&builder) + 1);
	if(pathptr == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR,
		               "Failed to allocate memory for path data %s\n", image->image.path);
		return false;
	}
	memcpy(pathptr, fullpath, nqiv_array_get_units_count(&builder));

	*pathptr_store = pathptr;
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG,
	               "Calculated thumbnail path '%s' for image at path '%s'.\n", pathptr,
	               image->image.path);
	return true;
}

/*18446744073709551615\0*/
#define NQIV_MTIME_STRLEN 21
/*18446744073709551615\0*/
#define NQIV_SIZE_STRLEN 21
/*2147483647\0*/
#define NQIV_DIMENSION_STRLEN 11

bool nqiv_thumbnail_create_vips(nqiv_image* image)
{
	assert(image != NULL);
	assert(image->image.path != NULL);
	assert(image->image.vips != NULL);
	assert(image->thumbnail.vips == NULL);
	assert(image->parent->thumbnail.size > 0);

	VipsImage* old_vips;
	VipsImage* thumbnail_vips;
	if(vips_copy(image->image.vips, &thumbnail_vips, NULL) == -1) {
		nqiv_log_vips_exception(image->parent->logger, image, &image->image);
		return false;
	}
	old_vips = thumbnail_vips;
	if(vips_crop(old_vips, &thumbnail_vips, 0, 0, image->image.width, image->image.height, NULL)
	   == -1) {
		g_object_unref(old_vips);
		nqiv_log_vips_exception(image->parent->logger, image, &image->image);
		return false;
	}
	g_object_unref(old_vips);

	image->thumbnail.animation.frame = 0;
	old_vips = thumbnail_vips;
	if(vips_thumbnail_image(old_vips, &thumbnail_vips,
	                        nqiv_thumbnail_get_closest_size(image->parent->thumbnail.size), NULL)
	   == -1) {
		g_object_unref(old_vips);
		nqiv_log_vips_exception(image->parent->logger, image, &image->image);
		return false;
	}
	g_object_unref(old_vips);
	image->thumbnail.vips = thumbnail_vips;

	image->thumbnail.width = vips_image_get_width(image->thumbnail.vips);
	image->thumbnail.height = vips_image_get_height(image->thumbnail.vips);
	image->thumbnail.srcrect.x = 0;
	image->thumbnail.srcrect.y = 0;
	image->thumbnail.srcrect.w = image->thumbnail.width;
	image->thumbnail.srcrect.h = image->thumbnail.height;
	image->thumbnail.animation.frame_count = 1;
	image->thumbnail.animation.frame = 0;
	image->thumbnail.animation.exists = false;

	return true;
}

bool nqiv_thumbnail_create(nqiv_image* image)
{
	assert(image != NULL);
	assert(image->parent != NULL);

	if(image->thumbnail.path == NULL) {
		return false;
	}
	if(!nqiv_thumbnail_create_vips(image)) {
		return false;
	}
	char actualpath[NQIV_URI_LEN + 1];
	if(!nqiv_thumbnail_render_uri(image, actualpath)) {
		return false;
	}
	nqiv_stat_data stat_data;
	if(!nqiv_stat(image->image.path, &stat_data)) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR,
		               "Failed to get stat data for image at %s\n", image->image.path);
		return false;
	}
	char mtime_string[NQIV_MTIME_STRLEN + 1] = {0};
	snprintf(mtime_string, NQIV_MTIME_STRLEN, "%" PRIuMAX, (uintmax_t)stat_data.mtime);
	char size_string[NQIV_SIZE_STRLEN + 1] = {0};
	snprintf(size_string, NQIV_SIZE_STRLEN, "%zu", stat_data.size);
	char width_string[NQIV_DIMENSION_STRLEN + 1] = {0};
	snprintf(width_string, NQIV_DIMENSION_STRLEN, "%d", image->image.width);
	char height_string[NQIV_DIMENSION_STRLEN + 1] = {0};
	snprintf(height_string, NQIV_DIMENSION_STRLEN, "%d", image->image.height);

	vips_image_set_string(image->thumbnail.vips, "png-comment-0-Thumb::URI", actualpath);
	vips_image_set_string(image->thumbnail.vips, "png-comment-1-Thumb::MTime", mtime_string);
	vips_image_set_string(image->thumbnail.vips, "png-comment-2-Thumb::Size", size_string);
	vips_image_set_string(image->thumbnail.vips, "png-comment-3-Thumb::Image::Width", width_string);
	vips_image_set_string(image->thumbnail.vips, "png-comment-4-Thumb::Image::Height",
	                      height_string);

	if(!nqiv_thumbnail_create_dirs(image->parent, false)) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR,
		               "Failed create thumbnail dirs under %s\n", image->parent->thumbnail.root);
		return false;
	}
	char         tmppath[PATH_MAX + 1] = {0};
	const size_t pathlen = strlen(image->thumbnail.path);
	assert(pathlen <= PATH_MAX - 4 + 1); /* XXX: Room for .tmp */
	memcpy(tmppath, image->thumbnail.path, pathlen);
	memcpy(tmppath + pathlen, ".tmp", nqiv_strlen(".tmp"));
	assert(strlen(tmppath) == pathlen + 4);
	if(vips_pngsave(image->thumbnail.vips, tmppath, NULL) == -1) {
		nqiv_log_vips_exception(image->parent->logger, image, &image->image);
		return false;
	}
	if(!nqiv_chmod(tmppath, 0600)) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR,
		               "Failed to set permissions for thumbnail path '%s' of image at path '%s'.\n",
		               image->thumbnail.path, image->image.path);
		return false;
	}
	if(g_rename(tmppath, image->thumbnail.path) == -1) {
		if(g_access(image->thumbnail.path, R_OK | W_OK) != -1) {
			nqiv_log_write(image->parent->logger, NQIV_LOG_INFO,
			               "Thumbnail already exists (concurrent creation) for"
			               "'%s' of image at path '%s'.\n",
			               image->thumbnail.path, image->image.path);
			return true;
		}
		if(g_access(image->thumbnail.path, F_OK) != -1) {
			nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
			               "Thumbnail already exists (concurrent creation) but does not have "
			               "read/write permissions for"
			               "'%s' of image at path '%s'.\n",
			               image->thumbnail.path, image->image.path);
		} else {
			nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
			               "Failed to save '%s' of image at path '%s'.\n", image->thumbnail.path,
			               image->image.path);
		}
		return false;
	}
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG,
	               "Created thumbnail at path '%s' for image at path '%s'.\n",
	               image->thumbnail.path, image->image.path);
	return true;
}
#undef NQIV_URI_LEN
#undef NQIV_MTIME_STRLEN
#undef NQIV_SIZE_STRLEN
#undef NQIV_DIMENSIONS_STRLEN

uintmax_t nqiv_thumbnail_get_field(bool*             output,
                                   const nqiv_image* image,
                                   gchar**           header_field_names,
                                   const char*       field)
{
	const int string_idx = nqiv_lookup_vips_png_comment(header_field_names, field);
	if(string_idx == -1) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Failed to lookup %s metadata from thumbnail for %s\n", field,
		               image->image.path);
		*output = false;
		return 0;
	}

	const char* field_string;
	if(vips_image_get_string(image->thumbnail.vips, header_field_names[string_idx], &field_string)
	   == -1) {
		nqiv_log_vips_exception(image->parent->logger, image, &image->image);
		*output = false;
		return 0;
	}

	const uintmax_t thumbnail_value = strtoumax(field_string, NULL, 10);
	if(thumbnail_value == 0 || errno == ERANGE) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING,
		               "Invalid '%s' for thumbnail of '%s' at '%s'.\n", field, image->image.path,
		               image->thumbnail.path);
		*output = false;
		return 0;
	}
	*output = true;
	return thumbnail_value;
}

bool nqiv_thumbnail_matches_image(nqiv_image* image)
{
	assert(image != NULL);
	assert(image->parent != NULL);
	assert(image->image.vips != NULL);
	assert(image->thumbnail.path != NULL);
	assert(image->thumbnail.vips != NULL);

	nqiv_stat_data stat_data;
	if(!nqiv_stat(image->image.path, &stat_data)) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR,
		               "Failed to get stat data for image at %s\n", image->image.path);
		return false;
	}
	gchar** header_field_names = vips_image_get_fields(image->thumbnail.vips);
	if(header_field_names == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG,
		               "Failed to get vips header field names for image %s\n", image->image.path);
		return false;
	}

	bool            result = false;
	const uintmax_t thumbnail_mtime_value =
		nqiv_thumbnail_get_field(&result, image, header_field_names, "Thumb::MTime");
	if(!result) {
		g_strfreev(header_field_names);
		return false;
	}
	const uintmax_t thumbnail_size_value =
		nqiv_thumbnail_get_field(&result, image, header_field_names, "Thumb::Size");
	if(!result) {
		g_strfreev(header_field_names);
		return false;
	}
	g_strfreev(header_field_names);

	const bool matches = thumbnail_mtime_value == (uintmax_t)(stat_data.mtime)
	                     && thumbnail_size_value == (uintmax_t)(stat_data.size);

	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "'%s' %s '%s'.\n", image->image.path,
	               matches ? "matches" : "does not match", image->thumbnail.path);
	return matches;
}
