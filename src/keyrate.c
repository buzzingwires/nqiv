#include <stdbool.h>

#include <SDL2/SDL.h>

#include "keybinds.h"
#include "keyrate.h"

Uint64 nqiv_keyrate_get_numerical_setting(const Uint64* manager, const Uint64* state)
{
	if(*state == 0) {
		return *manager;
	} else {
		return *state;
	}
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

bool nqiv_keyrate_filter_action(nqiv_keyrate_manager* manager, const nqiv_key_action action, const bool released)
{
	bool output = false;
	nqiv_keyrate_keystate* state = &manager->states[action];
	if(released) {
		memset( &state->ephemeral, 0, sizeof(nqiv_keyrate_keystate_ephemeral) );
		output = nqiv_keyrate_get_bool_setting(&manager->send_on_up, &state->send_on_up);
	} else if( nqiv_keyrate_get_bool_setting(&manager->send_on_down, &state->send_on_down) ) {
		if(state->ephemeral.next_event_time == 0) {
			const Uint64 start_delay = nqiv_keyrate_get_numerical_setting(&manager->settings.start_delay, &state->settings.start_delay);
			const Uint64 consecutive_delay = nqiv_keyrate_get_numerical_setting(&manager->settings.consecutive_delay, &state->settings.consecutive_delay);
			state->ephemeral.current_delay = consecutive_delay;
			if(start_delay == 0) {
				output = true;
				state->ephemeral.next_event_time = SDL_GetTicks64() + state->ephemeral.current_delay;
			} else {
				state->ephemeral.next_event_time = SDL_GetTicks64() + start_delay;
			}
		} else if(SDL_GetTicks64() >= state->ephemeral.next_event_time) {
			const Uint64 delay_accel = nqiv_keyrate_get_numerical_setting(&manager->settings.delay_accel, &state->settings.delay_accel);
			state->ephemeral.next_event_time = SDL_GetTicks64() + state->ephemeral.current_delay;
			if(state->ephemeral.current_delay >= delay_accel) {
				state->ephemeral.current_delay -= delay_accel;
			}
			output = true;
		}
		const Uint64 minimum_delay = nqiv_keyrate_get_numerical_setting(&manager->settings.minimum_delay, &state->settings.minimum_delay);
		if(state->ephemeral.current_delay < minimum_delay) {
			state->ephemeral.current_delay = minimum_delay;
		}
	}
	return output;
}
