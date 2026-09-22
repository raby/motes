// Node smoke test for the WASM build: load the module, label a few known masks, and check the
// results match what the native C++ tests assert. Run: node web/smoke.mjs  (after web/build.sh).
import createMotes from './build/motes.mjs'

const Module = await createMotes()

// Ergonomic-ish wrapper: copy a mask into the WASM heap, label it, free. (The real npm wrapper in
// TypeScript lands in the next slice; this is just enough to prove the binding works.)
function label(mask, width, height, connectivity = 8) {
  const n = width * height
  const ptr = Module._malloc(n)
  Module.HEAPU8.set(mask.subarray(0, n), ptr)
  const blobs = Module.labelMask(ptr, width, height, connectivity)
  Module._free(ptr)
  return blobs
}

function fromAscii(rows) {
  const height = rows.length
  const width = rows[0].length
  const mask = new Uint8Array(width * height)
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      mask[y * width + x] = rows[y][x] === '#' ? 255 : 0
    }
  }
  return { mask, width, height }
}

let failures = 0
function check(cond, msg) {
  if (cond) {
    console.log('ok:', msg)
  } else {
    console.error('FAIL:', msg)
    failures++
  }
}

{
  const { mask, width, height } = fromAscii(['......', '.###..', '.###..', '......'])
  const b = label(mask, width, height, 8)
  check(b.length === 1, 'single rectangle -> 1 blob')
  check(b[0].area === 6, 'area 6')
  check(b[0].minX === 1 && b[0].maxX === 3 && b[0].minY === 1 && b[0].maxY === 2, 'bounding box')
  check(Math.abs(b[0].centroidX - 2.0) < 1e-9 && Math.abs(b[0].centroidY - 1.5) < 1e-9, 'centroid (2, 1.5)')
}

{
  const { mask, width, height } = fromAscii(['#.', '.#'])
  check(label(mask, width, height, 8).length === 1, 'diagonal, 8-connectivity -> 1 blob')
  check(label(mask, width, height, 4).length === 2, 'diagonal, 4-connectivity -> 2 blobs')
}

{
  const { mask, width, height } = fromAscii(['#####', '#...#', '#...#', '#####'])
  const b = label(mask, width, height, 8)
  check(b.length === 1 && b[0].area === 14, 'ring -> 1 blob, area 14 (hole excluded)')
}

if (failures) {
  console.error(`\n${failures} check(s) failed`)
  process.exit(1)
}
console.log('\nall smoke checks passed — WASM matches native')
