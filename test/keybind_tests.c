#include <assert.h>
#include <string.h>

#include "../src/keybinds.h"
#include "../src/state.h"

#include "keybind_tests.h"

bool keybind_test_parse_print_entry(const bool  expected,
                                    const char* input_buf_arg,
                                    const char* compare_buf_arg)
{
	bool              result = true;
	nqiv_keybind_pair pair = {0};
	char              input_buf[NQIV_KEYBIND_STRLEN + 1] = {0};
	char              compare_buf[NQIV_KEYBIND_STRLEN + 1] = {0};
	char              output_buf[NQIV_KEYBIND_STRLEN + 1] = {0};
	result = result && strlen(input_buf_arg) < NQIV_KEYBIND_STRLEN;
	result = result && strlen(compare_buf_arg) < NQIV_KEYBIND_STRLEN;
	strcpy(input_buf, input_buf_arg);
	strcpy(compare_buf, compare_buf_arg);
	result = result && nqiv_keybind_text_to_keybind(input_buf, &pair) != -1;
	result = result && nqiv_keybind_to_string(&pair, output_buf);
	result = result && strlen(output_buf) < NQIV_KEYBIND_STRLEN;
	result = result && strcmp(output_buf, compare_buf) == 0;
	return result == expected;
}

void keybind_test_parse_print(void)
{
	assert(keybind_test_parse_print_entry(
		true,
		"Q+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+mode=quit+"
		"allow_on_up+allow_on_down",
		"num+caps+mode+ctrl+shift+alt+Q=allow_on_down+allow_on_up+quit"));
	assert(keybind_test_parse_print_entry(
		true,
		"Q+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+mode=quit+"
		"allow_on_up+allow_on_down+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_"
		"delay_100",
		"num+caps+mode+ctrl+shift+alt+Q=allow_on_down+allow_on_up+start_delay_100+consecutive_"
		"delay_100+delay_accel_100+minimum_delay_100+quit"));
	assert(keybind_test_parse_print_entry(
		true,
		"mouse0_double+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+"
		"mode=quit+allow_on_up+allow_on_down+start_delay_100+consecutive_delay_100+delay_accel_100+"
		"minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+mouse0_double=allow_on_down+allow_on_up+start_delay_100+"
		"consecutive_delay_100+delay_accel_100+minimum_delay_100+quit"));
	assert(keybind_test_parse_print_entry(
		true,
		"mouse0_double+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+"
		"mode=quit+deny_on_up+deny_on_down+start_delay_100+consecutive_delay_100+delay_accel_100+"
		"minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+mouse0_double=deny_on_down+deny_on_up+start_delay_100+"
		"consecutive_delay_100+delay_accel_100+minimum_delay_100+quit"));
	assert(keybind_test_parse_print_entry(
		true,
		"scroll_forward+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+"
		"mode=quit+deny_on_up+deny_on_down+start_delay_100+consecutive_delay_100+delay_accel_100+"
		"minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+scroll_forward=deny_on_down+deny_on_up+start_delay_100+"
		"consecutive_delay_100+delay_accel_100+minimum_delay_100+quit"));
	assert(keybind_test_parse_print_entry(
		true,
		"scroll_backward+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+"
		"mode=quit+deny_on_up+deny_on_down+start_delay_100+consecutive_delay_100+delay_accel_100+"
		"minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+scroll_backward=deny_on_down+deny_on_up+start_delay_100+"
		"consecutive_delay_100+delay_accel_100+minimum_delay_100+quit"));
	assert(keybind_test_parse_print_entry(
		true,
		"scroll_left+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+"
		"mode=quit+deny_on_up+deny_on_down+start_delay_100+consecutive_delay_100+delay_accel_100+"
		"minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+scroll_left=deny_on_down+deny_on_up+start_delay_100+"
		"consecutive_delay_100+delay_accel_100+minimum_delay_100+quit"));
	assert(keybind_test_parse_print_entry(
		true,
		"scroll_right+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+"
		"mode=quit+deny_on_up+deny_on_down+start_delay_100+consecutive_delay_100+delay_accel_100+"
		"minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+scroll_right=deny_on_down+deny_on_up+start_delay_100+"
		"consecutive_delay_100+delay_accel_100+minimum_delay_100+quit"));
	assert(keybind_test_parse_print_entry(
		false,
		"scroll_right+scroll_left+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+"
		"num+caps+mode=quit+deny_on_up+deny_on_down+start_delay_100+consecutive_delay_100+delay_"
		"accel_100+minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+scroll_right+scroll_left=deny_on_down+deny_on_up+start_delay_"
		"100+consecutive_delay_100+delay_accel_100+minimum_delay_100+quit"));
	assert(keybind_test_parse_print_entry(
		false,
		"scroll_right+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+"
		"mode=quit+deny_on_up+deny_on_down+allow_on_up+start_delay_100+consecutive_delay_100+delay_"
		"accel_100+minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+scroll_right=allow_on_up+deny_on_down+deny_on_up+start_delay_"
		"100+consecutive_delay_100+delay_accel_100+minimum_delay_100+quit"));
}

bool add_keybind_string(nqiv_keybind_manager* manager, const char* text)
{
	nqiv_keybind_pair pair = {0};
	return nqiv_keybind_text_to_keybind(text, &pair) != -1 && nqiv_keybind_add(manager, &pair);
}

void keybind_test_lookup(void)
{
	nqiv_log_ctx         logger = {0};
	nqiv_keybind_manager manager = {0};
	nqiv_queue           queue = {0};
	nqiv_key_match       match = {0};

	match.data.key.mod |= KMOD_SHIFT;
	match.data.key.mod |= KMOD_CTRL;
	match.data.key.mod |= KMOD_ALT;
	match.data.key.mod |= KMOD_GUI;
	match.data.key.mod |= KMOD_NUM;
	match.data.key.mod |= KMOD_CAPS;
	match.data.key.mod |= KMOD_MODE;
	match.data.key.mod |= KMOD_SCROLL;

	nqiv_log_init(&logger);
	nqiv_log_set_prefix_format(&logger, "#level# #time:%Y-%m-%d %T%z# ");
	nqiv_log_add_stream(&logger, stderr);
	logger.level = NQIV_LOG_ERROR;
	assert(!nqiv_log_has_error(&logger));

	assert(nqiv_queue_init(&queue, &logger, sizeof(nqiv_keybind_pair*), STARTING_QUEUE_LENGTH));
	assert(nqiv_keybind_create_manager(&manager, &logger, STARTING_QUEUE_LENGTH));

	assert(add_keybind_string(
		&manager,
		"Q+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+mode=quit"));
	assert(add_keybind_string(&manager, "mouse0_double+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+"
	                                    "ralt+lalt+gui+lgui+rgui+num+caps+mode=quit"));
	assert(add_keybind_string(&manager, "scroll_forward+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+"
	                                    "ralt+lalt+gui+lgui+rgui+num+caps+mode=quit"));

	match.mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
	assert(nqiv_keybind_lookup(&manager, &match, &queue) == NQIV_KEY_LOOKUP_NOT_FOUND);

	match.data.key.scancode = SDL_SCANCODE_Q;
	assert(nqiv_keybind_lookup(&manager, &match, &queue) == NQIV_KEY_LOOKUP_NOT_FOUND);

	match.data.mouse_button.button = 0;
	match.data.mouse_button.clicks = 2;
	assert(nqiv_keybind_lookup(&manager, &match, &queue) == NQIV_KEY_LOOKUP_NOT_FOUND);

	match.mode |= NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_FORWARD;
	assert(nqiv_keybind_lookup(&manager, &match, &queue) == NQIV_KEY_LOOKUP_FOUND);
	assert(nqiv_array_get_units_count(queue.array) == 1);

	match.mode |= NQIV_KEY_MATCH_MODE_KEY;
	assert(nqiv_keybind_lookup(&manager, &match, &queue) == NQIV_KEY_LOOKUP_NOT_FOUND);

	match.mode &= ~NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_FORWARD;
	assert(nqiv_keybind_lookup(&manager, &match, &queue) == NQIV_KEY_LOOKUP_FOUND);
	assert(nqiv_array_get_units_count(queue.array) == 2);

	match.mode &= ~NQIV_KEY_MATCH_MODE_KEY;
	match.mode |= NQIV_KEY_MATCH_MODE_MOUSE_BUTTON;
	assert(nqiv_keybind_lookup(&manager, &match, &queue) == NQIV_KEY_LOOKUP_FOUND);
	assert(nqiv_array_get_units_count(queue.array) == 3);

	nqiv_queue_destroy(&queue);
	nqiv_keybind_destroy_manager(&manager);
	nqiv_log_destroy(&logger);
}
