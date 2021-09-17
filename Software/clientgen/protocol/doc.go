/*
Copyright (c) Facebook, Inc. and its affiliates.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/*
Package protocol implements a subset of the PTPv2.1 protocol (IEEE 1588-2019).

Implementation is focused on unicast communications over IPv6 and is sufficient to build unicast PTP server or client.

This package also contains basic management client that can be used to exchange Management Packets
with ptp server.

Additionally it has helpers to work with NIC hardware and software timestamps.

All references throughout the code relate to the IEEE 1588-2019 Standard.
*/
package protocol
