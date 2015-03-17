#!/bin/bash
# migrates current directory to git
REMOTE=$1
git init
bzr fast-export --plain . | git fast-import
git checkout -f master
git remote add origin $REMOTE
git push origin master
echo "If everything is OK, execute 'rm -rf ~/.bzr'."
