// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GARNET_BIN_UI_VIEW_MANAGER_INTERNAL_VIEW_INSPECTOR_H_
#define GARNET_BIN_UI_VIEW_MANAGER_INTERNAL_VIEW_INSPECTOR_H_

#include <functional>
#include <vector>

#include <fuchsia/math/cpp/fidl.h>
#include <input/cpp/fidl.h>
#include <views_v1/cpp/fidl.h>
#include <views_v1_token/cpp/fidl.h>

namespace view_manager {
class InputConnectionImpl;
class InputDispatcherImpl;

// |FocusChain| defines the chain that a keyboard input event will follow.
struct FocusChain {
 public:
  // |version| of the focus chain.
  uint64_t version;

  // |chain| is the ordered list of views that a keyboard event will propagate
  std::vector<views_v1_token::ViewToken> chain;
};

// Provides information about a view which was hit during a hit tests.
struct ViewHit {
  // The view which was hit.
  views_v1_token::ViewToken view_token;

  // The origin of the ray that was used for the hit test, in device
  // coordinates.
  fuchsia::math::Point3F ray_origin;

  // The direction of the ray that was used for the hit test, in device
  // coordinates.
  fuchsia::math::Point3F ray_direction;

  // The distance along the ray that was passed in to the hit test, in the
  // coordinate system of the view.
  float distance;

  // Transforms the view tree coordinate system to the view's coordinate system.
  fuchsia::math::Transform inverse_transform;
};

// Provides a view associate with the ability to inspect and perform operations
// on the contents of views and view trees.
class ViewInspector {
 public:
  using HitTestCallback = std::function<void(fidl::VectorPtr<ViewHit>)>;
  using ResolveFocusChainCallback =
      std::function<void(std::unique_ptr<FocusChain>)>;
  using ActivateFocusChainCallback =
      std::function<void(std::unique_ptr<FocusChain>)>;
  using HasFocusCallback = std::function<void(bool)>;
  using OnEventDelivered = std::function<void(bool handled)>;

  virtual ~ViewInspector() {}

  // Performs a hit test using a vector with the provided ray, and returns the
  // list of views which were hit.
  virtual void HitTest(views_v1::ViewTreeToken view_tree_token,
                       const fuchsia::math::Point3F& ray_origin,
                       const fuchsia::math::Point3F& ray_direction,
                       HitTestCallback callback) = 0;

  // Given a token for a view tree, retrieve the current active focus chain for
  // this view tree.
  virtual void ResolveFocusChain(views_v1::ViewTreeToken view_tree_token,
                                 const ResolveFocusChainCallback& callback) = 0;

  // TODO(jpoichet) Move this
  // Set the current input focus to the provided |view_token|.
  // This is a back channel from input_manager to view_manager to swap focus
  // on touch down events. This logic should be moved in the future
  virtual void ActivateFocusChain(
      views_v1_token::ViewToken view_token,
      const ActivateFocusChainCallback& callback) = 0;

  // Returns whether view has focus
  virtual void HasFocus(views_v1_token::ViewToken view_token,
                        const HasFocusCallback& callback) = 0;

  // Retrieve the SoftKeyboardContainer that is the closest to the ViewToken
  // in the associated ViewTree
  virtual void GetSoftKeyboardContainer(
      views_v1_token::ViewToken view_token,
      fidl::InterfaceRequest<input::SoftKeyboardContainer> container) = 0;

  // Retrieve the IME Service that is the closest to the ViewToken
  // in the associated ViewTree
  virtual void GetImeService(
      views_v1_token::ViewToken view_token,
      fidl::InterfaceRequest<input::ImeService> ime_service) = 0;
};

}  // namespace view_manager

#endif  // GARNET_BIN_UI_VIEW_MANAGER_INTERNAL_VIEW_INSPECTOR_H_
