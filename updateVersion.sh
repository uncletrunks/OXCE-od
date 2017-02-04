#!/bin/bash

if [ -z "$1" ]; then
	echo "Arg not set"
	exit
fi

version_major=$1
version_minor=$2
version_patch=$3

vim -c ":/\[b\]Mod/ | call setline(line('.'), '[b]Mod version ${version_major}.${version_minor}${version_patch}[/b]') | /\[examples/ | ?\[br? | call append(line('.'), ['${version_major}.${version_minor}${version_patch}:[br/]', '[br/]']) | wq" Extended.txt
vim -c ":/OPENXCOM_VERSION_SHORT/ | call setline(line('.'), ['#define OPENXCOM_VERSION_SHORT \"${version_major}.${version_minor}${version_patch}\"', '#define OPENXCOM_VERSION_LONG \"${version_major}.${version_minor}.0.0\"','#define OPENXCOM_VERSION_NUMBER ${version_major},${version_minor},0,0']) | wq" src/version.h
git add Extended.txt src/version.h
git commit -m"Version bump to ${version_major}.${version_minor}${version_patch}"
