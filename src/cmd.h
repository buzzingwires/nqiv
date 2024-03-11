#ifndef NQIV_CMD_H
#define NQIV_CMD_H

#include "array.h"
#include "state.h"

/*
We have a buffer represented by a dynamic array which bytes can be freely added to.
It is possible to get safe (null-terminated) strings from within this array,
by means of temporarily replacing values.
We can also increment our position within this string.
When a function 'consumes' string, it does this incrementation based on the current position.
A consume function does *not* increment the string if it fails.
Each consume function returns a boolean value, and takes the parsing manager as a parameter.
The consume function *may* set an output value, passed as a pointer.
In the event that all consume functions have failed, we can then return false and print a message.
Each subcommand function has a help directive.
It is possible to reset the consume counter, which be done at the end of each failed consumption
*/
typedef struct nqiv_cmd_buffer
{
	nqiv_array* array;
	int bytes_consumed;
	int bytes_attempted;
	char replaced_byte;
	int replaced_byte_index;
} nqiv_cmd_buffer;

typedef struct nqiv_cmd_node nqiv_cmd_node;

/* TODO Stifle repeated events? */
struct nqiv_cmd_node
{
	char* name;
	char* description;
	nqiv_cmd_node* children;
	bool (*parser)(nqiv_cmd_manager*, const bool);
	bool (*parameter_grabber)(nqiv_cmd_manager*, const bool, void*);
};

typedef struct nqiv_cmd_manager
{
	nqiv_state* state;
	nqiv_cmd_buffer buffer;
	nqiv_cmd_node* current_node;
} nqiv_cmd_manager;

bool nqiv_cmd_destroy(nqiv_cmd_manager* manager);
bool nqiv_cmd_init(nqiv_cmd_manager* manager, nqiv_state* state);
bool nqiv_cmd_add_byte(nqiv_cmd_manager* manager, const char byte);
bool nqiv_cmd_parse(nqiv_cmd_manager* manager);
bool nqiv_cmd_add_then_parse(nqiv_cmd_manager* manager, const char byte);

#endif /* NQIV_CMD_H */
