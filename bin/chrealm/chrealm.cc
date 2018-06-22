// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "garnet/bin/chrealm/chrealm.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <fcntl.h>
#include <unistd.h>

#include <lib/fdio/namespace.h>
#include <lib/fdio/spawn.h>
#include <zircon/compiler.h>
#include <zircon/device/vfs.h>
#include <zircon/status.h>
#include <zircon/syscalls.h>
#include <zircon/types.h>

#include "lib/fxl/functional/auto_call.h"
#include "lib/fxl/strings/concatenate.h"

namespace chrealm {

zx_status_t RunBinaryInRealm(const std::string& realm_path,
                             const char** argv, int64_t* return_code) {
  *return_code = -1;
  fdio_ns_t* ns = nullptr;
  fdio_flat_namespace_t* flat_ns = nullptr;
  auto cleanup = fxl::MakeAutoCall([&]() {
    if (ns != nullptr) {
      fdio_ns_destroy(ns);
    }
    free(flat_ns);
  });

  // Get the process's namespace.
  zx_status_t status = fdio_ns_get_installed(&ns);
  if (status != ZX_OK) {
    fprintf(stderr, "error: could not obtain namespace\n");
    return status;
  }

  // Open the services dir in the provided path, which points to a realm's
  // /hub directory.
  zx_handle_t realm_svc_dir = ZX_HANDLE_INVALID;
  const std::string svc_path = fxl::Concatenate({realm_path, "/svc"});
  status = fdio_ns_open(ns, svc_path.c_str(), ZX_FS_RIGHT_READABLE,
                        &realm_svc_dir);
  if (status != ZX_OK) {
    fprintf(stderr, "error: could not open svc in realm: %s\n",
            svc_path.c_str());
    return status;
  }

  // TODO(geb): Consider if we should replace /hub as well.
  // TODO(geb): Spawn the process under the realm's job.

  // Convert 'ns' to a flat namespace and replace /svc with 'realm_svc_dir'.
  status = fdio_ns_export(ns, &flat_ns);
  if (status != ZX_OK) {
    fprintf(stderr, "error: could not flatten namespace\n");
    return status;
  }
  fdio_spawn_action_t actions[flat_ns->count];
  for (size_t i = 0; i < flat_ns->count; ++i) {
    zx_handle_t handle;
    if (std::string(flat_ns->path[i]) == "/svc") {
      handle = realm_svc_dir;
    } else {
      handle = flat_ns->handle[i];
    }
    fdio_spawn_action_t add_ns_entry = {
        .action = FDIO_SPAWN_ACTION_ADD_NS_ENTRY,
        .ns = {
            .prefix = flat_ns->path[i],
            .handle = handle,
        },
    };
    actions[i] = add_ns_entry;
  }

  // Launch the binary.
  const char* binary_path = argv[0];
  const uint32_t flags = FDIO_SPAWN_CLONE_ALL & ~FDIO_SPAWN_CLONE_NAMESPACE;

  char err_msg[FDIO_SPAWN_ERR_MSG_MAX_LENGTH];
  zx_handle_t proc = ZX_HANDLE_INVALID;
  status = fdio_spawn_etc(ZX_HANDLE_INVALID, flags, binary_path, argv,
                          nullptr, countof(actions), actions, &proc, err_msg);
  if (status != ZX_OK) {
    fprintf(stderr, "error: failed to launch command: %s\n", err_msg);
    return status;
  }

  status = zx_object_wait_one(proc, ZX_PROCESS_TERMINATED, ZX_TIME_INFINITE,
                              nullptr);
  if (status != ZX_OK) {
    fprintf(stderr, "error: could not wait for command\n");
    return status;
  }
  zx_info_process_t info;
  status = zx_object_get_info(proc, ZX_INFO_PROCESS, &info, sizeof(info),
                              nullptr, nullptr);
  if (status != ZX_OK) {
    fprintf(stderr, "error: could not get result of command\n");
    return status;
  }
  *return_code = info.return_code;
  return ZX_OK;
}

}  // namespace chrealm