#!/bin/bash
commitID=$(git rev-parse HEAD)
openXcomID=$(git merge-base HEAD supsuper/master)
openXcomDate=$(date -d"$(git show -s --format=%ci $openXcomID)" +%d\\/%m\\/%Y)

sed \
  -e 's/\[b\]//g' \
  -e 's/\[\/b\]//g' \
  -e 's/\[i\]//g' \
  -e 's/\[\/i\]//g' \
  -e 's/\[br\/\]//g' \
  -e 's/\[commit\/\]/'$commitID'/g' \
  -e 's/\[commitOriginDate\/\]/'$openXcomDate'/g' \
  -e 's/\[commitOrigin\/\]/'$openXcomID'/g' \
  ../Extended.txt > Readme.txt
sed \
  -e 's/  /\&nbsp;\&nbsp;/g' \
  -e 's/ #/\&nbsp;#/g' \
  -e 's/\[b\]/<h2 style="font-style:italic"><strong>/g' \
  -e 's/\[\/b\]/<\/strong><\/h2>/g' \
  -e 's/\[i\]/<p><strong>/g' \
  -e 's/\[\/i\]/<\/strong><\/p>/g' \
  -e 's/\[code\]/<div style="font-family: monospace; white-space: pre;">/g' \
  -e 's/\[\/code\]/<\/div>/g' \
  -e 's/\[br\/\]/<br \/>/g' \
  -e 's/\[commit\/\]/'$commitID'/g' \
  -e 's/\[commitOriginDate\/\]/'$openXcomDate'/g' \
  -e 's/\[commitOrigin\/\]/'$openXcomID'/g' \
  ../Extended.txt > OpenXcomExWeb.txt

sed \
  -e 's/\[b\]/\[b\]\[size=14pt\]/g' \
  -e 's/\[\/b\]/\[\/size\]\[\/b\]/g' \
  -e 's/\[br\/\]//g' \
  -e 's/\[i\]/\[b\]/g' \
  -e 's/\[\/i\]/\[\/b\]/g' \
  -e 's/\[commit\/\]/'$commitID'/g' \
  -e 's/\[commitOriginDate\/\]/'$openXcomDate'/g' \
  -e 's/\[commitOrigin\/\]/'$openXcomID'/g' \
  ../Extended.txt > OpenXcomExForum.txt
