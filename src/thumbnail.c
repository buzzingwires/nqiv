#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <limits.h>

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
	if(nqiv_realpath( image->image.path, actualpath + strlen(uristart) ) == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to calculate absolute path of %s.", image->image.path);
		return false;
	}

	size_t path_len = rootlen + thumblen + typelen + md5len + pnglen + 1;
	char* pathptr = calloc(1, path_len);
	if(pathptr == NULL) {
		nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to allocate memory for path data %s.", image->image.path);
		return false;
	}
	char* pathptr_base = pathptr;
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
	*pathptr_store = pathptr_base;
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Calculated thumbnail path '%s' for image at path '%s'.\n", pathptr_base, image->image.path);
	return true;
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
	if( !MagickWriteImage(thumbnail_wand, image->thumbnail.path) ) {
		nqiv_log_magick_wand_exception(image->parent->logger, image->image.wand, image->image.path);
		DestroyMagickWand(thumbnail_wand); /* TODO: Where should this be? */
		return false;
	}
	DestroyMagickWand(thumbnail_wand); /* TODO: Where should this be? */
	nqiv_log_write(image->parent->logger, NQIV_LOG_DEBUG, "Created thumbnail at path '%s' for image at path '%s'.\n", image->thumbnail.path, image->image.path);
	return true;
}
