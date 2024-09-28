#ifndef NQIV_TYPEDEFS_H
#define NQIV_TYPEDEFS_H

/* Simple forward declaration to help with some structures that reference each other from different
 * files, as well as some other circularly-shared things. This file should not include any other
 * nqiv file and should be included before the circularly referenced elements are needed. */

typedef struct nqiv_cmd_manager nqiv_cmd_manager;
typedef struct nqiv_state       nqiv_state;

/* Standard return value for functions that need to report a more precise status than true or false.
 * These values should be 'truthy' though, evaluating as booleans would. */
typedef enum nqiv_op_result
{
	NQIV_FAIL = 0, /* An error occurred. (false) */
	NQIV_PASS,     /* No error, but operation did not complete. (true) */
	NQIV_SUCCESS,  /* Operation completed without error. (true) */
} nqiv_op_result;

#endif /* NQIV_TYPEDEFS_H */
