// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/layer_tree.h"

#include "flutter/flow/frame_timings.h"
#include "flutter/flow/layers/layer.h"
#include "flutter/fml/time/time_point.h"
#include "flutter/fml/trace_event.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "third_party/skia/include/utils/SkNWayCanvas.h"

namespace flutter {

LayerTree::LayerTree(const SkISize& frame_size, float device_pixel_ratio)
    : frame_size_(frame_size),
      device_pixel_ratio_(device_pixel_ratio),
      rasterizer_tracing_threshold_(0),
      checkerboard_raster_cache_images_(false),
      checkerboard_offscreen_layers_(false) {
  FML_CHECK(device_pixel_ratio_ != 0.0f);
}

bool LayerTree::Preroll(CompositorContext::ScopedFrame& frame,
                        bool ignore_raster_cache,
                        SkRect cull_rect) {
  TRACE_EVENT0("flutter", "LayerTree::Preroll");

  if (!root_layer_) {
    FML_LOG(ERROR) << "The scene did not specify any layers.";
    return false;
  }

  SkColorSpace* color_space =
      frame.canvas() ? frame.canvas()->imageInfo().colorSpace() : nullptr;
  frame.context().raster_cache().SetCheckboardCacheImages(
      checkerboard_raster_cache_images_);
  MutatorsStack stack;
  PrerollContext context = {
      ignore_raster_cache ? nullptr : &frame.context().raster_cache(),
      frame.gr_context(),
      frame.view_embedder(),
      stack,
      color_space,
      cull_rect,
      false,
      frame.context().raster_time(),
      frame.context().ui_time(),
      frame.context().texture_registry(),
      checkerboard_offscreen_layers_,
      device_pixel_ratio_};

  root_layer_->Preroll(&context, frame.root_surface_transformation());
  return context.surface_needs_readback;
}

void LayerTree::Paint(CompositorContext::ScopedFrame& frame,
                      bool ignore_raster_cache) const {
  TRACE_EVENT0("flutter", "LayerTree::Paint");

  if (!root_layer_) {
    FML_LOG(ERROR) << "The scene did not specify any layers to paint.";
    return;
  }

  SkISize canvas_size = frame.canvas()->getBaseLayerSize();
  SkNWayCanvas internal_nodes_canvas(canvas_size.width(), canvas_size.height());
  internal_nodes_canvas.addCanvas(frame.canvas());
  if (frame.view_embedder() != nullptr) {
    auto overlay_canvases = frame.view_embedder()->GetCurrentCanvases();
    for (size_t i = 0; i < overlay_canvases.size(); i++) {
      internal_nodes_canvas.addCanvas(overlay_canvases[i]);
    }
  }

  Layer::PaintContext context = {
      static_cast<SkCanvas*>(&internal_nodes_canvas),
      frame.canvas(),
      frame.gr_context(),
      frame.view_embedder(),
      frame.context().raster_time(),
      frame.context().ui_time(),
      frame.context().texture_registry(),
      ignore_raster_cache ? nullptr : &frame.context().raster_cache(),
      checkerboard_offscreen_layers_,
      device_pixel_ratio_,
      SK_Scalar1,
      frame.display_list_builder(),
  };

  if (root_layer_->needs_painting(context)) {
    root_layer_->Paint(context);
  }
}

sk_sp<SkPicture> LayerTree::Flatten(const SkRect& bounds) {
  TRACE_EVENT0("flutter", "LayerTree::Flatten");

  SkPictureRecorder recorder;
  auto* canvas = recorder.beginRecording(bounds);

  if (!canvas) {
    return nullptr;
  }

  MutatorsStack unused_stack;
  const FixedRefreshRateStopwatch unused_stopwatch;
  TextureRegistry unused_texture_registry;
  SkMatrix root_surface_transformation;
  // No root surface transformation. So assume identity.
  root_surface_transformation.reset();

  PrerollContext preroll_context{
      nullptr,                  // raster_cache (don't consult the cache)
      nullptr,                  // gr_context  (used for the raster cache)
      nullptr,                  // external view embedder
      unused_stack,             // mutator stack
      nullptr,                  // SkColorSpace* dst_color_space
      kGiantRect,               // SkRect cull_rect
      false,                    // layer reads from surface
      unused_stopwatch,         // frame time (dont care)
      unused_stopwatch,         // engine time (dont care)
      unused_texture_registry,  // texture registry (not supported)
      false,                    // checkerboard_offscreen_layers
      device_pixel_ratio_       // ratio between logical and physical
  };

  SkISize canvas_size = canvas->getBaseLayerSize();
  SkNWayCanvas internal_nodes_canvas(canvas_size.width(), canvas_size.height());
  internal_nodes_canvas.addCanvas(canvas);

  Layer::PaintContext paint_context = {
      static_cast<SkCanvas*>(&internal_nodes_canvas),
      canvas,  // canvas
      nullptr,
      nullptr,
      unused_stopwatch,         // frame time (dont care)
      unused_stopwatch,         // engine time (dont care)
      unused_texture_registry,  // texture registry (not supported)
      nullptr,                  // raster cache
      false,                    // checkerboard offscreen layers
      device_pixel_ratio_       // ratio between logical and physical
  };

  // Even if we don't have a root layer, we still need to create an empty
  // picture.
  if (root_layer_) {
    root_layer_->Preroll(&preroll_context, root_surface_transformation);
    // The needs painting flag may be set after the preroll. So check it after.
    if (root_layer_->needs_painting(paint_context)) {
      root_layer_->Paint(paint_context);
    }
  }

  return recorder.finishRecordingAsPicture();
}

}  // namespace flutter
