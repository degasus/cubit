#!/bin/sh

export CUBITVERSION=0.0.1


mkdir -p debs
cd debs
rm -rf cubit-* cubit_*
git clone .. cubit-$CUBITVERSION
rm -rf cubit-$CUBITVERSION/.git
tar -zcf cubit_$CUBITVERSION.orig.tar.gz cubit-$CUBITVERSION
cd cubit-$CUBITVERSION
debuild
cd ..
rm -rf cubit-$CUBITVERSION
