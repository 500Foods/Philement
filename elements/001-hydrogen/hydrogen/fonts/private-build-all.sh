#! /bin/bash

npm run build -- --jCmd=8 woff2::AcuranzoSans
nm run build -- --jCmd=8 woff2::AcuranzoSansFancy
npm run build -- --jCmd=8 woff2::AcuranzoMono 
npm run build -- --jCmd=8 woff2::AcuranzoMonoFancy
npm run build -- --jCmd=8 woff2-unhinted::AcuranzoSansMail
npm run build -- --jCmd=8 woff2-unhinted::AcuranzoMonoMail