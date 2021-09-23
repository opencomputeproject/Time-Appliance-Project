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

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"os"
)

var identity PortIdentity

// ErrManagementMsgErrorStatus is what happens if we expected to get Management TLV in response, but received special ManagementErrorStatusTLV
var ErrManagementMsgErrorStatus = errors.New("received MANAGEMENT_ERROR_STATUS_TLV")

func init() {
	// store our PID as identity that we use to talk to ptp daemon
	identity.PortNumber = uint16(os.Getpid())
}

// Action indicate the action to be taken on receipt of the PTP message as defined in Table 57
type Action uint8

// actions as in Table 57 Values of the actionField
const (
	GET Action = iota
	SET
	RESPONSE
	COMMAND
	ACKNOWLEDGE
)

// ManagementTLVHead Spec Table 58 - Management TLV fields
type ManagementTLVHead struct {
	TLVHead

	ManagementID ManagementID
}

// ManagementMsgHead Spec Table 56 - Management message fields
type ManagementMsgHead struct {
	Header

	TargetPortIdentity   PortIdentity
	StartingBoundaryHops uint8
	BoundaryHops         uint8
	ActionField          Action
	Reserved             uint8
}

// Action returns ActionField
func (p *ManagementMsgHead) Action() Action {
	return p.ActionField
}

// MgmtID returns ManagementID
func (p *ManagementTLVHead) MgmtID() ManagementID {
	return p.ManagementID
}

// Management packet, see '15. PTP management messages'
type Management struct {
	ManagementMsgHead
	TLV ManagementTLV
}

// UnmarshalBinary parses []byte and populates struct fields
func (p *Management) UnmarshalBinary(rawBytes []byte) error {
	var err error
	head := ManagementMsgHead{}
	tlvHead := ManagementTLVHead{}
	r := bytes.NewReader(rawBytes)
	if err = binary.Read(r, binary.BigEndian, &head); err != nil {
		return err
	}
	if err = binary.Read(r, binary.BigEndian, &tlvHead.TLVHead); err != nil {
		return err
	}
	if tlvHead.TLVType == TLVManagementErrorStatus {
		return ErrManagementMsgErrorStatus
	}
	if tlvHead.TLVType != TLVManagement {
		return fmt.Errorf("got TLV type 0x%x instead of 0x%x", tlvHead.TLVType, TLVManagement)
	}

	if err = binary.Read(r, binary.BigEndian, &tlvHead.ManagementID); err != nil {
		return err
	}
	headSize := binary.Size(tlvHead)
	// seek back so we can read whole TLV
	if _, err := r.Seek(-int64(headSize), io.SeekCurrent); err != nil {
		return err
	}
	decoder, found := mgmtTLVDecoder[tlvHead.ManagementID]
	if !found {
		return fmt.Errorf("unsupported management TLV 0x%x", tlvHead.ManagementID)
	}
	tlvData, err := ioutil.ReadAll(r)
	if err != nil {
		return err
	}
	tlv, err := decoder(tlvData)
	if err != nil {
		return err
	}
	p.ManagementMsgHead = head
	p.TLV = tlv
	return nil
}

// MarshalBinaryTo converts packet to bytes and writes those into provided buffer
func (p *Management) MarshalBinaryTo(bytes io.Writer) error {
	if err := binary.Write(bytes, binary.BigEndian, p.ManagementMsgHead); err != nil {
		return err
	}
	if err := binary.Write(bytes, binary.BigEndian, p.TLV); err != nil {
		return err
	}
	return nil
}

// MarshalBinary converts packet to []bytes
func (p *Management) MarshalBinary() ([]byte, error) {
	var bytes bytes.Buffer
	err := p.MarshalBinaryTo(&bytes)
	return bytes.Bytes(), err
}

// ManagementErrorStatusTLV spec Table 108 MANAGEMENT_ERROR_STATUS TLV format
type ManagementErrorStatusTLV struct {
	TLVHead

	ManagementErrorID ManagementErrorID
	ManagementID      ManagementID
	Reserved          int32
	DisplayData       PTPText
}

// ManagementMsgErrorStatus is header + ManagementErrorStatusTLV
type ManagementMsgErrorStatus struct {
	ManagementMsgHead
	ManagementErrorStatusTLV
}

// UnmarshalBinary parses []byte and populates struct fields
func (p *ManagementMsgErrorStatus) UnmarshalBinary(rawBytes []byte) error {
	reader := bytes.NewReader(rawBytes)
	be := binary.BigEndian
	if err := binary.Read(reader, be, &p.ManagementMsgHead); err != nil {
		return fmt.Errorf("reading ManagementMsgErrorStatus ManagementMsgHead: %w", err)
	}
	if err := binary.Read(reader, be, &p.ManagementErrorStatusTLV.TLVHead); err != nil {
		return fmt.Errorf("reading ManagementMsgErrorStatus TLVHead: %w", err)
	}
	if err := binary.Read(reader, be, &p.ManagementErrorStatusTLV.ManagementErrorID); err != nil {
		return fmt.Errorf("reading ManagementMsgErrorStatus ManagementErrorID: %w", err)
	}
	if err := binary.Read(reader, be, &p.ManagementErrorStatusTLV.ManagementID); err != nil {
		return fmt.Errorf("reading ManagementMsgErrorStatus ManagementID: %w", err)
	}
	if err := binary.Read(reader, be, &p.ManagementErrorStatusTLV.Reserved); err != nil {
		return fmt.Errorf("reading ManagementMsgErrorStatus Reserved: %w", err)
	}
	// packet can have trailing bytes, let's make sure we don't try to read past given length
	toRead := int(p.ManagementMsgHead.Header.MessageLength)
	toRead -= binary.Size(p.ManagementMsgHead)
	toRead -= binary.Size(p.ManagementErrorStatusTLV.TLVHead)
	toRead -= binary.Size(p.ManagementErrorStatusTLV.ManagementErrorID)
	toRead -= binary.Size(p.ManagementErrorStatusTLV.ManagementID)
	toRead -= binary.Size(p.ManagementErrorStatusTLV.Reserved)

	if reader.Len() == 0 || toRead <= 0 {
		// DisplayData is completely optional
		return nil
	}
	data := make([]byte, reader.Len())
	if _, err := io.ReadFull(reader, data); err != nil {
		return err
	}
	if err := p.DisplayData.UnmarshalBinary(data); err != nil {
		return fmt.Errorf("reading ManagementMsgErrorStatus DisplayData: %w", err)
	}
	return nil
}

// MarshalBinaryTo converts packet to bytes and writes those into provided buffer
func (p *ManagementMsgErrorStatus) MarshalBinaryTo(bytes io.Writer) error {
	be := binary.BigEndian
	if err := binary.Write(bytes, be, &p.ManagementMsgHead); err != nil {
		return fmt.Errorf("writing ManagementMsgErrorStatus ManagementMsgHead: %w", err)
	}
	if err := binary.Write(bytes, be, &p.ManagementErrorStatusTLV.TLVHead); err != nil {
		return fmt.Errorf("writing ManagementMsgErrorStatus TLVHead: %w", err)
	}
	if err := binary.Write(bytes, be, &p.ManagementErrorStatusTLV.ManagementErrorID); err != nil {
		return fmt.Errorf("writing ManagementMsgErrorStatus ManagementErrorID: %w", err)
	}
	if err := binary.Write(bytes, be, &p.ManagementErrorStatusTLV.ManagementID); err != nil {
		return fmt.Errorf("writing ManagementMsgErrorStatus ManagementID: %w", err)
	}
	if err := binary.Write(bytes, be, &p.ManagementErrorStatusTLV.Reserved); err != nil {
		return fmt.Errorf("writing ManagementMsgErrorStatus Reserved: %w", err)
	}
	if p.DisplayData != "" {
		dd, err := p.DisplayData.MarshalBinary()
		if err != nil {
			return fmt.Errorf("writing ManagementMsgErrorStatus DisplayData: %w", err)
		}
		if _, err := bytes.Write(dd); err != nil {
			return err
		}
	}
	return nil
}

// MarshalBinary converts packet to []bytes
func (p *ManagementMsgErrorStatus) MarshalBinary() ([]byte, error) {
	var bytes bytes.Buffer
	err := p.MarshalBinaryTo(&bytes)
	return bytes.Bytes(), err
}

// ManagementErrorID is an enum for possible management errors
type ManagementErrorID uint16

// Table 109 ManagementErrorID enumeration
const (
	ErrorResponseTooBig ManagementErrorID = 0x0001 // The requested operation could not fit in a single response message
	ErrorNoSuchID       ManagementErrorID = 0x0002 // The managementId is not recognized
	ErrorWrongLength    ManagementErrorID = 0x0003 // The managementId was identified but the length of the data was wrong
	ErrorWrongValue     ManagementErrorID = 0x0004 // The managementId and length were correct but one or more values were wrong
	ErrorNotSetable     ManagementErrorID = 0x0005 // Some of the variables in the set command were not updated because they are not configurable
	ErrorNotSupported   ManagementErrorID = 0x0006 // The requested operation is not supported in this PTP Instance
	ErrorUnpopulated    ManagementErrorID = 0x0007 // The targetPortIdentity of the PTP management message refers to an entity that is not present in the PTP Instance at the time of the request
	// some reserved and provile-specific ranges
	ErrorGeneralError ManagementErrorID = 0xFFFE //An error occurred that is not covered by other ManagementErrorID values
)

// ManagementErrorIDToString is a map from ManagementErrorID to string
var ManagementErrorIDToString = map[ManagementErrorID]string{
	ErrorResponseTooBig: "RESPONSE_TOO_BIG",
	ErrorNoSuchID:       "NO_SUCH_ID",
	ErrorWrongLength:    "WRONG_LENGTH",
	ErrorWrongValue:     "WRONG_VALUE",
	ErrorNotSetable:     "NOT_SETABLE",
	ErrorNotSupported:   "NOT_SUPPORTED",
	ErrorUnpopulated:    "UNPOPULATED",
	ErrorGeneralError:   "GENERAL_ERROR",
}

func (t ManagementErrorID) String() string {
	s := ManagementErrorIDToString[t]
	if s == "" {
		return fmt.Sprintf("UNKNOWN_ERROR_ID=%d", t)
	}
	return s
}

func (t ManagementErrorID) Error() string {
	return t.String()
}

func decodeMgmtPacket(data []byte) (Packet, error) {
	packet := &Management{}
	err := packet.UnmarshalBinary(data)
	if errors.Is(err, ErrManagementMsgErrorStatus) {
		errorPacket := new(ManagementMsgErrorStatus)
		if err := errorPacket.UnmarshalBinary(data); err != nil {
			return nil, fmt.Errorf("got Management Error in response but failed to decode it: %w", err)
		}
		return errorPacket, nil
	}
	if err != nil {
		return nil, err
	}
	return packet, nil
}
