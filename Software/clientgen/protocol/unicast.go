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
	"fmt"
)

// UnicastMsgTypeAndFlags is a uint8 where first 4 bites contain MessageType and last 4 bits contain some flags
type UnicastMsgTypeAndFlags uint8

const RequestUnicastTransmissionTLVSize = 10

// MsgType extracts MessageType from UnicastMsgTypeAndFlags
func (m UnicastMsgTypeAndFlags) MsgType() MessageType {
	return MessageType(m >> 4)
}

// NewUnicastMsgTypeAndFlags builds new UnicastMsgTypeAndFlags from MessageType and flags
func NewUnicastMsgTypeAndFlags(msgType MessageType, flags uint8) UnicastMsgTypeAndFlags {
	return UnicastMsgTypeAndFlags(uint8(msgType)<<4 | (flags & 0x0f))
}

// Signaling packet. As it's of variable size, we cannot just binary.Read/Write it.
type Signaling struct {
	Header
	TargetPortIdentity PortIdentity
	TLVs               []TLV
}

func (p *Signaling) MarshalBinaryTo(b []byte) (int, error) {
	if len(p.TLVs) == 0 {
		return 0, fmt.Errorf("no TLVs in Signaling message, at least one required")
	}
	n := headerMarshalBinaryTo(&p.Header, b)
	binary.BigEndian.PutUint64(b[n:], uint64(p.TargetPortIdentity.ClockIdentity))
	binary.BigEndian.PutUint16(b[n+8:], p.TargetPortIdentity.PortNumber)
	pos := n + 10
	for _, tlv := range p.TLVs {
		if ttlv, ok := tlv.(BinaryMarshalerTo); ok {
			nn, err := ttlv.MarshalBinaryTo(b[pos:])
			if err != nil {
				return 0, err
			}
			pos += nn
			continue
		}
		// very inefficient path for TLVs that don't support MarshalBinaryTo
		buf := new(bytes.Buffer)
		if err := binary.Write(buf, binary.BigEndian, tlv); err != nil {
			return 0, err
		}
		bbytes := buf.Bytes()
		copy(b[pos:], bbytes)
		pos += len(bbytes)
	}
	return pos, nil
}

// MarshalBinary converts packet to []bytes
func (p *Signaling) MarshalBinary() ([]byte, error) {
	buf := make([]byte, 200)
	n, err := p.MarshalBinaryTo(buf)
	return buf[:n], err
}

func unmarshalTLVHeader(p *TLVHead, b []byte) error {
	if len(b) < TlvHeadSize {
		return fmt.Errorf("not enough data to decode PTP header")
	}
	p.TLVType = TLVType(binary.BigEndian.Uint16(b[0:]))
	p.LengthField = binary.BigEndian.Uint16(b[2:])
	return nil
}

// UnmarshalBinary parses []byte and populates struct fields
func (p *Signaling) UnmarshalBinary(b []byte) error {
	if len(b) < HeaderSize+10+TlvHeadSize {
		return fmt.Errorf("not enough data to decode Signaling")
	}
	unmarshalHeader(&p.Header, b)
	if p.SdoIDAndMsgType.MsgType() != MessageSignaling {
		return fmt.Errorf("not a signaling message %v", b)
	}
	p.TargetPortIdentity.ClockIdentity = ClockIdentity(binary.BigEndian.Uint64(b[HeaderSize:]))
	p.TargetPortIdentity.PortNumber = binary.BigEndian.Uint16(b[HeaderSize+8:])

	pos := HeaderSize + 10
	var tlvType TLVType
	for {
		head := TLVHead{}
		// packet can have trailing bytes, let's make sure we don't try to read past given length
		if pos+TlvHeadSize > int(p.MessageLength) {
			break
		}
		tlvType = TLVType(binary.BigEndian.Uint16(b[pos:]))

		switch tlvType {
		case TLVAcknowledgeCancelUnicastTransmission:
			tlv := &AcknowledgeCancelUnicastTransmissionTLV{}
			if err := tlv.UnmarshalBinary(b[pos:]); err != nil {
				return err
			}
			p.TLVs = append(p.TLVs, tlv)
			pos += TlvHeadSize + int(tlv.LengthField)

		case TLVGrantUnicastTransmission:
			tlv := &GrantUnicastTransmissionTLV{}
			if err := tlv.UnmarshalBinary(b[pos:]); err != nil {
				return err
			}
			p.TLVs = append(p.TLVs, tlv)
			pos += TlvHeadSize + int(tlv.LengthField)

		case TLVRequestUnicastTransmission:
			tlv := &RequestUnicastTransmissionTLV{}
			if err := tlv.UnmarshalBinary(b[pos:]); err != nil {
				return err
			}
			p.TLVs = append(p.TLVs, tlv)
			pos += TlvHeadSize + int(tlv.LengthField)
		case TLVCancelUnicastTransmission:
			tlv := &CancelUnicastTransmissionTLV{}
			if err := tlv.UnmarshalBinary(b[pos:]); err != nil {
				return err
			}
			p.TLVs = append(p.TLVs, tlv)
			pos += TlvHeadSize + int(tlv.LengthField)
		default:
			return fmt.Errorf("reading TLV %s (%d) is not yet implemented", head.TLVType, head.TLVType)
		}
	}
	if len(p.TLVs) == 0 {
		return fmt.Errorf("no TLVs read for Signaling message, at least one required")
	}
	return nil
}

// Unicast TLVs

// RequestUnicastTransmissionTLV Table 110 REQUEST_UNICAST_TRANSMISSION TLV format
type RequestUnicastTransmissionTLV struct {
	TLVHead
	MsgTypeAndReserved    UnicastMsgTypeAndFlags // first 4 bits only, same enums as with normal message type
	LogInterMessagePeriod LogInterval
	DurationField         uint32
}

func (t *RequestUnicastTransmissionTLV) MarshalBinaryTo(b []byte) (int, error) {
	tlvHeadMarshalBinaryTo(&t.TLVHead, b)
	b[TlvHeadSize] = byte(t.MsgTypeAndReserved)
	b[TlvHeadSize+1] = byte(t.LogInterMessagePeriod)
	binary.BigEndian.PutUint32(b[TlvHeadSize+2:], t.DurationField)
	return TlvHeadSize + 6, nil
}

// UnmarshalBinary parses []byte and populates struct fields
func (t *RequestUnicastTransmissionTLV) UnmarshalBinary(b []byte) error {
	if err := unmarshalTLVHeader(&t.TLVHead, b); err != nil {
		return err
	}
	t.MsgTypeAndReserved = UnicastMsgTypeAndFlags(b[4])
	t.LogInterMessagePeriod = LogInterval(b[5])
	t.DurationField = binary.BigEndian.Uint32(b[6:])
	return nil
}

// GrantUnicastTransmissionTLV Table 111 GRANT_UNICAST_TRANSMISSION TLV format
type GrantUnicastTransmissionTLV struct {
	TLVHead
	MsgTypeAndReserved    UnicastMsgTypeAndFlags // first 4 bits only, same enums as with normal message type
	LogInterMessagePeriod LogInterval
	DurationField         uint32
	Reserved              uint8
	Renewal               uint8
}

func (t *GrantUnicastTransmissionTLV) MarshalBinaryTo(b []byte) (int, error) {
	tlvHeadMarshalBinaryTo(&t.TLVHead, b)
	b[TlvHeadSize] = byte(t.MsgTypeAndReserved)
	b[TlvHeadSize+1] = byte(t.LogInterMessagePeriod)
	binary.BigEndian.PutUint32(b[TlvHeadSize+2:], t.DurationField)
	b[TlvHeadSize+6] = t.Reserved
	b[TlvHeadSize+7] = t.Renewal
	return TlvHeadSize + 8, nil
}

// UnmarshalBinary parses []byte and populates struct fields
func (t *GrantUnicastTransmissionTLV) UnmarshalBinary(b []byte) error {
	if err := unmarshalTLVHeader(&t.TLVHead, b); err != nil {
		return err
	}
	t.MsgTypeAndReserved = UnicastMsgTypeAndFlags(b[4])
	t.LogInterMessagePeriod = LogInterval(b[5])
	t.DurationField = binary.BigEndian.Uint32(b[6:])
	t.Reserved = b[10]
	t.Renewal = b[11]
	return nil
}

// CancelUnicastTransmissionTLV Table 112 CANCEL_UNICAST_TRANSMISSION TLV format
type CancelUnicastTransmissionTLV struct {
	TLVHead
	MsgTypeAndFlags UnicastMsgTypeAndFlags // first 4 bits is msg type, then flags R and/or G
	Reserved        uint8
}

func (t *CancelUnicastTransmissionTLV) MarshalBinaryTo(b []byte) (int, error) {
	tlvHeadMarshalBinaryTo(&t.TLVHead, b)
	b[TlvHeadSize] = byte(t.MsgTypeAndFlags)
	b[TlvHeadSize+1] = byte(t.Reserved)
	return TlvHeadSize + 2, nil
}

// UnmarshalBinary parses []byte and populates struct fields
func (t *CancelUnicastTransmissionTLV) UnmarshalBinary(b []byte) error {
	if err := unmarshalTLVHeader(&t.TLVHead, b); err != nil {
		return err
	}
	t.MsgTypeAndFlags = UnicastMsgTypeAndFlags(b[4])
	t.Reserved = b[5]
	return nil
}

// AcknowledgeCancelUnicastTransmissionTLV Table 113 ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION TLV format
type AcknowledgeCancelUnicastTransmissionTLV struct {
	TLVHead
	MsgTypeAndFlags UnicastMsgTypeAndFlags // first 4 bits is msg type, then flags R and/or G
	Reserved        uint8
}

func (t *AcknowledgeCancelUnicastTransmissionTLV) MarshalBinaryTo(b []byte) (int, error) {
	tlvHeadMarshalBinaryTo(&t.TLVHead, b)
	b[TlvHeadSize] = byte(t.MsgTypeAndFlags)
	b[TlvHeadSize+1] = byte(t.Reserved)
	return TlvHeadSize + 2, nil
}

// UnmarshalBinary parses []byte and populates struct fields
func (t *AcknowledgeCancelUnicastTransmissionTLV) UnmarshalBinary(b []byte) error {
	if err := unmarshalTLVHeader(&t.TLVHead, b); err != nil {
		return err
	}
	t.MsgTypeAndFlags = UnicastMsgTypeAndFlags(b[4])
	t.Reserved = b[5]
	return nil
}
