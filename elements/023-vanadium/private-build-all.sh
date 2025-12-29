#! /bin/bash

rm -rf dist/ .verda-cache/ node_modules/.cache/ .build/
npm cache clean --force

# Note: TTF files will be built as a pre-requisite for the WOFF2 files
# Note: jCmd=8 will limit the build to 8 cores and also limit memory usage
# Note: This is a very CPU and RAM-intensive build, taking potentially hours

npm run build -- --jCmd=8 woff2::VanadiumSans
npm run build -- --jCmd=8 woff2::VanadiumSansFancy
npm run build -- --jCmd=8 woff2::VanadiumMono 
npm run build -- --jCmd=8 woff2::VanadiumMonoFancy
npm run build -- --jCmd=8 woff2-unhinted::VanadiumSansMail
npm run build -- --jCmd=8 woff2-unhinted::VanadiumMonoMail
