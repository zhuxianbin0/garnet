// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GARNET_BIN_GUEST_GUEST_VIEW_H_
#define GARNET_BIN_GUEST_GUEST_VIEW_H_

#include <lib/async/dispatcher.h>
#include <zircon/types.h>

#include <fuchsia/cpp/views_v1.h>
#include "garnet/lib/machina/gpu_scanout.h"
#include "garnet/lib/machina/input_dispatcher.h"
#include "garnet/lib/machina/virtio_gpu.h"
#include "lib/app/cpp/application_context.h"
#include "lib/fidl/cpp/binding_set.h"
#include "lib/fxl/macros.h"
#include "lib/fxl/tasks/task_runner.h"
#include "lib/ui/scenic/client/host_memory.h"
#include "lib/ui/scenic/client/resources.h"
#include "lib/ui/view_framework/base_view.h"

// For now we expose a fixed size display to the guest. Scenic will scale this
// buffer to the actual window size on the host.
// TODO(PD-109): Support resizing the display.
// TODO(PD-108): Support resizing the input to match the display.
static constexpr uint32_t kGuestViewDisplayWidth = 1024;
static constexpr uint32_t kGuestViewDisplayHeight = 768;

class GuestView;

class ScenicScanout : public machina::GpuScanout,
                      public views_v1::ViewProvider {
 public:
  static zx_status_t Create(component::ApplicationContext* application_context,
                            machina::InputDispatcher* input_dispatcher,
                            fbl::unique_ptr<GpuScanout>* out);

  ScenicScanout(component::ApplicationContext* application_context,
                machina::InputDispatcher* input_dispatcher);

  // |GpuScanout|
  void InvalidateRegion(const machina::GpuRect& rect) override;

  // |ViewProvider|
  void CreateView(
      fidl::InterfaceRequest<views_v1_token::ViewOwner> view_owner_request,
      fidl::InterfaceRequest<component::ServiceProvider> view_services)
      override;

 private:
  machina::InputDispatcher* input_dispatcher_;
  component::ApplicationContext* application_context_;
  fidl::BindingSet<ViewProvider> bindings_;
  fbl::unique_ptr<GuestView> view_;
};

class GuestView : public mozart::BaseView {
 public:
  GuestView(
      machina::GpuScanout* scanout,
      machina::InputDispatcher* input_dispatcher,
      views_v1::ViewManagerPtr view_manager,
      fidl::InterfaceRequest<views_v1_token::ViewOwner> view_owner_request);

  ~GuestView() override;

 private:
  // |BaseView|:
  void OnSceneInvalidated(images::PresentationInfo presentation_info) override;
  bool OnInputEvent(input::InputEvent event) override;

  scenic_lib::ShapeNode background_node_;
  scenic_lib::Material material_;
  images::ImageInfo image_info_;
  fbl::unique_ptr<scenic_lib::HostMemory> memory_;

  machina::InputDispatcher* input_dispatcher_;

  float pointer_scale_x_ = 0.0f;
  float pointer_scale_y_ = 0.0f;
  bool view_ready_ = false;

  FXL_DISALLOW_COPY_AND_ASSIGN(GuestView);
};

#endif  // GARNET_BIN_GUEST_GUEST_VIEW_H_
