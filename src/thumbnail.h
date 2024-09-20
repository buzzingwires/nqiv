#ifndef NQIV_THUMBNAIL_H
#define NQIV_THUMBNAIL_H

#include "image.h"

/*
 * Thumbnail management according to the Freedesktop Thumbnail Managing standard 0.9.0 at the time
 * of writing, as well as creation of thumbnail VIPS data for situations where a thumbnail is
 * needed, but the file won't actually be saved.
 */

/* Get corresponding file dimension to the given dimension. */
int nqiv_thumbnail_get_closest_size(const int size);

/* Calculate path of thumbnail for image, allocate memory for it, and store its pointer at the given
 * address. This allocated memory must be freed. If failed, will use the thumbnail failed path. */
bool nqiv_thumbnail_calculate_path(const nqiv_image* image,
                                   char**            pathptr_store,
                                   const bool        failed);

/* Create thumbnail VIPS data from image data, but don't save it. */
bool nqiv_thumbnail_create_vips(nqiv_image* image);
bool nqiv_thumbnail_create(nqiv_image* image);

/* Tell if an image's thumbnail is up to date with the image. */
bool nqiv_thumbnail_matches_image(nqiv_image* image);

#endif /* NQIV_THUMBNAIL_H */
