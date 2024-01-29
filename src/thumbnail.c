#include "thumbnail.h"
#include "image.h"
#include "md5.h"

bool nqiv_thumbnail_calculate_path(nqiv_image* image, char** pathptr, const bool failed)
{
	assert(image != NULL);
	assert(image->parent != NULL);
	assert(image->parent.root != NULL);
	assert(image->parent.thumbnail.height > 0);
	assert(image->parent.thumbnail.width > 0);

	const char* thumbspart = "/thumbnails/";
	const char* typefail = "fail/";
	const char* typelarge = "large/";
	const char* typenormal = "normal/";
	const char* pngext = ".png";

	const size_t rootlen = strlen(image->parent.root);
	const size_t thumblen += strlen(thumbspart);
	size_t typelen;
	char* typeptr;
	if(failed) {
		typeptr = typefail;
		typelen = strlen(typefail)
	} else if(image->parent.thumbnail.height >= 256 || image->parent.thumbnail.width >= 256) {
		typeptr = typelarge;
		typelen = strlen(large);
	} else {
		typeptr = typenormal;
		typelen = strlen(normal);
	}
	const size_t md5len = 32; /* Length of MD5 digest */
	const size_t pnglen = strlen(pngext); /* Length of .png */

	MD5_CTX md5state;
	MD5_Init(&md5state);
	const char* uristart = "file://";
	char actualpath[PATH_MAX + strlen(uristart)];
	memset( actualpath, 0, PATH_MAX + strlen(uristart) );
	strncpy( actualpath, uristart, strlen(uristart) );
	if(realpath( image->image.path, actualpath + strlen(uristart) ) == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to calculate absolute path of %s.", image->image.path);
		return false;
	}

	size_t path_len = rootlen + thumblen + typelen + md5len + pnglen;
	pathptr = calloc(1, path_len);
	if(pathptr == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to allocate memory for path data %s.", path);
		return false;
	}
	strncpy(pathptr, image->parent.root, rootlen);
	pathptr += rootlen;
	strncpy(pathptr, thumbspart, thumblen);
	pathptr += thumblen;
	strncpy(pathptr, typeptr, typelen);
	pathptr += typelen;
	/* strncpy(pathptr, typeptr, md5len); */
	MD5_Update( &md5state, actualpath, strlen(actualpath) );
	MD5_Final(pathptr, &md5state);
	pathptr += md5len;
	strncpy(pathptr, pngext, pnglen);
	return true;
}

bool nqiv_log_magick_exception(const char* path, ExceptionInfo* exception)
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
	assert(image->parent.thumbnail.height > 0);
	assert(image->parent.thumbnail.width > 0);
	/* assert(image->thumbnail.path == NULL); */

	char* pathptr = NULL;
	if(image->thumbnail.path == NULL) {
		if( !nqiv_thumbnail_calculate_path(image, &pathptr, false) ) {
			nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to create thumbnail path for %s", image->image.path);
			return false;
		}
		assert(pathptr != NULL);
	}
	Image* image_object = GetImageFromMagickWand(image->image.wand); /* TODO NEED TO FREE */
	if(image_object == NULL) {
		free(pathptr);
		nqiv_log_magick_wand_exception(image->parent->logger, image->image.wand, image->image.path);
		return false;
	}
	ExceptionInfo* exception_info = AcquireExceptionInfo(); /* TODO LOG ME AND FREE WHEN NEEDED WHAT NEEDS TO BE FREED */
	if(exception_info == NULL) {
		free(pathptr);
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to create exception info for new thumbnail of path %s", image->image.path);
		return false;
	}
	image_object = CloneImage(image_object, 0, 0, 0, exception_info);
	if( CatchException(exception_info) ) {
		nqiv_log_magick_exception(image->image.path, exception_info);
		free(pathptr);
		DestroyExceptionInfo(exception_info);
		return false;
	}
	ImageInfo* image_info = CloneImageInfo(NULL);
	if(image_info == NULL) {
		free(pathptr);
		DestroyExceptionInfo(exception_info);
		DestroyImage(image_object);
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to create image info for new thumbnail of path %s", image->image.path);
		return false;
	}
	strcpy(image_info->filename, pathptr);
	ResizeImage(image, image->parent.thumbnail.width, image->parent.thumbnail.height, image->parent.interpolation, exception_info);
	if( CatchException(exception_info) ) {
		nqiv_log_magick_exception(image->image.path, exception_info);
		free(pathptr);
		DestroyExceptionInfo(exception_info);
		DestroyImage(image_object);
		DestroyImageInfo(image_info);
		return false;
	}
	WriteImage(image_info, image_object, exception_info);
	if( CatchException(exception_info) ) {
		nqiv_log_magick_exception(image->image.path, exception_info);
		free(pathptr);
		DestroyExceptionInfo(exception_info);
		DestroyImage(image_object);
		DestroyImageInfo(image_info);
		return false;
	}
	image->thumbnail.path = pathptr;
	DestroyExceptionInfo(exception_info);
	DestroyImage(image_object);
	DestroyImageInfo(image_info);
	return true;
}
