#ifndef NQIV_THUMBNAIL_H
#define NQIV_THUMBNAIL_H

#include "image.h"

int  nqiv_thumbnail_get_closest_size(const int size);

bool nqiv_thumbnail_calculate_path(const nqiv_image* image,
                                   char**            pathptr_store,
                                   const bool        failed);

bool nqiv_thumbnail_create_vips(nqiv_image* image);
bool nqiv_thumbnail_create(nqiv_image* image);

bool nqiv_thumbnail_matches_image(nqiv_image* image);

#endif /* NQIV_THUMBNAIL_H */
