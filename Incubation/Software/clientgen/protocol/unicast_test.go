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
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func Test_parseRequestUnicastTransmissionMultiTLV(t *testing.T) {
	raw := []uint8{0x0c, 0x02, 0x00, 0x4a, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb8, 0x59, 0x9f, 0xff, 0xfe, 0x55, 0xaf, 0x4e, 0x00, 0x01, 0x00, 0x00, 0x05, 0x7f, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0x00, 0x04, 0x00, 0x06, 0xb0, 0x01, 0x00, 0x00, 0x00, 0x3c, // first TLV
		0x00, 0x04, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3c, // second TLV
		0x00, 0x04, 0x00, 0x06, 0x90, 0x01, 0x00, 0x00, 0x00, 0x3c, // third TLV
		0x00, 0x00, // extra 2 bytes for udp6 checksum
	}
	packet := new(Signaling)
	err := FromBytes(raw, packet)
	require.Nil(t, err)
	want := Signaling{
		Header: Header{
			SdoIDAndMsgType:     NewSdoIDAndMsgType(MessageSignaling, 0),
			Version:             2,
			MessageLength:       uint16(len(raw) - 2),
			DomainNumber:        0,
			MinorSdoID:          0,
			FlagField:           FlagUnicast,
			CorrectionField:     0,
			MessageTypeSpecific: 0,
			SourcePortIdentity: PortIdentity{
				PortNumber:    1,
				ClockIdentity: 13283824497738493774,
			},
			SequenceID:         0,
			ControlField:       5,
			LogMessageInterval: 0x7f,
		},
		TargetPortIdentity: PortIdentity{
			PortNumber:    0xffff,
			ClockIdentity: 0xffffffffffffffff,
		},
		TLVs: []TLV{
			&RequestUnicastTransmissionTLV{
				TLVHead: TLVHead{
					TLVType:     TLVRequestUnicastTransmission,
					LengthField: 6,
				},
				MsgTypeAndReserved:    NewUnicastMsgTypeAndFlags(MessageAnnounce, 0),
				LogInterMessagePeriod: 1,
				DurationField:         60,
			},
			&RequestUnicastTransmissionTLV{
				TLVHead: TLVHead{
					TLVType:     TLVRequestUnicastTransmission,
					LengthField: 6,
				},
				MsgTypeAndReserved:    NewUnicastMsgTypeAndFlags(MessageSync, 0),
				LogInterMessagePeriod: 1,
				DurationField:         60,
			},
			&RequestUnicastTransmissionTLV{
				TLVHead: TLVHead{
					TLVType:     TLVRequestUnicastTransmission,
					LengthField: 6,
				},
				MsgTypeAndReserved:    NewUnicastMsgTypeAndFlags(MessageDelayResp, 0),
				LogInterMessagePeriod: 1,
				DurationField:         60,
			},
		},
	}
	require.Equal(t, want, *packet)
	b, err := Bytes(packet)
	require.Nil(t, err)
	assert.Equal(t, raw, b)

	// test generic DecodePacket as well
	pp, err := DecodePacket(raw)
	require.Nil(t, err)
	assert.Equal(t, &want, pp)
}

func Test_parseRequestUnicastTransmissionExtraBytes(t *testing.T) {
	raw := []uint8{0x0c, 0x02, 0x00, 0x40, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb8, 0x59, 0x9f, 0xff, 0xfe, 0x55, 0xaf, 0x4e, 0x00, 0x01, 0x00, 0x00, 0x05, 0x7f, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0x00, 0x04, 0x00, 0x06, 0xb0, 0x01, 0x00, 0x00, 0x00, 0x3c, // first TLV
		0x00, 0x04, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3c, // second TLV
		0x00, 0x00, // extra 2 bytes for udp6 checksum
	}
	// packets can arrive with trailing bytes, simulate it here
	extraBytes := []byte{0x00, 0x00}
	raw = append(raw, extraBytes...)

	packet := new(Signaling)
	err := FromBytes(raw, packet)
	require.Nil(t, err)
	want := Signaling{
		Header: Header{
			SdoIDAndMsgType:     NewSdoIDAndMsgType(MessageSignaling, 0),
			Version:             2,
			MessageLength:       uint16(len(raw) - 2 - len(extraBytes)),
			DomainNumber:        0,
			MinorSdoID:          0,
			FlagField:           FlagUnicast,
			CorrectionField:     0,
			MessageTypeSpecific: 0,
			SourcePortIdentity: PortIdentity{
				PortNumber:    1,
				ClockIdentity: 13283824497738493774,
			},
			SequenceID:         0,
			ControlField:       5,
			LogMessageInterval: 0x7f,
		},
		TargetPortIdentity: PortIdentity{
			PortNumber:    0xffff,
			ClockIdentity: 0xffffffffffffffff,
		},
		TLVs: []TLV{
			&RequestUnicastTransmissionTLV{
				TLVHead: TLVHead{
					TLVType:     TLVRequestUnicastTransmission,
					LengthField: 6,
				},
				MsgTypeAndReserved:    NewUnicastMsgTypeAndFlags(MessageAnnounce, 0),
				LogInterMessagePeriod: 1,
				DurationField:         60,
			},
			&RequestUnicastTransmissionTLV{
				TLVHead: TLVHead{
					TLVType:     TLVRequestUnicastTransmission,
					LengthField: 6,
				},
				MsgTypeAndReserved:    NewUnicastMsgTypeAndFlags(MessageSync, 0),
				LogInterMessagePeriod: 1,
				DurationField:         60,
			},
		},
	}
	require.Equal(t, want, *packet)
	b, err := Bytes(packet)
	require.Nil(t, err)
	assert.Equal(t, raw[:len(raw)-len(extraBytes)], b)

	// test generic DecodePacket as well
	pp, err := DecodePacket(raw)
	require.Nil(t, err)
	assert.Equal(t, &want, pp)
}

func Test_parseGrantUnicastTransmission(t *testing.T) {
	raw := []uint8{0x0c, 0x02, 0x00, 0x38, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xe4, 0x1d, 0x2d, 0xff, 0xfe, 0xbb, 0x64, 0x60, 0x00,
		0x01, 0x1d, 0xc4, 0x05, 0x7f, 0x48, 0x57, 0xdd, 0xff,
		0xfe, 0x08, 0x64, 0x88, 0x00, 0x01, 0x00, 0x05, 0x00,
		0x08, 0xb0, 0x01, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x01,
		0x00, 0x00,
	}
	packet := new(Signaling)
	err := FromBytes(raw, packet)
	require.Nil(t, err)
	want := Signaling{
		Header: Header{
			SdoIDAndMsgType:     NewSdoIDAndMsgType(MessageSignaling, 0),
			Version:             2,
			MessageLength:       uint16(len(raw) - 2),
			DomainNumber:        0,
			MinorSdoID:          0,
			FlagField:           FlagUnicast | FlagTwoStep,
			CorrectionField:     0,
			MessageTypeSpecific: 0,
			SourcePortIdentity: PortIdentity{
				PortNumber:    1,
				ClockIdentity: 16437344792485782624,
			},
			SequenceID:         7620,
			ControlField:       5,
			LogMessageInterval: 0x7f,
		},
		TargetPortIdentity: PortIdentity{
			PortNumber:    1,
			ClockIdentity: 5212879185253000328,
		},
		TLVs: []TLV{
			&GrantUnicastTransmissionTLV{
				TLVHead: TLVHead{
					TLVType:     TLVGrantUnicastTransmission,
					LengthField: 8,
				},
				MsgTypeAndReserved:    NewUnicastMsgTypeAndFlags(MessageAnnounce, 0),
				LogInterMessagePeriod: 1,
				DurationField:         60,
				Renewal:               1,
			},
		},
	}
	require.Equal(t, want, *packet)
	b, err := Bytes(packet)
	require.Nil(t, err)
	assert.Equal(t, raw, b)

	// test generic DecodePacket as well
	pp, err := DecodePacket(raw)
	require.Nil(t, err)
	assert.Equal(t, &want, pp)
}

func BenchmarkWriteSignaling(b *testing.B) {
	p := &Signaling{
		Header: Header{
			SdoIDAndMsgType:     NewSdoIDAndMsgType(MessageSignaling, 0),
			Version:             2,
			MessageLength:       56,
			DomainNumber:        0,
			MinorSdoID:          0,
			FlagField:           FlagUnicast | FlagTwoStep,
			CorrectionField:     0,
			MessageTypeSpecific: 0,
			SourcePortIdentity: PortIdentity{
				PortNumber:    1,
				ClockIdentity: 16437344792485782624,
			},
			SequenceID:         7620,
			ControlField:       5,
			LogMessageInterval: 0x7f,
		},
		TargetPortIdentity: PortIdentity{
			PortNumber:    1,
			ClockIdentity: 5212879185253000328,
		},
		TLVs: []TLV{
			&GrantUnicastTransmissionTLV{
				TLVHead: TLVHead{
					TLVType:     TLVGrantUnicastTransmission,
					LengthField: 8,
				},
				MsgTypeAndReserved:    NewUnicastMsgTypeAndFlags(MessageAnnounce, 0),
				LogInterMessagePeriod: 1,
				DurationField:         60,
				Renewal:               1,
			},
		},
	}
	buf := make([]byte, 64)
	for n := 0; n < b.N; n++ {
		_, _ = BytesTo(p, buf)
	}
}

func BenchmarkReadSignaling(b *testing.B) {
	raw := []uint8{0x0c, 0x02, 0x00, 0x38, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xe4, 0x1d, 0x2d, 0xff, 0xfe, 0xbb, 0x64, 0x60, 0x00,
		0x01, 0x1d, 0xc4, 0x05, 0x7f, 0x48, 0x57, 0xdd, 0xff,
		0xfe, 0x08, 0x64, 0x88, 0x00, 0x01, 0x00, 0x05, 0x00,
		0x08, 0xb0, 0x01, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x01,
		0x00, 0x00,
	}
	p := &Signaling{}
	for n := 0; n < b.N; n++ {
		_ = p.UnmarshalBinary(raw)
	}
}
