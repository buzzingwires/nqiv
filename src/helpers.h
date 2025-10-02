#ifndef NQIV_HELPERS_H
#define NQIV_HELPERS_H

#define NQIV_MAX(v1, v2)          (((v2) > (v1)) ? (v2) : (v1))
#define NQIV_MIN(v1, v2)          (((v2) < (v1)) ? (v2) : (v1))
#define NQIV_CLAMP(v, min, max)   (((v) < (min)) ? (min) : (((v) > (max)) ? (max) : (v)))

#define NQIV_BOOLSTR(cond)        ((cond) ? "true" : "false")
#define NQIV_CBOOLSTR(cond)       ((cond) ? "TRUE" : "FALSE")

#define NQIV_SAYFORM(image, form) ((form) == &(image)->image ? "image" : "thumbnail")

#define NQIV_ASSIGNIF(var, cond, val) ((var) = (cond) ? (val) : (var));

#endif /* NQIV_HELPERS_H */
