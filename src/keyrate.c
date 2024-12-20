#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <SDL2/SDL.h>

#include "keyrate.h"

Uint64 nqiv_keyrate_get_numerical_setting(const Sint64* manager, const Sint64* state)
{
	assert(*manager >= 0);
	return *state < 0 ? (Uint64)(*manager) : (Uint64)(*state);
}

bool nqiv_keyrate_get_bool_setting(const bool* manager, const nqiv_keyrate_press_action* state)
{
	if(*state == NQIV_KEYRATE_ON_MANAGER) {
		return *manager;
	} else if(*state == NQIV_KEYRATE_ALLOW) {
		return true;
	} else {
		return false;
	}
}

bool nqiv_keyrate_filter_action(const nqiv_keyrate_manager*       manager,
                                nqiv_keyrate_keystate*            state,
                                const nqiv_keyrate_release_option released,
                                Uint64                            ticks)
{
	bool output = false;
	if((released & NQIV_KEYRATE_ON_UP) != 0) {
		memset(&state->ephemeral, 0, sizeof(nqiv_keyrate_keystate_ephemeral));
		output = nqiv_keyrate_get_bool_setting(&manager->send_on_up, &state->send_on_up);
	}
	if(output) {
		return output;
	}
	if((released & NQIV_KEYRATE_ON_DOWN) != 0
	   && nqiv_keyrate_get_bool_setting(&manager->send_on_down, &state->send_on_down)) {
		if(state->ephemeral.next_event_time == 0) {
			const Uint64 start_delay = nqiv_keyrate_get_numerical_setting(
				&manager->settings.start_delay, &state->settings.start_delay);
			const Uint64 consecutive_delay = nqiv_keyrate_get_numerical_setting(
				&manager->settings.consecutive_delay, &state->settings.consecutive_delay);
			state->ephemeral.current_delay = consecutive_delay;
			if(start_delay == 0) {
				output = true;
				state->ephemeral.next_event_time = ticks + state->ephemeral.current_delay;
			} else {
				state->ephemeral.next_event_time = ticks + start_delay;
			}
		} else if(ticks >= state->ephemeral.next_event_time) {
			const Uint64 delay_accel = nqiv_keyrate_get_numerical_setting(
				&manager->settings.delay_accel, &state->settings.delay_accel);
			state->ephemeral.next_event_time = ticks + state->ephemeral.current_delay;
			if(state->ephemeral.current_delay >= delay_accel) {
				state->ephemeral.current_delay -= delay_accel;
			} else {
				state->ephemeral.current_delay = 0;
			}
			output = true;
		}
		const Uint64 minimum_delay = nqiv_keyrate_get_numerical_setting(
			&manager->settings.minimum_delay, &state->settings.minimum_delay);
		if(state->ephemeral.current_delay < minimum_delay) {
			state->ephemeral.current_delay = minimum_delay;
		}
	}
	return output;
}
