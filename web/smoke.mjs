// Smoke test for the published API (label + labelImageData) over the WASM build. Confirms the
// browser/Node path produces the same blobs the native C++ tests assert. Run: node smoke.mjs.
import { label, labelImageData } from './index.mjs'

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

// label(): raw binary masks
{
  const { mask, width, height } = fromAscii(['......', '.###..', '.###..', '......'])
  const b = await label(mask, width, height, { connectivity: 8 })
  check(b.length === 1, 'single rectangle -> 1 blob')
  check(b[0].area === 6, 'area 6')
  check(b[0].minX === 1 && b[0].maxX === 3 && b[0].minY === 1 && b[0].maxY === 2, 'bounding box')
  check(Math.abs(b[0].centroidX - 2.0) < 1e-9 && Math.abs(b[0].centroidY - 1.5) < 1e-9, 'centroid (2, 1.5)')
}
{
  const { mask, width, height } = fromAscii(['#.', '.#'])
  check((await label(mask, width, height, { connectivity: 8 })).length === 1, 'diagonal, 8-connectivity -> 1 blob')
  check((await label(mask, width, height, { connectivity: 4 })).length === 2, 'diagonal, 4-connectivity -> 2 blobs')
}
{
  const { mask, width, height } = fromAscii(['#####', '#...#', '#...#', '#####'])
  const b = await label(mask, width, height)
  check(b.length === 1 && b[0].area === 14, 'ring -> 1 blob, area 14 (hole excluded)')
}

// labelImageData(): threshold an RGBA image, then label
{
  const width = 6
  const height = 4
  const data = new Uint8ClampedArray(width * height * 4) // black, transparent
  const white = (x, y) => {
    const p = (y * width + x) * 4
    data[p] = data[p + 1] = data[p + 2] = data[p + 3] = 255
  }
  white(2, 1)
  white(3, 1)
  white(2, 2)
  white(3, 2)
  const b = await labelImageData({ data, width, height }, { threshold: 128 })
  check(b.length === 1 && b[0].area === 4, 'labelImageData: a white 2x2 square -> 1 blob, area 4')
}

// error handling
{
  let threw = false
  try {
    await label(new Uint8Array(2), 4, 4) // too small for 4x4
  } catch (e) {
    threw = e instanceof RangeError
  }
  check(threw, 'label() throws RangeError on an undersized mask')
}

if (failures) {
  console.error(`\n${failures} check(s) failed`)
  process.exit(1)
}
console.log('\nall smoke checks passed — public WASM API matches native')
