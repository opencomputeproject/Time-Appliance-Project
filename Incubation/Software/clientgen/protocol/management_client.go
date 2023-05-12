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

package protocol

// management client is used to talk to (presumably local) PTP server using Management packets

import (
	"encoding/binary"
	"fmt"
	"io"
)

// MgmtClient talks to ptp server over unix socket
type MgmtClient struct {
	Connection io.ReadWriter
	Sequence   uint16
}

// SendPacket sends packet, incrementing sequence counter
func (c *MgmtClient) SendPacket(packet *Management) error {
	c.Sequence++
	packet.SetSequence(c.Sequence)
	b, err := packet.MarshalBinary()
	if err != nil {
		return err
	}
	return binary.Write(c.Connection, binary.BigEndian, b)
}

// Communicate sends the management the packet, parses response into something usable
func (c *MgmtClient) Communicate(packet *Management) (*Management, error) {
	var err error

	if err := c.SendPacket(packet); err != nil {
		return nil, err
	}
	response := make([]uint8, 1024)
	n, err := c.Connection.Read(response)
	if err != nil {
		return nil, err
	}
	res, err := decodeMgmtPacket(response[:n])
	if err != nil {
		return nil, err
	}
	errorPacket, ok := res.(*ManagementMsgErrorStatus)
	if ok {
		return nil, fmt.Errorf("got Management Error in response: %v", errorPacket.ManagementErrorStatusTLV.ManagementErrorID)
	}
	p, ok := res.(*Management)
	if !ok {
		return nil, fmt.Errorf("got unexpected management packet %T", res)
	}
	return p, nil
}

// ParentDataSet sends PARENT_DATA_SET request and returns response
func (c *MgmtClient) ParentDataSet() (*ParentDataSetTLV, error) {
	req := ParentDataSetRequest()
	p, err := c.Communicate(req)
	if err != nil {
		return nil, err
	}
	tlv, ok := p.TLV.(*ParentDataSetTLV)
	if !ok {
		return nil, fmt.Errorf("got unexpected management TLV %T, wanted %T", p.TLV, tlv)
	}
	return tlv, nil
}

// DefaultDataSet sends DEFAULT_DATA_SET request and returns response
func (c *MgmtClient) DefaultDataSet() (*DefaultDataSetTLV, error) {
	req := DefaultDataSetRequest()
	p, err := c.Communicate(req)
	if err != nil {
		return nil, err
	}
	tlv, ok := p.TLV.(*DefaultDataSetTLV)
	if !ok {
		return nil, fmt.Errorf("got unexpected management TLV %T, wanted %T", p.TLV, tlv)
	}
	return tlv, nil
}

// CurrentDataSet sends CURRENT_DATA_SET request and returns response
func (c *MgmtClient) CurrentDataSet() (*CurrentDataSetTLV, error) {
	req := CurrentDataSetRequest()
	p, err := c.Communicate(req)
	if err != nil {
		return nil, err
	}
	tlv, ok := p.TLV.(*CurrentDataSetTLV)
	if !ok {
		return nil, fmt.Errorf("got unexpected management TLV %T, wanted %T", p.TLV, tlv)
	}
	return tlv, nil
}
