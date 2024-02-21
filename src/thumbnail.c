#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>

#include <MagickCore/MagickCore.h>
#include <MagickWand/MagickWand.h>

#include "image.h"
#include "md5.h"
#include "thumbnail.h"
#include "platform.h"

void nqiv_thumbnail_digest_to_string(char* output, const unsigned char* md5raw)
{
	int idx;
	for(idx = 0; idx < 16; ++idx) {
	    sprintf(&output[idx*2], "%02x", (unsigned int)md5raw[idx]);
	}
}

/*file://
 */
#define NQIV_URI_LEN PATH_MAX + 7
bool nqiv_thumbnail_render_uri(nqiv_image* image, char* uri)
{
	const char* uristart = "file://";
	memset(uri, 0, NQIV_URI_LEN);
	memcpy( uri, uristart, strlen(uristart) );
	if(nqiv_realpath( image->image.path, uri + strlen(uristart) ) == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to calculate absolute path of %s.\n", image->image.path);
		return false;
	}
	return true;
}

void nqiv_thumbnail_get_type(nqiv_image_manager* images, const bool failed, char* typeseg, size_t* typelen_ptr)
{
	const char* typefail = "fail/";
	const char* typelarge = "large/";
	const char* typenormal = "normal/";

	const char* typeptr = NULL;
	size_t typelen = 0;
	if(failed) {
		typeptr = typefail;
		typelen = strlen(typefail);
	} else if(images->thumbnail.height >= 128 || images->thumbnail.width >= 128) {
		typeptr = typelarge;
		typelen = strlen(typelarge);
	} else {
		typeptr = typenormal;
		typelen = strlen(typenormal);
	}
	assert(typeptr != NULL);
	assert(typelen != 0);

	memcpy(typeseg, typeptr, typelen);
	if(typelen_ptr != NULL) {
		*typelen_ptr = typelen;
	}
}

bool nqiv_thumbnail_create_dirs(nqiv_image_manager* images, const bool failed)
{
	assert(images != NULL);
	assert(images->thumbnail.height > 0);
	assert(images->thumbnail.width > 0);
	assert(images->thumbnail.root != NULL);

	const char* thumbspart = "/thumbnails/";

	char fullpath[PATH_MAX] = {0};
	char* fullpath_ptr = fullpath;

	memcpy( fullpath_ptr, images->thumbnail.root, strlen(images->thumbnail.root) );
	fullpath_ptr += strlen(images->thumbnail.root);

	memcpy( fullpath_ptr, thumbspart, strlen(thumbspart) );
	if( !nqiv_mkdir(fullpath) ) {
		return false;
	}
	fullpath_ptr += strlen(thumbspart);

	nqiv_thumbnail_get_type(images, failed, fullpath_ptr, NULL);
	if( !nqiv_mkdir(fullpath) ) {
		return false;
	}
	return true;
}

bool nqiv_thumbnail_calculate_path(nqiv_image* image, char** pathptr_store, const bool failed)
{
	assert(image != NULL);
	assert(image->parent != NULL);
	assert(image->parent->thumbnail.height > 0);
	assert(image->parent->thumbnail.width > 0);
	assert(image->parent->thumbnail.root != NULL);

	const char* thumbspart = "/thumbnails/";
	const char* pngext = ".png";

	const size_t raw_rootlen = strlen(image->parent->thumbnail.root);
	assert(raw_rootlen >= 1);
	const size_t rootlen = image->parent->thumbnail.root[raw_rootlen - 1] == '/' ? raw_rootlen - 1 : raw_rootlen;

	const size_t thumblen = strlen(thumbspart);
	size_t typelen;
	char typeseg[PATH_MAX];
	nqiv_thumbnail_get_type(image->parent, failed, typeseg, &typelen);
	const size_t md5len = 32; /* Length of MD5 digest in text form */
	const size_t pnglen = strlen(pngext); /* Length of .png */

	MD5_CTX md5state;
	MD5_Init(&md5state);
	char actualpath[NQIV_URI_LEN];
	if( !nqiv_thumbnail_render_uri(image, actualpath) ) {
		return false;
	}

	size_t path_len = rootlen + thumblen + typelen + md5len + pnglen + 1;
	char* pathptr = calloc(1, path_len);
	if(pathptr == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to allocate memory for path data %s.\n", image->image.path);
		return false;
	}
	char* pathptr_base = pathptr;
	strncpy(pathptr, image->parent->thumbnail.root, rootlen);
	pathptr += rootlen;
	memcpy(pathptr, thumbspart, thumblen);
	pathptr += thumblen;
	memcpy(pathptr, typeseg, typelen);
	pathptr += typelen;
	/* strncpy(pathptr, typeptr, md5len); */
	MD5_Update( &md5state, actualpath, strlen(actualpath) );
	unsigned char md5raw[16];
	MD5_Final(md5raw, &md5state);
	nqiv_thumbnail_digest_to_string(pathptr, md5raw);
	pathptr += md5len;
	memcpy(pathptr, pngext, pnglen);
	*pathptr_store = pathptr_base;
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Calculated thumbnail path '%s' for image at path '%s'.\n", pathptr_base, image->image.path);
	return true;
}

/*18446744073709551615\0*/
#define NQIV_MTIME_STRLEN 21
/*18446744073709551615\0*/
#define NQIV_SIZE_STRLEN 21
/*2147483647\0*/
#define NQIV_DIMENSION_STRLEN 11
bool nqiv_thumbnail_create(nqiv_image* image)
{
	assert(image != NULL);
	assert(image->parent != NULL);
	assert(image->image.path != NULL);
	assert(image->image.file != NULL);
	assert(image->image.wand != NULL);
	assert(image->parent->thumbnail.height > 0);
	assert(image->parent->thumbnail.width > 0);
	/* assert(image->thumbnail.path == NULL); */

	if(image->thumbnail.path == NULL) {
		return false;
	}
	MagickWand* thumbnail_wand = CloneMagickWand(image->image.wand);
	if(thumbnail_wand == NULL) {
		nqiv_log_magick_wand_exception(image->parent->logger, image->image.wand, image->image.path);
		return false;
	}
	MagickResetIterator(thumbnail_wand);
	if( !MagickResizeImage(thumbnail_wand, image->parent->thumbnail.width, image->parent->thumbnail.height, image->parent->thumbnail.interpolation) ) {
		nqiv_log_magick_wand_exception(image->parent->logger, image->image.wand, image->image.path);
		DestroyMagickWand(thumbnail_wand); /* TODO: Where should this be? */
		return false;
	}
	char actualpath[NQIV_URI_LEN];
	if( !nqiv_thumbnail_render_uri(image, actualpath) ) {
		DestroyMagickWand(thumbnail_wand); /* TODO: Where should this be? */
		return false;
	}
	nqiv_stat_data stat_data;
	if( !nqiv_fstat(image->image.file, &stat_data) ) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to get stat data for image at %s.\n", image->image.path);
		DestroyMagickWand(thumbnail_wand); /* TODO: Where should this be? */
		return false;
	}
	char mtime_string[NQIV_MTIME_STRLEN] = {0};
	snprintf(mtime_string, NQIV_MTIME_STRLEN, "%" PRIuMAX, stat_data.mtime);
	char size_string[NQIV_SIZE_STRLEN] = {0};
	snprintf(size_string, NQIV_SIZE_STRLEN, "%zu", stat_data.size);
	char width_string[NQIV_DIMENSION_STRLEN] = {0};
	snprintf(width_string, NQIV_DIMENSION_STRLEN, "%d", image->image.width);
	char height_string[NQIV_DIMENSION_STRLEN] = {0};
	snprintf(height_string, NQIV_DIMENSION_STRLEN, "%d", image->image.height);
	if( !MagickSetImageProperty(thumbnail_wand, "Thumb::URI", actualpath) ||
		!MagickSetImageProperty(thumbnail_wand, "Thumb::MTime", mtime_string) ||
		!MagickSetImageProperty(thumbnail_wand, "Thumb::Size", size_string) ||
		!MagickSetImageProperty(thumbnail_wand, "Thumb::Image::Width", width_string) ||
		!MagickSetImageProperty(thumbnail_wand, "Thumb::Image::Height", height_string) ) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to get stat data for image at %s.\n", image->image.path);
		DestroyMagickWand(thumbnail_wand); /* TODO: Where should this be? */
		return false;
	}
	if( !nqiv_thumbnail_create_dirs(image->parent, false) ) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed create thumbnail dirs under %s.\n", image->parent->thumbnail.root);
		DestroyMagickWand(thumbnail_wand); /* TODO: Where should this be? */
		return false;
	}
	if( !MagickWriteImage(thumbnail_wand, image->thumbnail.path) ) {
		nqiv_log_magick_wand_exception(image->parent->logger, image->image.wand, image->image.path);
		DestroyMagickWand(thumbnail_wand); /* TODO: Where should this be? */
		return false;
	}
	DestroyMagickWand(thumbnail_wand); /* TODO: Where should this be? */
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Created thumbnail at path '%s' for image at path '%s'.\n", image->thumbnail.path, image->image.path);
	return true;
}
#undef NQIV_URI_LEN
#undef NQIV_MTIME_STRLEN
#undef NQIV_SIZE_STRLEN
#undef NQIV_DIMENSIONS_STRLEN

bool nqiv_thumbnail_matches_image(nqiv_image* image)
{
	assert(image != NULL);
	assert(image->parent != NULL);
	assert(image->image.file != NULL);
	assert(image->image.file != NULL);
	assert(image->image.wand != NULL);
	assert(image->thumbnail.path != NULL);
	assert(image->thumbnail.file != NULL);
	assert(image->thumbnail.wand != NULL);

	nqiv_stat_data stat_data;
	if( !nqiv_fstat(image->image.file, &stat_data) ) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to get stat data for image at %s.\n", image->image.path);
		return false;
	}

	char* thumbnail_mtime = MagickGetImageProperty(image->thumbnail.wand, "Thumb::MTime");
	if(thumbnail_mtime == NULL) {
		nqiv_log_magick_wand_exception(image->parent->logger, image->image.wand, image->image.path);
		return false;
	}
	const uintmax_t thumbnail_mtime_value = strtoumax(thumbnail_mtime, NULL, 10);
	if(thumbnail_mtime_value == 0 || errno == ERANGE) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_WARNING, "Invalid MTime for thumbnail of '%s' at '%s'.\n", image->image.path, image->thumbnail.path);
		return false;
	}
	MagickRelinquishMemory(thumbnail_mtime);

	const bool matches = thumbnail_mtime_value == (uintmax_t)(stat_data.mtime);

	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "'%s' %s '%s'.\n", image->image.path, matches ? "matches" : "does not match", image->thumbnail.path);
	return matches;
}
