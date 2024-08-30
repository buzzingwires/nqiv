#include <assert.h>
#include <string.h>

#include "../src/keybinds.h"

#include "keybind_tests.h"

bool keybind_test_parse_print_entry(const bool expected, const char* input_buf_arg, const char* compare_buf_arg)
{
	bool result = true;
	nqiv_keybind_pair pair = {0};
	char input_buf[NQIV_KEYBIND_STRLEN] = {0};
	char compare_buf[NQIV_KEYBIND_STRLEN] = {0};
	char output_buf[NQIV_KEYBIND_STRLEN] = {0};
	result = result && strlen(input_buf_arg) < NQIV_KEYBIND_STRLEN;
	result = result && strlen(compare_buf_arg) < NQIV_KEYBIND_STRLEN;
	memcpy( input_buf, input_buf_arg, strlen(input_buf_arg) );
	memcpy( compare_buf, compare_buf_arg, strlen(compare_buf_arg) );
	result = result && nqiv_keybind_text_to_keybind(input_buf, &pair) != -1;
	result = result && nqiv_keybind_to_string(&pair, output_buf);
	result = result && strlen(output_buf) < NQIV_KEYBIND_STRLEN;
	result = result && strcmp(output_buf, compare_buf) == 0;
	return result == expected;
}

void keybind_test_parse_print(void)
{
	assert( keybind_test_parse_print_entry(true,
		"Q+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+mode=quit+allow_on_up+allow_on_down+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+Q=allow_on_down+allow_on_up+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100+quit") );
	assert( keybind_test_parse_print_entry(true,
		"mouse0_double+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+mode=quit+allow_on_up+allow_on_down+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+mouse0_double=allow_on_down+allow_on_up+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100+quit") );
	assert( keybind_test_parse_print_entry(true,
		"mouse0_double+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+mode=quit+deny_on_up+deny_on_down+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+mouse0_double=deny_on_down+deny_on_up+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100+quit") );
	assert( keybind_test_parse_print_entry(true,
		"scroll_forward+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+mode=quit+deny_on_up+deny_on_down+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+scroll_forward=deny_on_down+deny_on_up+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100+quit") );
	assert( keybind_test_parse_print_entry(true,
		"scroll_backward+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+mode=quit+deny_on_up+deny_on_down+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+scroll_backward=deny_on_down+deny_on_up+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100+quit") );
	assert( keybind_test_parse_print_entry(true,
		"scroll_left+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+mode=quit+deny_on_up+deny_on_down+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+scroll_left=deny_on_down+deny_on_up+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100+quit") );
	assert( keybind_test_parse_print_entry(true,
		"scroll_right+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+mode=quit+deny_on_up+deny_on_down+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+scroll_right=deny_on_down+deny_on_up+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100+quit") );
	assert( keybind_test_parse_print_entry(false,
		"scroll_right+scroll_left+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+mode=quit+deny_on_up+deny_on_down+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+scroll_right+scroll_left=deny_on_down+deny_on_up+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100+quit") );
	assert( keybind_test_parse_print_entry(false,
		"scroll_right+shift+lshift+rshift+ctrl+rctrl+lctrl+alt+ralt+lalt+gui+lgui+rgui+num+caps+mode=quit+deny_on_up+deny_on_down+allow_on_up+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100",
		"num+caps+mode+ctrl+shift+alt+scroll_right=allow_on_up+deny_on_down+deny_on_up+start_delay_100+consecutive_delay_100+delay_accel_100+minimum_delay_100+quit") );
}
