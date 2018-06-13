// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "garnet/drivers/bluetooth/lib/common/device_address.h"
#include "garnet/drivers/bluetooth/lib/common/uint128.h"
#include "garnet/drivers/bluetooth/lib/sm/smp.h"

namespace btlib {
namespace sm {
namespace util {

// Used to select the key generation method as described in Vol 3, Part H,
// 2.3.5.1 based on local and peer authentication parameters:
//   - |secure_connections|: True if Secure Connections pairing is used. False
//     means Legacy Pairing.
//   - |local_oob|: Local OOB auth data is available.
//   - |peer_oob|: Peer OOB auth data is available.
//   - |mitm_required|: True means at least one of the devices requires MITM
//     protection.
//   - |local_ioc|, |peer_ioc|: Indicate local and peer IO capabilities.
PairingMethod SelectPairingMethod(bool secure_connections, bool local_oob,
                                  bool peer_oob, bool mitm_required,
                                  IOCapability local_ioc,
                                  IOCapability peer_ioc);

// Implements the "Security Function 'e'" defined in Vol 3, Part H, 2.2.1.
void Encrypt(const common::UInt128& key, const common::UInt128& plaintext_data,
             common::UInt128* out_encrypted_data);

// Implements the "Confirm Value Generation" or "c1" function for LE Legacy
// Pairing described in Vol 3, Part H, 2.2.3.
//
//   |tk|: 128-bit TK value
//   |rand|: 128-bit random number
//   |preq|: 56-bit SMP "Pairing Request" PDU
//   |pres|: 56-bit SMP "Pairing Response" PDU
//   |initiator_addr|: Device address of the initiator used while establishing
//                     the connection.
//   |responder_addr|: Device address of the responder used while establishing
//                     the connection.
//
// The generated confirm value will be returned in |out_confirm_value|.
void C1(const common::UInt128& tk, const common::UInt128& rand,
        const common::ByteBuffer& preq, const common::ByteBuffer& pres,
        const common::DeviceAddress& initiator_addr,
        const common::DeviceAddress& responder_addr,
        common::UInt128* out_confirm_value);

}  // namespace util
}  // namespace sm
}  // namespace btlib
