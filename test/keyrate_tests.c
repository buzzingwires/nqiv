#include <assert.h>
#include <string.h>

#include "../src/keyrate.h"

#include "keyrate_tests.h"

void keyrate_test_default(void)
{
	nqiv_keyrate_manager  manager = {0};
	nqiv_keyrate_keystate state = {0};

	manager.settings.start_delay = 50;
	manager.settings.consecutive_delay = 25;
	manager.settings.delay_accel = 5;
	manager.settings.minimum_delay = 10;
	manager.send_on_down = true;
	manager.send_on_up = true;

	state.settings.start_delay = -1;
	state.settings.consecutive_delay = -1;
	state.settings.delay_accel = -1;
	state.settings.minimum_delay = -1;
	state.send_on_down = NQIV_KEYRATE_ON_MANAGER;
	state.send_on_up = NQIV_KEYRATE_ON_MANAGER;

	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_UP, 0));
	assert(
		nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_UP | NQIV_KEYRATE_ON_DOWN, 0));
	assert(!nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 0));
	assert(!nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 0));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 50));
	assert(!nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 50));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 75));
	assert(!nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 75));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 100));
	assert(!nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 100));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 120));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 135));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 145));
	assert(!nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 150));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 155));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_UP, 0));
	assert(!nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 165));
	assert(!nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 175));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 215));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 240));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 260));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_UP, 0));

	manager.settings.start_delay = 0;
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 0));
	assert(!nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 0));
	manager.settings.consecutive_delay = 0;
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_UP, 0));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 0));

	state.settings.start_delay = 40;
	state.settings.consecutive_delay = 30;
	state.settings.delay_accel = 20;
	state.settings.minimum_delay = 10;
	state.send_on_down = NQIV_KEYRATE_ALLOW;
	state.send_on_up = NQIV_KEYRATE_DENY;

	assert(!nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_UP, 0));
	assert(!nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 0));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 40));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 70));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 80));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 90));
	assert(nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_DOWN, 100));
	assert(!nqiv_keyrate_filter_action(&manager, &state, NQIV_KEYRATE_ON_UP, 0));
}
