#include "typedefs.h"

#include <inttypes.h>
#include <string.h>

#include <omp.h>

#include "event.h"

void nqiv_shared_var_init(nqiv_shared_var* var)
{
	memset(var, 0, sizeof(nqiv_shared_var));
	omp_init_lock(&var->lock);
}

void nqiv_shared_var_destroy(nqiv_shared_var* var)
{
	omp_destroy_lock(&var->lock);
	memset(var, 0, sizeof(nqiv_shared_var));
}

void nqiv_shared_var_lock(nqiv_shared_var* var)
{
	omp_set_lock(&var->lock);
}

void nqiv_shared_var_unlock(nqiv_shared_var* var)
{
	omp_unset_lock(&var->lock);
}

void nqiv_shared_var_set_op_result(nqiv_shared_var* var, const nqiv_op_result value)
{
	nqiv_shared_var_lock(var);
	var->data.as_op_result = value;
	nqiv_shared_var_unlock(var);
}

nqiv_op_result nqiv_shared_var_get_op_result(nqiv_shared_var* var)
{
	nqiv_shared_var_lock(var);
	const nqiv_op_result result = var->data.as_op_result;
	nqiv_shared_var_unlock(var);
	return result;
}

void nqiv_shared_var_inc_int(nqiv_shared_var* var)
{
	nqiv_shared_var_lock(var);
	var->data.as_int += 1;
	nqiv_shared_var_unlock(var);
}

void nqiv_shared_var_dec_int(nqiv_shared_var* var)
{
	nqiv_shared_var_lock(var);
	var->data.as_int -= 1;
	nqiv_shared_var_unlock(var);
}

int64_t nqiv_shared_var_get_int(nqiv_shared_var* var)
{
	nqiv_shared_var_lock(var);
	const int64_t result = var->data.as_int;
	nqiv_shared_var_unlock(var);
	return result;
}

void nqiv_shared_var_set_int(nqiv_shared_var* var, const int64_t value)
{
	nqiv_shared_var_lock(var);
	var->data.as_int = value;
	nqiv_shared_var_unlock(var);
}
