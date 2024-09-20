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

/*
 * The nqiv_cmd_manager is the primary means by which configuration directives
 * and command line arguments are passed to nqiv.
 *
 * It works by representing a parse tree with branch nodes and leaf nodes.
 *
 * Branch nodes may have other branch nodes or leaf nodes as children. Leaf
 * nodes point to functions that allow their relevant values to be stored or
 * printed and have a list of arguments that are handled by their store
 * function. They are expected to have at least a store function, though a print
 * function is optional, as some commands only perform an action, rather than
 * storing a value.
 *
 * Commands are case-sensitive and terminate at the end of a line. Each 'node'
 * is traversed based on a space-separated name. A line may begin with # to make
 * it a comment, where it will be passed over by the parser. Help may be printed
 * for any node, its children, or recursively, or a functional list of
 * documented commands may be printed. The intent of this system is to be
 * self-documenting.
 *
 * Command example:
 *
 * set      color  background 0             0            0            255 BRANCH
 * BRANCH LEAF       UINT8(0-255)  UINT8(0-255) UINT8(0-255) UINT8(0-255)
 */

/* Max number of args for a cmd */
#define NQIV_CMD_MAX_ARGS 8
/* Used to build the full name of a command from nodes. */
#define NQIV_CMD_DUMPCFG_BUFFER_LENGTH 1024
/* Buffer length for an individual char */
#define NQIV_CMD_ADD_BYTE_BUFFER_LENGTH (sizeof(char) * 1)
/* Buffer length for unprocessed commands. */
#define NQIV_CMD_READ_BUFFER_LENGTH     131072
#define NQIV_CMD_READ_BUFFER_LENGTH_MAX 1048576
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
	nqiv_cmd_arg_type         type;
	nqiv_cmd_arg_desc_setting setting;
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
} nqiv_cmd_arg_value;

typedef struct nqiv_cmd_arg_token
{
	nqiv_cmd_arg_type  type;
	char*              raw; /* Pointer to string containing unparsed arg, and its length */
	int                length;
	nqiv_cmd_arg_value value;
} nqiv_cmd_arg_token;

typedef struct nqiv_cmd_manager_print_settings
{
	/* Loose information needed for various tasks related to printing the
	 * command tree. */
	int   indent;
	bool  dumpcfg;
	char* prefix;
} nqiv_cmd_manager_print_settings;

typedef struct nqiv_cmd_node nqiv_cmd_node;

struct nqiv_cmd_manager
{
	nqiv_state*                     state;
	nqiv_array*                     buffer;
	nqiv_cmd_manager_print_settings print_settings;
	nqiv_cmd_node*                  root_node;
};

/*Read characters until we get an EOL. Then, begin by traversing the tree to find the name of the
 * node. Once we no longer find names, we begin grabbing the parameters. We do paremeters by seeing
 * if they match whatever format. Then we create an array of structs, with a type enum, pointer to
 * the string, a union with the raw data, if relevant. Whenever we're finished with text, we can
 * traverse the string forward.*/

struct nqiv_cmd_node
{
	char* name;
	char* description;
	bool (*store_value)(nqiv_cmd_manager*, nqiv_cmd_arg_token**);
	void (*print_value)(nqiv_cmd_manager*);
	nqiv_cmd_arg_desc** args;
	/* Child nodes are chosen from a linked list of peers. */
	nqiv_cmd_node* peer;
	nqiv_cmd_node* child;
	/* Deprecated commands are still usable, but will not appear unless directly
	 * referenced. A branch node's deprecated status will be propagated to its
	 * children. */
	bool deprecated;
};

bool nqiv_cmd_add_line_and_parse(nqiv_cmd_manager* manager, const char* str);
bool nqiv_cmd_add_string(nqiv_cmd_manager* manager, const char* str);
bool nqiv_cmd_parse(nqiv_cmd_manager* manager);
bool nqiv_cmd_consume_stream(nqiv_cmd_manager* manager, FILE* stream);
bool nqiv_cmd_consume_stream_from_path(nqiv_cmd_manager* manager, const char* path);
void nqiv_cmd_manager_destroy(nqiv_cmd_manager* manager);
bool nqiv_cmd_manager_init(nqiv_cmd_manager* manager, nqiv_state* state);

/* Just helper functions that see use elsewhere. */
int nqiv_cmd_scan_not_whitespace(const char* data, const int start, const int end, int* length);
int nqiv_cmd_scan_whitespace(const char* data, const int start, const int end, int* length);
#endif /* NQIV_CMD_H */
