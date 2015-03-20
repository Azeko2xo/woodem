#!/bin/sh
# replace the current repository (bzr-controlled?) by git 
set -x -e
git init
git remote add origin $1
git fetch origin
git reset --hard origin/master
git checkout -f -b master --track origin/master
echo "If everything is OK, run 'rm -rf .bzr'"
