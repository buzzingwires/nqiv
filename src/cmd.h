#ifndef NQIV_CMD_H
#define NQIV_CMD_H

#include <stdbool.h>

#include <SDL2/SDL.h>

#include "array.h"
#include "state.h"
#include "logging.h"
#include "keybinds.h"
#include "keyrate.h"

#define NQIV_CMD_MAX_ARGS 4
#define NQIV_CMD_DUMPCFG_BUFFER_LENGTH 1024

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
} nqiv_event_type;

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
	bool spaceless;
} nqiv_cmd_arg_desc_setting_string;

typedef struct nqiv_cmd_arg_desc_setting_key_action
{
	bool brief;
} nqiv_cmd_arg_desc_setting_key_action;

typedef union nqiv_cmd_arg_desc_setting
{
	nqiv_cmd_arg_desc_setting_int of_int;
	nqiv_cmd_arg_desc_setting_Uint64 of_Uint64;
	nqiv_cmd_arg_desc_setting_double of_double;
	nqiv_cmd_arg_desc_setting_string of_string;
	nqiv_cmd_arg_desc_setting_keyaction of_key_action;
} nqiv_cmd_arg_desc_setting;

typedef struct nqiv_cmd_arg_desc
{
	nqiv_cmd_arg_type type;
	nqiv_cmd_arg_desc_setting setting;
} nqiv_cmd_arg_desc;

typedef union nqiv_cmd_arg_value
{
	int as_int;
	double as_double;
	Uint64 as_Uint64;
	Uint8 as_Uint8;
	bool as_bool;
	nqiv_log_level as_log_level;
	nqiv_keyrate_press_action as_press_action;
	nqiv_key_action as_key_action;
	nqiv_keybind_pair as_keybind;
} nqiv_cmd_arg_value;

typedef struct nqiv_cmd_arg_token
{
	nqiv_cmd_arg_type type;
	char* raw;
	int length;
	nqiv_cmd_arg_value value;
} nqiv_cmd_arg_token;

/*Read characters until we get an EOL. Then, begin by traversing the tree to find the name of the node. Once we no longer find names, we begin grabbing the parameters. We do paremeters by seeing if they match whatever format. Then we create an array of structs, with a type enum, pointer to the string, a union with the raw data, if relevant. Whenever we're finished with text, we can traverse the string forward.*/
typedef struct nqiv_cmd_node nqiv_cmd_node;

/* TODO Stifle repeated events? */
struct nqiv_cmd_node
{
	char* name;
	char* description;
	bool (*store_value)(nqiv_cmd_manager*, nqiv_cmd_arg_token**);
	void (*print_value)(nqiv_cmd_manager*, const int);
	nqiv_cmd_arg_desc* args[];
	nqiv_cmd_node* children[];
};

typedef struct nqiv_cmd_manager
{
	nqiv_state* state;
	nqiv_array* buffer;
} nqiv_cmd_manager;

bool nqiv_cmd_add_line_and_parse(nqiv_cmd_manager* manager, const char* str);
bool nqiv_cmd_consume_stream(nqiv_cmd_manager* manager, FILE* stream);
bool nqiv_cmd_consume_stream_from_path(nqiv_cmd_manager* manager, const char* path);
void nqiv_cmd_manager_destroy(nqiv_cmd_manager* manager);
bool nqiv_cmd_manager_init(nqiv_cmd_manager* manager, nqiv_state* state);
#endif /* NQIV_CMD_H */
