#ifndef NQIV_CMD_H
#define NQIV_CMD_H

#include <stdbool.h>

#include <SDL2/SDL.h>

#include "typedefs.h"
#include "array.h"
#include "logging.h"
#include "keybinds.h"
#include "keyrate.h"
#include "pruner.h"
#include "worker.h"

/* Max number of args for a cmd */
#define NQIV_CMD_MAX_ARGS 8
/* Used to build the full name of a command from nodes. */
#define NQIV_CMD_DUMPCFG_BUFFER_LENGTH 1024
/* Buffer length for unprocessed commands. */
#define NQIV_CMD_READ_BUFFER_LENGTH     10240
#define NQIV_CMD_READ_BUFFER_LENGTH_MAX 10240
/* Sane floating point values. */
#define NQIV_CMD_ARG_FLOAT_MIN 0.001
#define NQIV_CMD_ARG_FLOAT_MAX 100.0

typedef enum nqiv_cmd_arg_type
{
	NQIV_CMD_ARG_INT,
	NQIV_CMD_ARG_DOUBLE,
	NQIV_CMD_ARG_UINT64,
	NQIV_CMD_ARG_UINT8,
	NQIV_CMD_ARG_BOOL,
	NQIV_CMD_ARG_LOG_LEVEL,
	NQIV_CMD_ARG_PRESS_ACTION,
	NQIV_CMD_ARG_KEY_ACTION,
	NQIV_CMD_ARG_KEYBIND,
	NQIV_CMD_ARG_STRING,
	NQIV_CMD_ARG_PRUNER,
	NQIV_CMD_ARG_WORKER_SPEC,
} nqiv_cmd_arg_type;

typedef struct nqiv_cmd_arg_desc_setting_int
{
	int min;
	int max;
} nqiv_cmd_arg_desc_setting_int;

typedef struct nqiv_cmd_arg_desc_setting_Uint64
{
	Uint64 min;
	Uint64 max;
} nqiv_cmd_arg_desc_setting_Uint64;

typedef struct nqiv_cmd_arg_desc_setting_double
{
	double min;
	double max;
} nqiv_cmd_arg_desc_setting_double;

typedef struct nqiv_cmd_arg_desc_setting_string
{
	bool spaceless; /* String musn't have spaces. */
} nqiv_cmd_arg_desc_setting_string;

typedef struct nqiv_cmd_arg_desc_setting_key_action
{
	bool brief; /* Actually just for less-verbose printing. */
} nqiv_cmd_arg_desc_setting_key_action;

typedef union nqiv_cmd_arg_desc_setting
{
	nqiv_cmd_arg_desc_setting_int        of_int;
	nqiv_cmd_arg_desc_setting_Uint64     of_Uint64;
	nqiv_cmd_arg_desc_setting_double     of_double;
	nqiv_cmd_arg_desc_setting_string     of_string;
	nqiv_cmd_arg_desc_setting_key_action of_key_action;
} nqiv_cmd_arg_desc_setting;

typedef struct nqiv_cmd_arg_desc
{
	/* Type and properties of a particular argument. */
	nqiv_cmd_arg_desc_setting setting;
	nqiv_cmd_arg_type         type;
} nqiv_cmd_arg_desc;

typedef union nqiv_cmd_arg_value
{
	int                       as_int;
	double                    as_double;
	Uint64                    as_Uint64;
	Uint8                     as_Uint8;
	bool                      as_bool;
	nqiv_log_level            as_log_level;
	nqiv_keyrate_press_action as_press_action;
	nqiv_key_action           as_key_action;
	nqiv_keybind_pair         as_keybind;
	nqiv_pruner_desc          as_pruner;
	nqiv_worker_spec          as_worker_spec;
} nqiv_cmd_arg_value;

typedef struct nqiv_cmd_arg_token
{
	nqiv_cmd_arg_value value;
	char*              raw; /* Pointer to string containing unparsed arg, and its length */
	nqiv_cmd_arg_type  type;
	int                length;
} nqiv_cmd_arg_token;

typedef struct nqiv_cmd_node nqiv_cmd_node;

typedef struct nqiv_cmd_manager_print_settings
{
	/* Loose information needed for various tasks related to printing the
	 * command tree. */
	int            indent;
	bool           dumpcfg;
	char*          prefix;
	nqiv_cmd_node* current_node;
	bool           in_escape;    /* Are we currently waiting for an escape sequence? */
	bool           finished_cmd; /* Do we have a finished command for parsing? */
} nqiv_cmd_manager_print_settings;

struct nqiv_cmd_manager
{
	nqiv_state*                     state;
	nqiv_array*                     buffer;
	nqiv_cmd_manager_print_settings print_settings;
	nqiv_cmd_node*                  root_node;
};

struct nqiv_cmd_node
{
	char* name;
	char* description;
	/* Pointer to the data handled by this node. This may or may not be used, depending on whether
	 * the store/print functions are specialized. */
	void* data;
	bool (*store_value)(nqiv_cmd_manager*, nqiv_cmd_arg_token*);
	void (*print_value)(nqiv_cmd_manager*);
	nqiv_cmd_arg_desc** args;
	/* Child nodes are chosen from a linked list of peers. */
	nqiv_cmd_node*      peer;
	nqiv_cmd_node*      child;
	/* Deprecated commands are still usable, but will not appear unless directly
	 * referenced. A branch node's deprecated status will be propagated to its
	 * children. */
	bool                deprecated;
};

/* Send an SDL event to main to update based on config parsing. */
bool nqiv_cmd_alert_main(nqiv_cmd_manager* manager);

bool           nqiv_cmd_add_cmd_and_parse(nqiv_cmd_manager* manager, const char* str);
nqiv_op_result nqiv_cmd_add_stream_cmd(nqiv_cmd_manager* manager, FILE* stream);
bool           nqiv_cmd_parse(nqiv_cmd_manager* manager);
bool           nqiv_cmd_consume_stream(nqiv_cmd_manager* manager, FILE* stream);
bool           nqiv_cmd_consume_stream_from_path(nqiv_cmd_manager* manager, const char* path);
void           nqiv_cmd_manager_destroy(nqiv_cmd_manager* manager);
bool           nqiv_cmd_manager_init(nqiv_cmd_manager* manager, nqiv_state* state);

/* Just helper functions that see use elsewhere. */
int nqiv_cmd_scan_not_whitespace(const char* data, const int start, const int end);
int nqiv_cmd_scan_whitespace(const char* data, const int start, const int end);
#endif /* NQIV_CMD_H */
