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

// all references are given for IEEE 1588-2019 Standard

import (
	"bytes"
	"encoding"
	"encoding/binary"
	"fmt"
)

// Version is what version of PTP protocol we implement
const Version uint8 = 2

/* UDP port numbers
The UDP destination port of a PTP event message shall be 319.
The UDP destination port of a multicast PTP general message shall be 320.
The UDP destination port of a unicast PTP general message that is addressed to a PTP Instance shall be 320.
The UDP destination port of a unicast PTP general message that is addressed to a manager shall be the UDP source
port value of the PTP message to which this is a response.
*/
const (
	PortEvent   = 319
	PortGeneral = 320
)

// MgmtLogMessageInterval is the default LogInterval value used in Management packets
const MgmtLogMessageInterval LogInterval = 0x7f // as per Table 42 Values of logMessageInterval field

// DefaultTargetPortIdentity is a port identity that means any port
var DefaultTargetPortIdentity = PortIdentity{
	ClockIdentity: 0xffffffffffffffff,
	PortNumber:    0xffff,
}

// Header Table 35 Common PTP message header
type Header struct {
	SdoIDAndMsgType     SdoIDAndMsgType // first 4 bits is SdoId, next 4 bytes are msgtype
	Version             uint8
	MessageLength       uint16
	DomainNumber        uint8
	MinorSdoID          uint8
	FlagField           uint16
	CorrectionField     Correction
	MessageTypeSpecific uint32
	SourcePortIdentity  PortIdentity
	SequenceID          uint16
	ControlField        uint8       // the use of this field is obsolete according to IEEE, unless it's ipv4
	LogMessageInterval  LogInterval // see Table 42 Values of logMessageInterval field
}

const HeaderSize = 34 // bytes

// unmarshalHeader is not a Header.UnmarshalBinary to prevent all packets
// from having default (and incomplete) UnmarshalBinary implementation through embedding
func unmarshalHeader(p *Header, b []byte) {
	p.SdoIDAndMsgType = SdoIDAndMsgType(b[0])
	p.Version = b[1]
	p.MessageLength = binary.BigEndian.Uint16(b[2:])
	p.DomainNumber = b[4]
	p.MinorSdoID = b[5]
	p.FlagField = binary.BigEndian.Uint16(b[6:])
	p.CorrectionField = Correction(binary.BigEndian.Uint64(b[8:]))
	p.MessageTypeSpecific = binary.BigEndian.Uint32(b[16:])
	p.SourcePortIdentity.ClockIdentity = ClockIdentity(binary.BigEndian.Uint64(b[20:]))
	p.SourcePortIdentity.PortNumber = binary.BigEndian.Uint16(b[28:])
	p.SequenceID = binary.BigEndian.Uint16(b[30:])
	p.ControlField = b[32]
	p.LogMessageInterval = LogInterval(b[33])
}

// MessageType returns MessageType
func (p *Header) MessageType() MessageType {
	return p.SdoIDAndMsgType.MsgType()
}

// SetSequence populates sequence field
func (p *Header) SetSequence(sequence uint16) {
	p.SequenceID = sequence
}

// headerMarshalBinaryTo is not a Header.MarshalBinaryTo to prevent all packets
// from having default (and incomplete) MarshalBinaryTo implementation through embedding
func headerMarshalBinaryTo(p *Header, b []byte) int {
	b[0] = byte(p.SdoIDAndMsgType)
	b[1] = byte(p.Version)
	binary.BigEndian.PutUint16(b[2:], p.MessageLength)
	b[4] = byte(p.DomainNumber)
	b[5] = byte(p.MinorSdoID)
	binary.BigEndian.PutUint16(b[6:], p.FlagField)
	binary.BigEndian.PutUint64(b[8:], uint64(p.CorrectionField))
	binary.BigEndian.PutUint32(b[16:], p.MessageTypeSpecific)
	binary.BigEndian.PutUint64(b[20:], uint64(p.SourcePortIdentity.ClockIdentity))
	binary.BigEndian.PutUint16(b[28:], p.SourcePortIdentity.PortNumber)
	binary.BigEndian.PutUint16(b[30:], p.SequenceID)
	b[32] = byte(p.ControlField)
	b[33] = byte(p.LogMessageInterval)
	return HeaderSize
}

// flags used in FlagField as per Table 37 Values of flagField
const (
	// first octet
	FlagAlternateMaster  uint16 = 1 << (8 + 0)
	FlagTwoStep          uint16 = 1 << (8 + 1)
	FlagUnicast          uint16 = 1 << (8 + 2)
	FlagProfileSpecific1 uint16 = 1 << (8 + 5)
	FlagProfileSpecific2 uint16 = 1 << (8 + 6)
	// second octet
	FlagLeap61                   uint16 = 1 << 0
	FlagLeap59                   uint16 = 1 << 1
	FlagCurrentUtcOffsetValid    uint16 = 1 << 2
	FlagPTPTimescale             uint16 = 1 << 3
	FlagTimeTraceable            uint16 = 1 << 4
	FlagFrequencyTraceable       uint16 = 1 << 5
	FlagSynchronizationUncertain uint16 = 1 << 6
)

// General PTP messages

// All packets are split in two parts: Header (which is common) and body that is unique
// for most packets (both in length and structure).
// The idea is that anything using this library to read packets will have to do roughly this:
// * receive raw packets as bytes, create bytes.Reader from it
// * use binary.Read to parse Header from this reader
// * analyze header fields and switch on MessageType
// * parse rest of the data into one of Body structs, with exact struct being chosen according to header MessageType.

// AnnounceBody Table 43 Announce message fields
type AnnounceBody struct {
	OriginTimestamp         Timestamp
	CurrentUTCOffset        int16
	Reserved                uint8
	GrandmasterPriority1    uint8
	GrandmasterClockQuality ClockQuality
	GrandmasterPriority2    uint8
	GrandmasterIdentity     ClockIdentity
	StepsRemoved            uint16
	TimeSource              TimeSource
}

// Announce is a full Announce packet
type Announce struct {
	Header
	AnnounceBody
}

func (p *Announce) MarshalBinaryTo(b []byte) (int, error) {
	if len(b) < HeaderSize+30 {
		return 0, fmt.Errorf("not enough buffer to write Announce")
	}
	n := headerMarshalBinaryTo(&p.Header, b)
	copy(b[n:], p.OriginTimestamp.Seconds[:]) //uint48
	binary.BigEndian.PutUint32(b[n+6:], p.OriginTimestamp.Nanoseconds)
	binary.BigEndian.PutUint16(b[n+10:], uint16(p.CurrentUTCOffset))
	b[n+12] = byte(p.Reserved)
	b[n+13] = byte(p.GrandmasterPriority1)
	b[n+14] = byte(p.GrandmasterClockQuality.ClockClass)
	b[n+15] = byte(p.GrandmasterClockQuality.ClockAccuracy)
	binary.BigEndian.PutUint16(b[n+16:], p.GrandmasterClockQuality.OffsetScaledLogVariance)
	b[n+18] = byte(p.GrandmasterPriority2)
	binary.BigEndian.PutUint64(b[n+19:], uint64(p.GrandmasterIdentity))
	binary.BigEndian.PutUint16(b[n+27:], p.StepsRemoved)
	b[n+29] = byte(p.TimeSource)
	return n + 30, nil
}

// MarshalBinary converts packet to []bytes
func (p *Announce) MarshalBinary() ([]byte, error) {
	buf := make([]byte, 64)
	n, err := p.MarshalBinaryTo(buf)
	return buf[:n], err
}

// SyncDelayReqBody Table 44 Sync and Delay_Req message fields
type SyncDelayReqBody struct {
	OriginTimestamp Timestamp
}

// SyncDelayReq is a full Sync/Delay_Req packet
type SyncDelayReq struct {
	Header
	SyncDelayReqBody
}

func (p *SyncDelayReq) MarshalBinaryTo(b []byte) (int, error) {
	if len(b) < HeaderSize+10 {
		return 0, fmt.Errorf("not enough buffer to write SyncDelayReq")
	}
	n := headerMarshalBinaryTo(&p.Header, b)
	copy(b[n:], p.OriginTimestamp.Seconds[:]) //uint48
	binary.BigEndian.PutUint32(b[n+6:], p.OriginTimestamp.Nanoseconds)
	return n + 10, nil
}

// MarshalBinary converts packet to []bytes
func (p *SyncDelayReq) MarshalBinary() ([]byte, error) {
	buf := make([]byte, 44)
	n, err := p.MarshalBinaryTo(buf)
	return buf[:n], err
}

func (p *SyncDelayReq) UnmarshalBinary(b []byte) error {
	if len(b) < HeaderSize+10 {
		return fmt.Errorf("not enough data to decode SyncDelayReq")
	}
	unmarshalHeader(&p.Header, b)
	copy(p.OriginTimestamp.Seconds[:], b[HeaderSize:]) //uint48
	p.OriginTimestamp.Nanoseconds = binary.BigEndian.Uint32(b[HeaderSize+6:])
	return nil
}

// FollowUpBody Table 45 Follow_Up message fields
type FollowUpBody struct {
	PreciseOriginTimestamp Timestamp
}

// FollowUp is a full Follow_Up packet
type FollowUp struct {
	Header
	FollowUpBody
}

func (p *FollowUp) MarshalBinaryTo(b []byte) (int, error) {
	if len(b) < HeaderSize+10 {
		return 0, fmt.Errorf("not enough buffer to write FollowUp")
	}
	n := headerMarshalBinaryTo(&p.Header, b)
	copy(b[n:], p.PreciseOriginTimestamp.Seconds[:]) //uint48
	binary.BigEndian.PutUint32(b[n+6:], p.PreciseOriginTimestamp.Nanoseconds)
	return n + 10, nil
}

// MarshalBinary converts packet to []bytes
func (p *FollowUp) MarshalBinary() ([]byte, error) {
	buf := make([]byte, 44)
	n, err := p.MarshalBinaryTo(buf)
	return buf[:n], err
}

func (p *FollowUp) UnmarshalBinary(b []byte) error {
	if len(b) < HeaderSize+10 {
		return fmt.Errorf("not enough data to decode FollowUp")
	}
	unmarshalHeader(&p.Header, b)
	copy(p.PreciseOriginTimestamp.Seconds[:], b[HeaderSize:]) //uint48
	p.PreciseOriginTimestamp.Nanoseconds = binary.BigEndian.Uint32(b[HeaderSize+6:])
	return nil
}

// DelayRespBody Table 46 Delay_Resp message fields
type DelayRespBody struct {
	ReceiveTimestamp       Timestamp
	RequestingPortIdentity PortIdentity
}

// DelayResp is a full Delay_Resp packet
type DelayResp struct {
	Header
	DelayRespBody
}

func (p *DelayResp) MarshalBinaryTo(b []byte) (int, error) {
	if len(b) < HeaderSize+20 {
		return 0, fmt.Errorf("not enough buffer to write DelayResp")
	}
	n := headerMarshalBinaryTo(&p.Header, b)
	copy(b[n:], p.ReceiveTimestamp.Seconds[:]) //uint48
	binary.BigEndian.PutUint32(b[n+6:], p.ReceiveTimestamp.Nanoseconds)
	binary.BigEndian.PutUint64(b[n+10:], uint64(p.RequestingPortIdentity.ClockIdentity))
	binary.BigEndian.PutUint16(b[n+18:], p.RequestingPortIdentity.PortNumber)
	return n + 20, nil
}

// MarshalBinary converts packet to []bytes
func (p *DelayResp) MarshalBinary() ([]byte, error) {
	buf := make([]byte, 54)
	n, err := p.MarshalBinaryTo(buf)
	return buf[:n], err
}

func (p *DelayResp) UnmarshalBinary(b []byte) error {
	if len(b) < HeaderSize+20 {
		return fmt.Errorf("not enough data to decode DelayResp")
	}
	unmarshalHeader(&p.Header, b)
	copy(p.ReceiveTimestamp.Seconds[:], b[HeaderSize:]) //uint48
	p.ReceiveTimestamp.Nanoseconds = binary.BigEndian.Uint32(b[HeaderSize+6:])
	p.RequestingPortIdentity.ClockIdentity = ClockIdentity(binary.BigEndian.Uint64(b[HeaderSize+10:]))
	p.RequestingPortIdentity.PortNumber = binary.BigEndian.Uint16(b[HeaderSize+18:])
	return nil
}

// PDelayReqBody Table 47 Pdelay_Req message fields
type PDelayReqBody struct {
	OriginTimestamp Timestamp
	Reserved        [10]uint8
}

// PDelayReq is a full Pdelay_Req packet
type PDelayReq struct {
	Header
	PDelayReqBody
}

// PDelayRespBody Table 48 Pdelay_Resp message fields
type PDelayRespBody struct {
	RequestReceiptTimestamp Timestamp
	RequestingPortIdentity  PortIdentity
}

// PDelayResp is a full Pdelay_Resp packet
type PDelayResp struct {
	Header
	PDelayRespBody
}

// PDelayRespFollowUpBody Table 49 Pdelay_Resp_Follow_Up message fields
type PDelayRespFollowUpBody struct {
	ResponseOriginTimestamp Timestamp
	RequestingPortIdentity  PortIdentity
}

// PDelayRespFollowUp is a full Pdelay_Resp_Follow_Up packet
type PDelayRespFollowUp struct {
	Header
	PDelayRespFollowUpBody
}

// Packet is an iterface to abstract all different packets
type Packet interface {
	MessageType() MessageType
	SetSequence(uint16)
}

// BinaryMarshalerTo is an interface implemented by an object that can marshal itself into a binary form into provided []byte
type BinaryMarshalerTo interface {
	MarshalBinaryTo([]byte) (int, error)
}

// BytesTo marhals packets that support this optimized marshalling into []byte
func BytesTo(p BinaryMarshalerTo, buf []byte) (int, error) {
	n, err := p.MarshalBinaryTo(buf)
	if err != nil {
		return 0, err
	}
	// add two zero bytes
	buf[n] = 0x0
	buf[n+1] = 0x0
	return n + 2, nil
}

var twoZeros = []byte{0, 0}

// Bytes converts any packet to []bytes
// PTP over UDPv6 requires adding extra two bytes that
// may be modified by the initiator or an intermediate PTP Instance to ensure that the UDP checksum
// remains uncompromised after any modification of PTP fields.
// We simply always add them - in worst case they add extra 2 unused bytes when used over UDPv4.
func Bytes(p Packet) ([]byte, error) {
	// interface smuggling
	if pp, ok := p.(encoding.BinaryMarshaler); ok {
		b, err := pp.MarshalBinary()
		return append(b, twoZeros...), err
	}
	var bytes bytes.Buffer
	err := binary.Write(&bytes, binary.BigEndian, p)
	if err != nil {
		return nil, err
	}
	err = binary.Write(&bytes, binary.BigEndian, twoZeros)
	return bytes.Bytes(), err
}

// FromBytes parses []byte into any packet
func FromBytes(rawBytes []byte, p Packet) error {
	// interface smuggling
	if pp, ok := p.(encoding.BinaryUnmarshaler); ok {
		return pp.UnmarshalBinary(rawBytes)
	}
	reader := bytes.NewReader(rawBytes)
	return binary.Read(reader, binary.BigEndian, p)
}

// DecodePacket provides single entry point to try and decode any []bytes to PTPv2 packet.
// It can be used for easy integration with anything that provides UDP packet payload as bytes.
// Resulting Packet user can then either switch based on MessageType(), or just with type switch.
func DecodePacket(b []byte) (Packet, error) {
	r := bytes.NewReader(b)
	head := &Header{}
	if err := binary.Read(r, binary.BigEndian, head); err != nil {
		return nil, err
	}
	msgType := head.MessageType()
	var p Packet
	switch msgType {
	case MessageSync, MessageDelayReq:
		p = &SyncDelayReq{}
	case MessagePDelayReq:
		p = &PDelayReq{}
	case MessagePDelayResp:
		p = &PDelayResp{}
	case MessageFollowUp:
		p = &FollowUp{}
	case MessageDelayResp:
		p = &DelayResp{}
	case MessagePDelayRespFollowUp:
		p = &PDelayRespFollowUp{}
	case MessageAnnounce:
		p = &Announce{}
	case MessageSignaling:
		p = &Signaling{}
	case MessageManagement:
		return decodeMgmtPacket(b)
	default:
		return nil, fmt.Errorf("unsupported type %s", msgType)
	}

	if err := FromBytes(b, p); err != nil {
		return nil, err
	}
	return p, nil
}
