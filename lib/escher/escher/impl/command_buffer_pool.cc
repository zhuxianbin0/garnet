// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/impl/command_buffer_pool.h"

#include "escher/impl/vulkan_utils.h"

namespace escher {
namespace impl {

CommandBufferPool::CommandBufferPool(vk::Device device,
                                     vk::Queue queue,
                                     uint32_t queue_family_index)
    : device_(device), queue_(queue) {
  FTL_DCHECK(device);
  FTL_DCHECK(queue);
  vk::CommandPoolCreateInfo info;
  info.flags = vk::CommandPoolCreateFlagBits::eTransient |
               vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  info.queueFamilyIndex = queue_family_index;
  pool_ = ESCHER_CHECKED_VK_RESULT(device_.createCommandPool(info));
}

CommandBufferPool::~CommandBufferPool() {
  Cleanup();
  if (!pending_buffers_.empty()) {
    // We didn't call waitIdle() above to avoid unnecessary blocking: there
    // may be other pools with pending buffers, and there is no need to wait
    // for them to finish if the initial call to Cleanup() successfully returns
    // all buffers to the free list.
    device_.waitIdle();
    Cleanup();
  }
  FTL_DCHECK(pending_buffers_.empty());
  std::vector<vk::CommandBuffer> buffers_to_free;
  buffers_to_free.reserve(free_buffers_.size());
  while (!free_buffers_.empty()) {
    auto& buf = free_buffers_.front();
    buffers_to_free.push_back(buf->get());
    device_.destroyFence(buf->fence());
    free_buffers_.pop();
  }
  device_.freeCommandBuffers(pool_,
                             static_cast<uint32_t>(buffers_to_free.size()),
                             buffers_to_free.data());
  device_.destroyCommandPool(pool_);
}

CommandBuffer* CommandBufferPool::GetCommandBuffer() {
  // TODO: perhaps do when buffer is submitted?
  Cleanup();

  // Find an existing CommandBuffer for reuse, or create a new one.
  CommandBuffer* buffer = nullptr;
  if (free_buffers_.empty()) {
    vk::CommandBufferAllocateInfo info;
    info.commandPool = pool_;
    info.level = vk::CommandBufferLevel::ePrimary;
    info.commandBufferCount = 1;
    auto allocated_vulkan_buffers =
        ESCHER_CHECKED_VK_RESULT(device_.allocateCommandBuffers(info));

    vk::Fence fence =
        ESCHER_CHECKED_VK_RESULT(device_.createFence(vk::FenceCreateInfo()));

    buffer = new CommandBuffer(device_, allocated_vulkan_buffers[0], fence);
    pending_buffers_.push(std::unique_ptr<CommandBuffer>(buffer));
  } else {
    buffer = free_buffers_.front().get();
    pending_buffers_.push(std::move(free_buffers_.front()));
    free_buffers_.pop();
  }
  buffer->Begin();
  return buffer;
}

void CommandBufferPool::Cleanup() {
  // TODO: add some guard against potential re-entrant calls resulting from
  // invocation of CommandBufferFinishedCallbacks.

  while (!pending_buffers_.empty()) {
    auto& buffer = pending_buffers_.front();
    if (buffer->Retire()) {
      free_buffers_.push(std::move(pending_buffers_.front()));
      pending_buffers_.pop();
    } else {
      // The first buffer in the queue is not finished, so neither are the rest.
      // TODO: this is not necessarily true, but it should be close enough as
      // long as everyone is being a "good citizen" (e.g. by submitting buffers
      // in a timely fashion.)
      break;
    }
  }
}

}  // namespace impl
}  // namespace escher
