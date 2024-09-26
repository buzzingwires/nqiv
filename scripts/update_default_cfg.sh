#!/bin/sh -efu

src/nqiv -N -B -C './scripts/platform.cfg' -c "dumpcfg" > ./default.cfg
