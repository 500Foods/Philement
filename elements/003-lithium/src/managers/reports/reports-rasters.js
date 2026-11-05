/**
 * Display PNGs for Preview. The script receives params.rasters and
 * does not call image_scale. Import converts a file once, through
 * image_scale, and stores that canonical bitmap.
 */

import { fitRect, rasterJobs } from './model/images.js';

const PNG_MAGIC = [0x89, 0x50, 0x4e, 0x47];

function bytesOf(buffer) {
  return buffer instanceof Uint8Array ? buffer : new Uint8Array(buffer);
}

function readU16(bytes, offset) {
  return bytes[offset] | (bytes[offset + 1] << 8);
}

function readU32(bytes, offset) {
  return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
}

/**
 * Largest ICO frame. PNG frames stay PNG. DIB frames gain a BMP header.
 * @param {Uint8Array} bytes
 * @returns {{ bytes: Uint8Array, mime: string }|null}
 */
export function largestIcoFrame(bytes) {
  const data = bytesOf(bytes);
  if (data.length < 6 || readU16(data, 0) !== 0 || readU16(data, 2) !== 1) return null;
  const count = readU16(data, 4);
  let best = null;
  for (let index = 0; index < count; index += 1) {
    const at = 6 + index * 16;
    if (at + 16 > data.length) return null;
    const width = data[at] === 0 ? 256 : data[at];
    const height = data[at + 1] === 0 ? 256 : data[at + 1];
    const size = readU32(data, at + 8);
    const offset = readU32(data, at + 12);
    if (offset + size > data.length) return null;
    const area = width * height;
    if (!best || area > best.area || (area === best.area && size > best.size)) {
      best = {
        area, size, width, height, offset, bytes: data.slice(offset, offset + size),
      };
    }
  }
  if (!best) return null;
  const png = PNG_MAGIC.every((byte, index) => best.bytes[index] === byte);
  if (png) return { bytes: best.bytes, mime: 'image/png', width: best.width, height: best.height };
  return { bytes: dibToBmp(best.bytes), mime: 'image/bmp', width: best.width, height: best.height };
}

function dibToBmp(dib) {
  const header = dib.length >= 4 ? readU32(dib, 0) : 40;
  let palette = 0;
  if (header >= 16 && dib.length >= 16) {
    const bitCount = readU16(dib, 14);
    if (bitCount > 0 && bitCount <= 8 && dib.length >= 36) {
      const colors = readU32(dib, 32);
      palette = (colors === 0 ? 2 ** bitCount : colors) * 4;
    }
  }
  const pixelOffset = 14 + header + palette;
  const file = new Uint8Array(14 + dib.length);
  file[0] = 0x42;
  file[1] = 0x4d;
  const view = new DataView(file.buffer);
  view.setUint32(2, file.length, true);
  view.setUint32(10, pixelOffset, true);
  file.set(dib, 14);
  return file;
}

function base64(bytes) {
  let text = '';
  const data = bytesOf(bytes);
  for (let index = 0; index < data.length; index += 1) text += String.fromCharCode(data[index]);
  return btoa(text);
}

function dataUri(mime, bytes) {
  return `data:${mime};base64,${base64(bytes)}`;
}

async function fileBytes(file) {
  return bytesOf(await file.arrayBuffer());
}

function loadImage(src) {
  return new Promise((resolve, reject) => {
    const img = new Image();
    img.onload = () => resolve(img);
    img.onerror = () => reject(new Error('Image could not be decoded.'));
    img.src = src;
  });
}

/**
 * One display PNG. Cover is cropped to the canvas. Grayscale and
 * invert are baked. Flip and rotation are not.
 * @param {object} job
 * @returns {Promise<string>}
 */
export async function drawRaster(job) {
  const img = await loadImage(job.src);
  const canvas = document.createElement('canvas');
  canvas.width = job.pxW;
  canvas.height = job.pxH;
  const ctx = canvas.getContext('2d', { willReadFrequently: true });
  if (!ctx) throw new Error('Canvas is not available.');
  const rect = fitRect(img.naturalWidth || img.width, img.naturalHeight || img.height, job.pxW, job.pxH, job.fit, job.fitAlign, job.fitValign);
  ctx.drawImage(img, rect.dx, rect.dy, rect.dw, rect.dh);
  if (job.grayscale || job.invert) {
    const frame = ctx.getImageData(0, 0, job.pxW, job.pxH);
    const pixels = frame.data;
    for (let index = 0; index < pixels.length; index += 4) {
      let red = pixels[index];
      let green = pixels[index + 1];
      let blue = pixels[index + 2];
      if (job.grayscale) {
        const gray = 0.2126 * red + 0.7152 * green + 0.0722 * blue;
        red = gray;
        green = gray;
        blue = gray;
      }
      if (job.invert) {
        red = 255 - red;
        green = 255 - green;
        blue = 255 - blue;
      }
      pixels[index] = red;
      pixels[index + 1] = green;
      pixels[index + 2] = blue;
    }
    ctx.putImageData(frame, 0, 0);
  }
  return canvas.toDataURL('image/png');
}

/**
 * Map of raster key to `{ data }`. Empty when the report has no bitmap.
 * @param {object} doc
 * @param {(job: object) => Promise<string>} [draw]
 * @returns {Promise<object>}
 */
export async function buildRasterMap(doc, draw) {
  const jobs = rasterJobs(doc);
  if (jobs.length === 0) return {};
  const paint = draw || drawRaster;
  const map = {};
  for (const job of jobs) {
    const data = await paint(job);
    if (typeof data === 'string' && data !== '') map[job.key] = { data };
  }
  return map;
}

async function postScale(post, bytes, width, height, format) {
  const body = await post('reporting/image_scale', {
    image: base64(bytes),
    format,
    width,
    height,
    units: 'px',
    dpi: 72,
  });
  if (!body || typeof body.image !== 'string' || body.image === '') {
    throw new Error('Image scale did not return a bitmap.');
  }
  return body;
}

/**
 * Canonical data URI. SVG is stored as text. A bitmap goes through
 * image_scale, twice when EXIF orientation changes the sides.
 * @param {Function|null} post
 * @param {File|Blob} file
 * @returns {Promise<string>}
 */
export async function canonicalImage(post, file) {
  const name = typeof file.name === 'string' ? file.name.toLowerCase() : '';
  const mime = typeof file.type === 'string' ? file.type.toLowerCase() : '';
  const bytes = await fileBytes(file);
  if (mime === 'image/svg+xml' || name.endsWith('.svg')) {
    const text = new TextDecoder().decode(bytes);
    return `data:image/svg+xml;charset=utf-8,${encodeURIComponent(text)}`;
  }
  if (typeof post !== 'function') throw new Error('Image scale is not available.');
  const jpeg = mime === 'image/jpeg' || name.endsWith('.jpg') || name.endsWith('.jpeg');
  let payload = bytes;
  let requestFormat = jpeg ? 'jpg' : 'png';
  let decodeMime = jpeg ? 'image/jpeg' : (mime || 'application/octet-stream');
  let fallback = null;
  const icon = mime === 'image/x-icon' || mime === 'image/vnd.microsoft.icon' || name.endsWith('.ico');
  if (icon) {
    const frame = largestIcoFrame(bytes);
    if (!frame) throw new Error('Image file is not a readable icon.');
    payload = frame.bytes;
    requestFormat = 'png';
    decodeMime = frame.mime;
    fallback = { width: frame.width, height: frame.height };
  }
  let width = fallback ? fallback.width : 0;
  let height = fallback ? fallback.height : 0;
  try {
    const decoded = await loadImage(dataUri(decodeMime, payload));
    width = decoded.naturalWidth || decoded.width || width;
    height = decoded.naturalHeight || decoded.height || height;
  } catch (error) {
    if (!width || !height) throw error;
  }
  if (!width || !height) throw new Error('Image has no pixels.');
  let scaled = await postScale(post, payload, width, height, requestFormat);
  const dims = scaled.input_dimensions;
  if (dims && (dims.width !== width || dims.height !== height) && dims.width > 0 && dims.height > 0) {
    scaled = await postScale(post, payload, dims.width, dims.height, requestFormat);
  }
  const outMime = requestFormat === 'jpg' ? 'image/jpeg' : 'image/png';
  return `data:${scaled.mime_type || outMime};base64,${scaled.image}`;
}
