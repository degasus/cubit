#!/bin/sh

export CUBITVERSION=0.0.5


mkdir -p debs

cd debs

rm -rf cubit-* cubit*diff.gz cubit*dsc cubit*build cubit*changes 

git clone .. cubit-$CUBITVERSION

rm -rf cubit-$CUBITVERSION/.git

if [ ! -e cubit_$CUBITVERSION.orig.tar.gz ]
then echo "Creating new orig.tar.gz file"
tar -zcf cubit_$CUBITVERSION.orig.tar.gz cubit-$CUBITVERSION
fi

cd cubit-$CUBITVERSION

debuild

cd ..

