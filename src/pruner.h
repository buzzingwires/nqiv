#ifndef NQIV_PRUNER_H
#define NQIV_PRUNER_H

#include <stdbool.h>
#include <stdint.h>

#include "logging.h"
#include "array.h"
#include "queue.h"
#include "montage.h"

#define NQIV_PRUNER_DESC_STRLEN 2048 /* Notation for pruner should not be longer than this. */

/* These can be checked together. */
typedef enum nqiv_pruner_count_op
{
	NQIV_PRUNER_COUNT_OP_UNKNOWN = 0,
	/* For operations that work with some kind of numerical value passing a
	 * threshold (*_ahead, *_behind), add the value past that threshold and
	 * check against a given sum. */
	NQIV_PRUNER_COUNT_OP_SUM = 1,
	/* OR the result of checks. */
	NQIV_PRUNER_COUNT_OP_OR = 2,
	/* AND the result of checks. */
	NQIV_PRUNER_COUNT_OP_AND = 4,
} nqiv_pruner_count_op;

typedef union nqiv_pruner_desc_datapoint_content
{
	int  as_int;
	int  as_int_pair[2];
	bool as_bool;
} nqiv_pruner_desc_datapoint_content;

typedef struct nqiv_pruner_desc_datapoint
{
	bool                               active;
	nqiv_pruner_desc_datapoint_content condition;
	nqiv_pruner_desc_datapoint_content value;
} nqiv_pruner_desc_datapoint;

typedef struct nqiv_pruner_desc_dataset
{
	/* Is this image itself animated. Does not check a data form. Boolean. */
	nqiv_pruner_desc_datapoint not_animated;
	/* Is the form loaded? Boolean. */
	nqiv_pruner_desc_datapoint loaded_self;
	/* Numerical. Int pair. Is the form loaded past a certain image index? If
	 * so, how many are permitted? If this is passed, also produce a
	 * booleanresult. */
	nqiv_pruner_desc_datapoint loaded_ahead;
	/* Ditto but behind. */
	nqiv_pruner_desc_datapoint loaded_behind;
	/* Ditto, but use rough size approximation H * W * 4 (sizeof RGBA pixel in
	 * bytes) */
	nqiv_pruner_desc_datapoint bytes_ahead;
	nqiv_pruner_desc_datapoint bytes_behind;
} nqiv_pruner_desc_dataset;

typedef struct nqiv_pruner_state
{
	/* Current image index. */
	int  idx;
	int  montage_start;
	int  montage_end;
	int  total_sum;
	bool or_result;
	bool and_result;
	/* Used to make sure we start with the and_result being true, before ANDing
	 * it with an actual check result. */
	bool and_is_set;
} nqiv_pruner_state;

typedef struct nqiv_pruner_desc
{
	nqiv_pruner_count_op     counter;
	nqiv_pruner_state        state_check; /* Compare against this state. */
	nqiv_pruner_desc_dataset vips_set;
	nqiv_pruner_desc_dataset surface_set;
	nqiv_pruner_desc_dataset texture_set;
	nqiv_pruner_desc_dataset thumbnail_vips_set;
	nqiv_pruner_desc_dataset thumbnail_surface_set;
	nqiv_pruner_desc_dataset thumbnail_texture_set;
	bool                     unload_vips;
	bool                     unload_surface;
	bool                     unload_texture;
	bool                     unload_thumbnail_vips;
	bool                     unload_thumbnail_surface;
	bool                     unload_thumbnail_texture;
	bool                     unload_vips_soft;
	bool                     unload_surface_soft;
	bool                     unload_thumbnail_vips_soft;
	bool                     unload_thumbnail_surface_soft;
} nqiv_pruner_desc;

typedef struct nqiv_pruner
{
	nqiv_state*     parent;
	nqiv_array*       pruners;
	nqiv_pruner_state state;
	int               thread_event_transaction_group;
} nqiv_pruner;

void nqiv_pruner_destroy(nqiv_pruner* pruner);
bool nqiv_pruner_init(nqiv_pruner* pruner, nqiv_state* parent, const int queue_length);
int  nqiv_pruner_run(nqiv_pruner*         pruner);
bool nqiv_pruner_append(nqiv_pruner* pruner, const nqiv_pruner_desc* desc);
bool nqiv_pruner_create_desc(nqiv_log_ctx* logger, const char* text, nqiv_pruner_desc* desc);
bool nqiv_pruner_desc_to_string(const nqiv_pruner_desc* desc, char* buf);

bool nqiv_pruner_desc_dataset_compare(const nqiv_pruner_desc_dataset* first,
                                      const nqiv_pruner_desc_dataset* second);
bool nqiv_pruner_desc_compare(const nqiv_pruner_desc* first, const nqiv_pruner_desc* second);

#endif /* NQIV_PRUNER_H */
