// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! The device layer.

pub mod arp;
pub mod ethernet;

use std::collections::HashMap;
use std::fmt::{self, Debug, Display, Formatter};

use device::ethernet::EthernetDeviceState;
use ip::{IpAddr, Subnet};
use wire::SerializationCallback;
use StackState;

/// An ID identifying a device.
#[derive(Copy, Clone)]
pub struct DeviceId {
    id: u64,
    protocol: DeviceProtocol,
}

impl DeviceId {
    fn new_ethernet(id: u64) -> DeviceId {
        DeviceId {
            id,
            protocol: DeviceProtocol::Ethernet,
        }
    }
}

impl Display for DeviceId {
    fn fmt(&self, f: &mut Formatter) -> Result<(), fmt::Error> {
        write!(f, "{}:{}", self.protocol, self.id)
    }
}

impl Debug for DeviceId {
    fn fmt(&self, f: &mut Formatter) -> Result<(), fmt::Error> {
        Display::fmt(self, f)
    }
}

#[derive(Copy, Clone)]
enum DeviceProtocol {
    Ethernet,
}

impl Display for DeviceProtocol {
    fn fmt(&self, f: &mut Formatter) -> Result<(), fmt::Error> {
        write!(
            f,
            "{}",
            match self {
                DeviceProtocol::Ethernet => "Ethernet",
            }
        )
    }
}

/// The state associated with the device layer.
#[derive(Default)]
pub struct DeviceLayerState {
    // Invariant: even though each protocol has its own hash map, IDs (used as
    // keys in the hash maps) are unique across all hash maps. This is
    // guaranteed by allocating IDs sequentially, and never re-using an ID
    // (which is a requirement of the FIDL API anyway).
    next_id: u64,
    ethernet: HashMap<u64, EthernetDeviceState>,
}

impl DeviceLayerState {
    /// Add a new ethernet device to the device layer.
    pub fn add_ethernet_device(&mut self, state: EthernetDeviceState) -> DeviceId {
        let id = self.allocate_id();
        self.ethernet.insert(id, state);
        debug!("adding Ethernet device with ID {}", id);
        DeviceId::new_ethernet(id)
    }

    fn allocate_id(&mut self) -> u64 {
        let id = self.next_id;
        self.next_id += 1;
        id
    }
}

/// Send an IP packet in a device layer frame.
///
/// `send_ip_frame` accepts a device ID, a local IP address, and a callback. It
/// computes the routing information and invokes the callback with the number of
/// prefix bytes required by all encapsulating headers, and the minimum size of
/// the body plus padding. The callback is expected to return a byte buffer and
/// a range which corresponds to the desired body. The portion of the buffer
/// beyond the end of the body range will be treated as padding. The total
/// number of bytes in the body and the post-body padding must not be smaller
/// than the minimum size passed to the callback.
///
/// For more details on the callback, see the [`::wire::SerializationCallback`]
/// documentation.
///
/// # Panics
///
/// `send_ip_frame` panics if the buffer returned from `get_buffer` does not
/// have sufficient space preceding the body for all encapsulating headers or
/// does not have enough body plus padding bytes to satisfy the requirement
/// passed to the callback.
pub fn send_ip_frame<A, B, F>(
    state: &mut StackState, device: DeviceId, local_addr: A, get_buffer: F,
) where
    A: IpAddr,
    B: AsRef<[u8]> + AsMut<[u8]>,
    F: SerializationCallback<B>,
{
    match device.protocol {
        DeviceProtocol::Ethernet => {
            self::ethernet::send_ip_frame(state, device.id, local_addr, get_buffer)
        }
    }
}

/// Receive a device layer frame from the network.
pub fn receive_frame(state: &mut StackState, device: DeviceId, bytes: &mut [u8]) {
    match device.protocol {
        DeviceProtocol::Ethernet => self::ethernet::receive_frame(state, device.id, bytes),
    }
}

/// Get the IP address and subnet associated with this device.
pub fn get_ip_addr<A: IpAddr>(state: &mut StackState, device: DeviceId) -> Option<(A, Subnet<A>)> {
    match device.protocol {
        DeviceProtocol::Ethernet => self::ethernet::get_ip_addr(state, device.id),
    }
}

/// Set the IP address and subnet associated with this device.
pub fn set_ip_addr<A: IpAddr>(
    state: &mut StackState, device: DeviceId, addr: A, subnet: Subnet<A>,
) {
    assert!(subnet.contains(addr));
    match device.protocol {
        DeviceProtocol::Ethernet => self::ethernet::set_ip_addr(state, device.id, addr, subnet),
    }
}
