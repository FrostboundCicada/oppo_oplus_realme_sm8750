#!/bin/sh
# Overlay patched files from repo into common/ after upstream common is extracted
set -e
if [ -d "$GITHUB_WORKSPACE/patch_files" ]; then
  echo "Overlaying patched files from repo/patch_files onto common/"
  rsync -a --omit-dir-times --no-perms "$GITHUB_WORKSPACE/patch_files/" ./common/
else
  echo "No patched files to overlay"
fi
