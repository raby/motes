// Emscripten/embind binding for motes: expose `labelMask` to JavaScript.
//
// The JS side allocates a width*height byte mask in the WASM heap and passes its pointer; this
// wraps it in a motes::Mask (no copy) and returns a JS array of plain blob objects. Kept tiny on
// purpose — the ergonomic wrapper (malloc/free, ImageData → mask) lives in TypeScript.
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <cstdint>

#include "motes/motes.hpp"

namespace {

emscripten::val label_mask(std::uintptr_t data_ptr, int width, int height, int connectivity) {
  const auto* data = reinterpret_cast<const std::uint8_t*>(data_ptr);
  const motes::Mask mask{data, width, height};
  const motes::Connectivity conn =
      (connectivity == 4) ? motes::Connectivity::Four : motes::Connectivity::Eight;

  emscripten::val out = emscripten::val::array();
  for (const motes::Blob& b : motes::label(mask, conn)) {
    emscripten::val o = emscripten::val::object();
    o.set("label", b.label);
    o.set("minX", b.min_x);
    o.set("minY", b.min_y);
    o.set("maxX", b.max_x);
    o.set("maxY", b.max_y);
    o.set("area", b.area);
    o.set("centroidX", b.centroid_x);
    o.set("centroidY", b.centroid_y);
    out.call<void>("push", o);
  }
  return out;
}

} // namespace

EMSCRIPTEN_BINDINGS(motes_module) {
  emscripten::function("labelMask", &label_mask);
}
