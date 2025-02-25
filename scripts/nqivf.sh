#!/bin/sh -efu

errcho()
{
	>&2 echo "$@";
}

exit_usage()
{
	local L_CODE="$1"

	errcho "Usage: $0 [option]... <target>..."
	errcho
	errcho "Options:"
	errcho
	errcho "'-h' : Print this message, then quit."
	errcho
	errcho "'-d' : Maximum depth to search for. Overrides -r"
	errcho
	errcho "'-r' : Scan for images recursively with unlimited depth. Overrides -d"
	errcho
	errcho "'-e' : Add extension for nqiv to open."
	errcho "       Default, case insensitive, containing:"
	errcho "       .jpg .jpeg .png .webp .gif .tiff .svg"
	errcho
	errcho "'-f' : Exclude the given pattern from image paths."
	errcho
	errcho "'-t' : Sort by modification time. Most recent first."
	errcho
	errcho "'-S' : Case sensitive sorting."
	errcho
	errcho "'-R' : Reverse sorting."
	errcho
	errcho "'-p' : Sort by whole path instead of filename."
	errcho
	errcho "'-n' : Do not do automatic name sort. This alone will sort by natural order."
	errcho
	errcho "'-l' : Print the commands list passed to nqiv to the given path."
	errcho
	errcho "'-q' : Quit instead of running nqiv- useful with -l"
	errcho
	errcho "Passed through nqiv options:"
	errcho
	errcho "$(nqiv -h 2>&1 | sed --quiet --posix --sandbox -E -e '/^-h.+$/d' -e '/^-v.+$/d' -e '/^-s.+$/d' -e '/^To create default config.+$/d' -e 'p')"

	exit "$L_CODE"
}

check_fifo()
{
	local L_PATH="$1"
	[ -p "$L_PATH" ]
}

check_file()
{
	local L_PATH="$1"
	[ -f "$L_PATH" ]
}

populate_file()
{
	L_TARGETS="$1"
	L_DEPTH="$2"
	L_EXTENSIONS="$3"
	L_FILTER="$4"
	L_SORT="$5"
	L_FILELIST_PATH="$6"

	eval find $L_TARGETS "$L_DEPTH" "$L_EXTENSIONS" "$L_FILTER" -type f -printf "%T@\\\t%f\\\t%p\\\n" \
	| eval "$L_SORT" \
	| sed --quiet --posix --sandbox -E -e 's/^.+\t.+\t(.+)$/append image \1/' -e 'p' \
	> "$l_filelist_path"
}

run_nqiv()
{
	# Args
	local L_TARGETS="$1"
	local L_DEPTH="$2"
	local L_SORT="$3"
	local L_EXTENSIONS="$4"
	local L_FILTER="$5"
	local L_PASSTHROUGH_ARGS="$6"
	local L_COMMAND_LIST="$7"
	local L_QUIT="$8"

	# Create file list
	local l_filelist_create="mkfifo"
	local l_filelist_check="check_fifo"
	local l_async="yes"
	if [ -n "$L_COMMAND_LIST" ] || ! command -v mkfifo >/dev/null 2>&1;
	then
		l_filelist_create="touch"
		l_filelist_check="check_file"
		l_async=""
	fi

	local l_filelist_dir="/tmp"
	local l_filelist_file="nqivf-tmp-files"
	local l_filelist_file_number="1"
	local l_filelist_path="$L_COMMAND_LIST"
	if [ -z "$l_filelist_path" ]
	then
		if command -v mktemp >/dev/null 2>&1;
		then
			l_filelist_dir="$(mktemp -p "$l_filelist_dir" -d .nqivf.XXXXXXXXXX)"
		else
			while [ $l_filelist_file_number -le 1000 ] && "$l_filelist_check" "$l_filelist_dir/$l_filelist_file-$l_filelist_file_number";
			do
				l_filelist_file_number="$((l_filelist_file_number + 1))"
			done
		fi
		l_filelist_path="$l_filelist_dir/$l_filelist_file-$l_filelist_file_number"
		if [ -e "$l_filelist_path" ] || [ $l_filelist_file_number -gt 1000 ]
		then
			errcho "Path of '$l_filelist_path' must not exist, and must be numbered less than or equal to 1000."
			exit 1
		fi
	fi

	"$l_filelist_create" "$l_filelist_path"

	# Populate file list
	if [ -n "$l_async" ]
	then
		populate_file "$L_TARGETS" "$L_DEPTH" "$L_EXTENSIONS" "$L_FILTER" "$L_SORT" "$l_filelist_path" &
	else
		populate_file "$L_TARGETS" "$L_DEPTH" "$L_EXTENSIONS" "$L_FILTER" "$L_SORT" "$l_filelist_path"
	fi

	# Actually run nqiv
	if [ -z "$L_QUIT" ]
	then
		eval nqiv -C "$l_filelist_path" -s "$L_PASSTHROUGH_ARGS"
	fi

	# Cleanup
	if [ -z "$L_COMMAND_LIST" ]
	then
		rm "$l_filelist_path"
	fi
	if [ "$l_filelist_dir" != "/tmp" ]
	then
		rmdir "$l_filelist_dir"
	fi
}

append_with_space()
{
	local L_EXTENSIONS="$1"
	local L_NEW="$2"
	echo "$L_EXTENSIONS $L_NEW"
}

append_extension()
{
	local L_EXTENSIONS="$1"
	local L_NEW="$2"
	echo "$L_EXTENSIONS -o -iname '*.$L_NEW*'"
}

append_filter()
{
	local L_FILTER="$1"
	local L_PATTERN="$2"
	echo "$L_FILTER ! -ipath '$L_PATTERN'"
}

prepare_args()
{
	for a in "$@"
	do
		echo -n "$a" | sed --quiet --posix --sandbox -E                \
													 -e 's/\\/\\\\\\/' \
													 -e 's/"/\"/'      \
													 -e 's/\$/\\$/'      \
													 -e 's/`/\`/'      \
													 -e 's/^/\"/'      \
													 -e 's/$/\" /'     \
													 -e 'p'
	done
}

main()
{
	set -eu
	local a_depth="-maxdepth 1"
	local a_extensions="\\( -iname '*.jpg*' -o -iname '*.jpeg*' -o -iname '*.png*' -o -iname '*.webp*' -o -iname '*.gif*' -o -iname '*.tiff*'  -o -iname '*.svg*'"
	local a_filter=""
	local a_time_sort=""
	local a_ignore_sort_case="-f"
	local a_reverse_sort=""
	local a_reverse_reversed_sort="-r"
	local a_reverse_natural=""
	local a_sort_base="| sort --version-sort --stable --field-separator '	' --key"
	local a_name_sort_key="2"
	local a_passthrough_args=""
	local a_command_list=""
	local a_quit=""
	while getopts "he:f:tSRrd:pnBNc:C:l:q" opt;
	do
		if [ "$(echo "$OPTARG" | cut -c -1)" = "-" ]
		then
			errcho "Argument option '$OPTARG' for '$opt' must not start with a '-'."
			exit 1
		fi
		case "$opt" in
		'h')
			exit_usage 0
			;;
		'r')
			a_depth=""
			;;
		'd')
			a_depth="-maxdepth $OPTARG"
			;;
		'e')
			a_extensions="$(append_extension "$a_extensions" "$OPTARG")"
			;;
		'f')
			a_filter="$(append_filter "$a_filter" "$OPTARG")"
			;;
		't')
			a_time_sort="| sort --general-numeric-sort --stable --key 1 --field-separator '	'"
			;;
		'S')
			a_ignore_sort_case=""
			;;
		'R')
			a_reverse_sort="-r"
			a_reverse_reversed_sort=""
			a_reverse_natural="| sed --quiet --posix --sandbox -e '1!G;h;\$!d' -e 'p'"
			;;
		'p')
			a_name_sort_key="3"
			;;
		'n')
			a_sort_base=""
			;;
		'l')
			a_command_list="$OPTARG"
			;;
		'q')
			a_quit="yes"
			;;
		'B')
			a_passthrough_args="$(append_with_space "$a_passthrough_args" "-B")"
			;;
		'N')
			a_passthrough_args="$(append_with_space "$a_passthrough_args" "-N")"
			;;
		'c')
			a_passthrough_args="$(append_with_space "$a_passthrough_args" "-c \"$OPTARG\"")"
			;;
		'C')
			a_passthrough_args="$(append_with_space "$a_passthrough_args" "-C \"$OPTARG\"")"
			;;
		*)
			errcho "Error: Unrecognized flag '$opt'."
			exit_usage 1
			;;
		esac
	done
	shift $((OPTIND - 1))

	l_sort="cat"
	if [ -n "$a_sort_base" ]
	then
		l_sort="$(append_with_space "$l_sort" "$a_sort_base")"
		l_sort="$(append_with_space "$l_sort" "$a_name_sort_key")"
		l_sort="$(append_with_space "$l_sort" "$a_ignore_sort_case")"
		l_sort="$(append_with_space "$l_sort" "$a_reverse_sort")"
	else
		l_sort="$(append_with_space "$l_sort" "$a_reverse_natural")"
	fi
	if [ -n "$a_time_sort" ]
	then
		l_sort="$(append_with_space "$l_sort" "$a_time_sort")"
		l_sort="$(append_with_space "$l_sort" "$a_ignore_sort_case")"
		l_sort="$(append_with_space "$l_sort" "$a_reverse_reversed_sort")"
	fi

	a_extensions="$(append_with_space "$a_extensions" "\\)")"

	local l_targets="."
	if [ $# -gt 0 ]
	then
		l_targets="$(prepare_args "$@")"
	fi

	#echo "$l_sort"
	run_nqiv "$l_targets" "$a_depth" "$l_sort" "$a_extensions" "$a_filter" "$a_passthrough_args" "$a_command_list" "$a_quit"

	return 0
}

main "$@"
