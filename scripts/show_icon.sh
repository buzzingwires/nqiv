#!/bin/sh -efu

errcho()
{
	>&2 echo "$@";
}

exit_usage()
{
	local L_CODE="$1"

	errcho "Usage: $0 <dimension>..."

	exit "$L_CODE"
}

main()
{
	if [ $# -ne 1 ]
	then
		exit_usage 1
	fi

	local A_DIMENSION="$1"
	src/nqiv -N -B -C './scripts/media_common.cfg'      \
				   -c "set window width $A_DIMENSION"   \
				   -c "set window height $A_DIMENSION"  \
				   -c "set thumbnail size $A_DIMENSION" \
				   -c "sendkey set_montage"             \
				   './media/logo_N.png'                 \
	|| exit_usage 1

	return 0
}

main "$@"
