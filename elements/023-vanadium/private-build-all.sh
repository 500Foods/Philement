#! /bin/bash

rm -rf dist/ .verda-cache/ node_modules/.cache/ .build/
npm cache clean --force

npm run build -- --jCmd=8 woff2::VanadiumSans
npm run build -- --jCmd=8 woff2::VanadiumSansFancy
npm run build -- --jCmd=8 woff2::VanadiumMono 
npm run build -- --jCmd=8 woff2::VanadiumMonoFancy
npm run build -- --jCmd=8 woff2-unhinted::VanadiumSansMail
npm run build -- --jCmd=8 woff2-unhinted::VanadiumMonoMail
