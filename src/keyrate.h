#ifndef NQIV_KEYRATE_H
#define NQIV_KEYRATE_H

#include <stdbool.h>

#include <SDL2/SDL.h>

/*
 * Handle key repeat rates and whether they should register on being being
 * pressed or released.
 *
 * There are default settings, and settings related to a particular keybind.
 * These include ephemeral information about the current key state, and actual
 * specifications as to how that should be handled. For the latter, setting them
 * to be less than zero will cause the default settings to be used.
 *
 * If the key is released, clear ephemeral data and then tell whether the
 * released key should be registered.
 *
 * If the key is held down and pressed keys are supposed to be registered,
 * follow the pattern in nqiv_keyrate_keystate_settings
 */

typedef enum nqiv_keyrate_release_option
{
	NQIV_KEYRATE_ON_NONE = 0,
	NQIV_KEYRATE_ON_DOWN = 1,
	NQIV_KEYRATE_ON_UP = 2
} nqiv_keyrate_release_option;

typedef enum nqiv_keyrate_press_action
{
	NQIV_KEYRATE_ON_MANAGER = 0,
	NQIV_KEYRATE_START = NQIV_KEYRATE_ON_MANAGER,
	NQIV_KEYRATE_ALLOW,
	NQIV_KEYRATE_DENY,
	NQIV_KEYRATE_END = NQIV_KEYRATE_DENY,
} nqiv_keyrate_press_action;

typedef struct nqiv_keyrate_keystate_ephemeral
{
	Uint64 current_delay;
	Uint64 next_event_time;
} nqiv_keyrate_keystate_ephemeral;

typedef struct nqiv_keyrate_keystate_settings
{
	/* When the key is first pressed, it must be pressed for this long before events are emitted. */
	Sint64 start_delay;
	/* After start delay has passed and key is deemed to have been pressed, this is the starting
	 * delay between each event. */
	Sint64 consecutive_delay;
	/* After each consecutive delay, subtract this from it... */
	Sint64 delay_accel;
	/* ...but cap at this minimum value */
	Sint64 minimum_delay;
} nqiv_keyrate_keystate_settings;

typedef struct nqiv_keyrate_keystate
{
	nqiv_keyrate_keystate_ephemeral ephemeral;
	nqiv_keyrate_keystate_settings  settings;
	/* Send events if the key is held down */
	nqiv_keyrate_press_action       send_on_down;
	/* Send events when the key is released */
	nqiv_keyrate_press_action       send_on_up;
} nqiv_keyrate_keystate;

typedef struct nqiv_keyrate_manager
{
	/* Defaults for each state */
	nqiv_keyrate_keystate_settings settings;
	bool                           send_on_down;
	bool                           send_on_up;
} nqiv_keyrate_manager;

bool nqiv_keyrate_filter_action(const nqiv_keyrate_manager*       manager,
                                /* Settings for this particular keybind. */
                                nqiv_keyrate_keystate*            state,
                                /* Consider the key pressed, released, or both. */
                                const nqiv_keyrate_release_option released,
                                /* Timestamp in SDL ticks (milliseconds) */
                                Uint64                            ticks);

#endif /* NQIV_KEYRATE_H */
