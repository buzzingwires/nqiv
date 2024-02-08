#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <limits.h>

#include "image.h"
#include "md5.h"
#include "thumbnail.h"

void nqiv_thumbnail_digest_to_string(char* output, const unsigned char* md5raw)
{
	int idx;
	for(idx = 0; idx < 16; ++idx) {
	    sprintf(&output[idx*2], "%02x", (unsigned int)md5raw[idx]);
	}
}

bool nqiv_thumbnail_calculate_path(nqiv_image* image, char** pathptr_store, const bool failed)
{
	assert(image != NULL);
	assert(image->parent != NULL);
	assert(image->parent->thumbnail.height > 0);
	assert(image->parent->thumbnail.width > 0);

	const char* thumbspart = "/thumbnails/";
	const char* typefail = "fail/";
	const char* typelarge = "large/";
	const char* typenormal = "normal/";
	const char* pngext = ".png";

	const size_t rootlen = strlen(image->parent->thumbnail.root);
	const size_t thumblen = strlen(thumbspart);
	size_t typelen;
	const char* typeptr;
	if(failed) {
		typeptr = typefail;
		typelen = strlen(typefail);
	} else if(image->parent->thumbnail.height >= 256 || image->parent->thumbnail.width >= 256) {
		typeptr = typelarge;
		typelen = strlen(typelarge);
	} else {
		typeptr = typenormal;
		typelen = strlen(typenormal);
	}
	const size_t md5len = 32; /* Length of MD5 digest in text form */
	const size_t pnglen = strlen(pngext); /* Length of .png */

	MD5_CTX md5state;
	MD5_Init(&md5state);
	const char* uristart = "file://";
	char actualpath[PATH_MAX + strlen(uristart)];
	memset( actualpath, 0, PATH_MAX + strlen(uristart) );
	memcpy( actualpath, uristart, strlen(uristart) );
	if(realpath( image->image.path, actualpath + strlen(uristart) ) == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to calculate absolute path of %s.", image->image.path);
		return false;
	}

	size_t path_len = rootlen + thumblen + typelen + md5len + pnglen;
	char* pathptr = calloc(1, path_len);
	if(pathptr == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to allocate memory for path data %s.", image->image.path);
		return false;
	}
	strncpy(pathptr, image->parent->thumbnail.root, rootlen);
	pathptr += rootlen;
	memcpy(pathptr, thumbspart, thumblen);
	pathptr += thumblen;
	memcpy(pathptr, typeptr, typelen);
	pathptr += typelen;
	/* strncpy(pathptr, typeptr, md5len); */
	MD5_Update( &md5state, actualpath, strlen(actualpath) );
	unsigned char md5raw[16];
	MD5_Final(md5raw, &md5state);
	nqiv_thumbnail_digest_to_string(pathptr, md5raw);
	pathptr += md5len;
	memcpy(pathptr, pngext, pnglen);
	*pathptr_store = pathptr;
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Calculated thumbnail path '%s' for image at path '%s'.\n", pathptr, image->image.path);
	return true;
}

void nqiv_log_magick_exception(nqiv_log_ctx* logger, const char* path, ExceptionInfo* exception)
{
	char* description = GetExceptionMessage(exception->error_number);
	nqiv_log_write(logger, NQIV_LOG_ERROR, "ImageMagick exception for path %s: %s %s\n", path, GetMagickModule(), description);
	MagickRelinquishMemory(description);
}

bool nqiv_thumbnail_create(nqiv_image* image)
{
	assert(image != NULL);
	assert(image->parent != NULL);
	assert(image->image.path != NULL);
	assert(image->image.wand != NULL);
	assert(image->parent->thumbnail.height > 0);
	assert(image->parent->thumbnail.width > 0);
	/* assert(image->thumbnail.path == NULL); */

	if(image->thumbnail.path == NULL) {
		return false;
	}
	Image* image_object = GetImageFromMagickWand(image->image.wand); /* TODO NEED TO FREE */
	if(image_object == NULL) {
		nqiv_log_magick_wand_exception(image->parent->logger, image->image.wand, image->image.path);
		return false;
	}
	ExceptionInfo* exception_info = AcquireExceptionInfo(); /* TODO LOG ME AND FREE WHEN NEEDED WHAT NEEDS TO BE FREED */
	if(exception_info == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to create exception info for new thumbnail of path %s", image->image.path);
		return false;
	}
	image_object = CloneImage(image_object, 0, 0, 0, exception_info);
	CatchException(exception_info);
	if(exception_info->error_number != 0) {
		nqiv_log_magick_exception(image->parent->logger, image->image.path, exception_info);
		DestroyExceptionInfo(exception_info);
		return false;
	}
	ImageInfo* image_info = CloneImageInfo(NULL);
	if(image_info == NULL) {
		DestroyExceptionInfo(exception_info);
		DestroyImage(image_object);
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to create image info for new thumbnail of path %s", image->image.path);
		return false;
	}
	strcpy(image_info->filename, image->thumbnail.path);
	InterpolativeResizeImage(image_object, image->parent->thumbnail.width, image->parent->thumbnail.height, image->parent->thumbnail.interpolation, exception_info);
	CatchException(exception_info);
	if(exception_info->error_number != 0) {
		nqiv_log_magick_exception(image->parent->logger, image->image.path, exception_info);
		DestroyExceptionInfo(exception_info);
		DestroyImage(image_object);
		DestroyImageInfo(image_info);
		return false;
	}
	WriteImage(image_info, image_object, exception_info);
	CatchException(exception_info);
	if(exception_info->error_number != 0) {
		nqiv_log_magick_exception(image->parent->logger, image->image.path, exception_info);
		DestroyExceptionInfo(exception_info);
		DestroyImage(image_object);
		DestroyImageInfo(image_info);
		return false;
	}
	DestroyExceptionInfo(exception_info);
	DestroyImage(image_object);
	DestroyImageInfo(image_info);
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Created thumbnail at path '%s' for image at path '%s'.\n", image->thumbnail.path, image->image.path);
	return true;
}
