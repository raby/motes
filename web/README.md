# motes

Small, fast, dependency-free **connected-components / blob detection**, compiled to **WebAssembly**.
Give it a binary mask (or an image + a threshold); get back the connected regions, each with a
bounding box, area and centroid. The core is a header-only C++ library — this is its web build.

```sh
npm install @raby/motes
```

## Usage

```js
import { labelImageData, label } from '@raby/motes'

// From a canvas ImageData (thresholded on luma, default 128):
const image = ctx.getImageData(0, 0, canvas.width, canvas.height)
const blobs = await labelImageData(image, { threshold: 128, connectivity: 8 })

for (const b of blobs) {
  // b.label, b.minX, b.minY, b.maxX, b.maxY, b.area, b.centroidX, b.centroidY
  ctx.strokeRect(b.minX, b.minY, b.maxX - b.minX + 1, b.maxY - b.minY + 1)
}

// Or from a raw binary mask (width*height bytes; any non-zero byte is foreground):
const mask = new Uint8Array(width * height)
const blobs2 = await label(mask, width, height, { connectivity: 4 })
```

Both functions are `async` (the WASM module loads once, lazily, then is cached). Everything runs
**in the browser** — no server, no upload.

## API

- `label(mask: Uint8Array, width, height, { connectivity?: 4 | 8 }): Promise<Blob[]>`
- `labelImageData(image: { data, width, height }, { threshold?: 0..255, connectivity?: 4 | 8 }): Promise<Blob[]>`
- `Blob = { label, minX, minY, maxX, maxY, area, centroidX, centroidY }` — the label is 1-based, in
  raster order; the bounding box is inclusive; the centroid is area-weighted.

## Performance

Real-time by design: a 640×480 frame labels in well under a millisecond (see the
[C++ benchmark](https://github.com/raby/motes#performance)). The WASM is ~16 KB.

## Licence

[MIT](LICENSE) © Raby Whyte
