// Type definitions for motes (WebAssembly connected-components / blob detection).

/** One connected region ("mote"). */
export interface Blob {
  /** 1-based label, in raster order of first encounter. */
  label: number
  /** Inclusive bounding box. */
  minX: number
  minY: number
  maxX: number
  maxY: number
  /** Pixel count. */
  area: number
  /** Area-weighted centroid. */
  centroidX: number
  centroidY: number
}

export interface LabelOptions {
  /** Pixel connectivity (default 8). */
  connectivity?: 4 | 8
}

export interface ImageLike {
  data: Uint8Array | Uint8ClampedArray
  width: number
  height: number
}

export interface LabelImageOptions extends LabelOptions {
  /** Luma threshold 0..255; a pixel is foreground when its luma is >= this (default 128). */
  threshold?: number
}

/**
 * Label the foreground of a binary mask (width*height bytes, row-major; any non-zero byte is
 * foreground) into connected blobs.
 */
export function label(
  mask: Uint8Array,
  width: number,
  height: number,
  options?: LabelOptions,
): Promise<Blob[]>

/** Threshold an RGBA image to a mask (luma >= threshold), then label it. */
export function labelImageData(image: ImageLike, options?: LabelImageOptions): Promise<Blob[]>
