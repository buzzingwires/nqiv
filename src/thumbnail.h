#ifndef NQIV_THUMBNAIL_H
#define NQIV_THUMBNAIL_H

#include "image.h"

/*
* move thumbnail to own file
* move md5 to own file if it's sufficiently complicated
calculate path based on root and size
see if path exists using relevant platform-specific functions
if thumbnail loading is enabled, load thumbnail from disk, otherwise resize existing image
if thumbnail saving is enabled and thumbnail doesn't exist, generate thumbnail from existing image
we need to copy, resize, write metadata, optimize the image, and finally save to disk
*/

int  nqiv_thumbnail_get_closest_size(const int size);

bool nqiv_thumbnail_calculate_path(const nqiv_image* image,
                                   char**            pathptr_store,
                                   const bool        failed);

bool nqiv_thumbnail_create_vips(nqiv_image* image);
bool nqiv_thumbnail_create(nqiv_image* image);

bool nqiv_thumbnail_matches_image(nqiv_image* image);

#endif /* NQIV_THUMBNAIL_H */
