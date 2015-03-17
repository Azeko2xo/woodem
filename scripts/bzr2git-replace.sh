#!/bin/sh
# replace the current repository (bzr-controlled?) by git 
git init
git remote add origin $1
git fetch origin
git checkout -b master --track origin/master
git reset origin/master
echo "If everything is OK, run 'rm -rf .bzr'"
