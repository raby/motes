// motes — connected-components / blob detection in the browser (and Node), via WebAssembly.
//
// Public API:
//   label(mask, width, height, options?)  — label a raw binary mask
//   labelImageData(image, options?)       — threshold an RGBA image, then label it
//
// The WASM module loads once, lazily, and is cached. Both functions are async.
import createMotes from './motes.mjs'

let modulePromise = null
function moduleReady() {
  if (modulePromise === null) modulePromise = createMotes()
  return modulePromise
}

/**
 * Label the foreground of a binary mask into connected blobs.
 *
 * @param {Uint8Array} mask  width*height bytes, row-major; any non-zero byte is foreground.
 * @param {number} width
 * @param {number} height
 * @param {{connectivity?: 4|8}} [options]  pixel connectivity (default 8).
 * @returns {Promise<Array<{label:number,minX:number,minY:number,maxX:number,maxY:number,area:number,centroidX:number,centroidY:number}>>}
 */
export async function label(mask, width, height, options = {}) {
  const connectivity = options.connectivity === 4 ? 4 : 8
  const n = width * height
  if (!mask || typeof mask.length !== 'number' || mask.length < n) {
    throw new RangeError(
      `motes.label: mask needs at least ${n} bytes for ${width}x${height}, got ${mask ? mask.length : 'nothing'}`,
    )
  }
  if (n === 0) return []

  const Module = await moduleReady()
  const ptr = Module._malloc(n)
  try {
    Module.HEAPU8.set(mask.subarray(0, n), ptr)
    return Module.labelMask(ptr, width, height, connectivity)
  } finally {
    Module._free(ptr)
  }
}

/**
 * Threshold an RGBA image to a binary mask (luma >= threshold is foreground), then label it.
 *
 * @param {{data: Uint8Array|Uint8ClampedArray, width: number, height: number}} image  e.g. a canvas ImageData.
 * @param {{threshold?: number, connectivity?: 4|8}} [options]  luma threshold 0..255 (default 128).
 * @returns {Promise<Array>}  same blob shape as `label`.
 */
export async function labelImageData(image, options = {}) {
  const { data, width, height } = image
  const threshold = options.threshold ?? 128
  const mask = new Uint8Array(width * height)
  for (let i = 0, p = 0; i < mask.length; i++, p += 4) {
    const luma = 0.299 * data[p] + 0.587 * data[p + 1] + 0.114 * data[p + 2]
    mask[i] = luma >= threshold ? 255 : 0
  }
  return label(mask, width, height, options)
}
