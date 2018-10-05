#!/bin/sh

[ "$#" -ne 1 ] && exit 1

echo "tree" | "$1" example.iso
