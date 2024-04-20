#ifndef NQIV_PRUNER_H
#define NQIV_PRUNER_H

#include <stdbool.h>

#include "logging.h"
#include "array.h"

/*
SUM OR AND
thumbnail_vips thumbnail_data thumbnail_surface thumbnail_texture vips data surface texture
loaded_ahead INTEGER loaded_behind INTEGER bytes_ahead INTEGER bytes_behind INTEGER self
UNLOAD
vips data surface texture
*/

typedef enum nqiv_pruner_count_op
{
	NQIV_PRUNER_COUNT_OP_UNKNOWN = 0,
	NQIV_PRUNER_COUNT_OP_SUM = 1,
	NQIV_PRUNER_COUNT_OP_OR = 2,
	NQIV_PRUNER_COUNT_OP_AND = 4,
} nqiv_pruner_count_op;

typedef union nqiv_pruner_desc_datapoint_content
{
	int as_int;
	int as_int_pair[2];
	bool as_bool;
} nqiv_pruner_desc_datapoint_content;

typedef struct nqiv_pruner_desc_datapoint
{
	bool active;
	nqiv_pruner_desc_datapoint_content condition;
	nqiv_pruner_desc_datapoint_content value;
} nqiv_pruner_desc_datapoint;

typedef struct nqiv_pruner_desc_dataset
{
	nqiv_pruner_desc_datapoint loaded_self;
	nqiv_pruner_desc_datapoint loaded_ahead;
	nqiv_pruner_desc_datapoint loaded_behind;
	nqiv_pruner_desc_datapoint bytes_ahead;
	nqiv_pruner_desc_datapoint bytes_behind;
} nqiv_pruner_desc_dataset;

typedef struct nqiv_pruner_state
{
	int idx;
	int selection;
	int montage_start;
	int montage_end;
	int total_sum;
	bool or_result;
	bool and_result;
} nqiv_pruner_state;

typedef struct nqiv_pruner_desc
{
	nqiv_pruner_count_op counter;
	nqiv_pruner_state state_check;
	nqiv_pruner_desc_dataset vips_set;
	nqiv_pruner_desc_dataset raw_set;
	nqiv_pruner_desc_dataset surface_set;
	nqiv_pruner_desc_dataset texture_set;
	nqiv_pruner_desc_dataset thumbnail_vips_set;
	nqiv_pruner_desc_dataset thumbnail_raw_set;
	nqiv_pruner_desc_dataset thumbnail_surface_set;
	nqiv_pruner_desc_dataset thumbnail_texture_set;
	bool unload_vips;
	bool unload_raw;
	bool unload_surface;
	bool unload_texture;
	bool unload_thumbnail_vips;
	bool unload_thumbnail_raw;
	bool unload_thumbnail_surface;
	bool unload_thumbnail_texture;
} nqiv_pruner_desc;

typedef struct nqiv_pruner
{
	nqiv_log_ctx* logger;
	nqiv_array* pruners;
	nqiv_pruner_state state;
} nqiv_pruner;

void nqiv_pruner_destroy(nqiv_pruner* pruner);
bool nqiv_pruner_init(nqiv_pruner* pruner, nqiv_log_ctx* logger, const int queue_length);
bool nqiv_pruner_run(nqiv_pruner* pruner, nqiv_montage_state* montage, nqiv_image_manager* images, nqiv_queue* thread_queue);
bool nqiv_pruner_append(nqiv_pruner* pruner, nqiv_pruner_desc* desc);
bool nqiv_pruner_create_desc(nqiv_log_ctx* logger, const char* text, nqiv_pruner_desc* desc);

#endif /* NQIV_PRUNER_H */
