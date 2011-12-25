#!/bin/sh

export PROJECT=librazorqt
export OPTS="-noobsolete"
export TARGET="-recursive .."

../../../scripts/translate-one.sh "$@"
