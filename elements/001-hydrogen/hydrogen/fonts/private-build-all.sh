#! /bin/bash

rm -rf dist/ .verda-cache/ node_modules/.cache/ .build/
npm cache clean --force

npm run build -- --jCmd=8 woff2::AcuranzoSans
npm run build -- --jCmd=8 woff2::AcuranzoSansFancy
npm run build -- --jCmd=8 woff2::AcuranzoMono 
npm run build -- --jCmd=8 woff2::AcuranzoMonoFancy
npm run build -- --jCmd=8 woff2-unhinted::AcuranzoSansMail
npm run build -- --jCmd=8 woff2-unhinted::AcuranzoMonoMail
