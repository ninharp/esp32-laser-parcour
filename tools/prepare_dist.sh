#!/bin/bash

if [ ! $# -gt 0 ]; then
  DIST_FOLDER="dist"
else
  DIST_FOLDER=$1
fi

if [ ! -d $DIST_FOLDER ]; then
  mkdir $DIST_FOLDER
  echo "Created non existent distribution folder $DIST_FOLDER"
fi

if [ $# -gt 1 ]; then
  DIST_VERSION=$2
  if [ ! -d $DIST_FOLDER/$DIST_VERSION ]; then
    mkdir $DIST_FOLDER/$DIST_VERSION
    echo "Created non existent version folder $DIST_FOLDER/$DIST_VERSION"
  fi
fi