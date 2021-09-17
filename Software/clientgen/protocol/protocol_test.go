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
	"encoding/binary"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func Test_parseSync(t *testing.T) {
	raw := []uint8{
		0x10, 0x02, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x63, 0xff,
		0xff, 0x00, 0x09, 0xba, 0x00, 0x01, 0x00, 0x74,
		0x00, 0x00, 0x00, 0x00, 0x45, 0xb1, 0x11, 0x5a,
		0x0a, 0x64, 0xfa, 0xb0, 0x00, 0x00,
	}
	packet := new(SyncDelayReq)
	err := FromBytes(raw, packet)
	require.Nil(t, err)
	want := SyncDelayReq{
		Header: Header{
			SdoIDAndMsgType:     NewSdoIDAndMsgType(MessageSync, 1),
			Version:             2,
			MessageLength:       44,
			DomainNumber:        0,
			MinorSdoID:          0,
			FlagField:           0,
			CorrectionField:     0,
			MessageTypeSpecific: 0,
			SourcePortIdentity: PortIdentity{
				PortNumber:    1,
				ClockIdentity: 36138748164966842,
			},
			SequenceID:         116,
			ControlField:       0,
			LogMessageInterval: 0,
		},
		SyncDelayReqBody: SyncDelayReqBody{
			OriginTimestamp: Timestamp{
				Seconds:     [6]byte{0x0, 0x00, 0x45, 0xb1, 0x11, 0x5a},
				Nanoseconds: 174389936,
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

func Test_parseFollowup(t *testing.T) {
	raw := []uint8{
		0x8, 0x2, 0x0, 0x2c, 0x0, 0x0, 0x4, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x80, 0x63, 0xff, 0xff, 0x0,
		0x9, 0xba, 0x0, 0x1, 0x0, 0x0, 0x2, 0x0, 0x0,
		0x0, 0x45, 0xb1, 0x11, 0x5e, 0x4, 0x5d, 0xd2, 0x6e, 0x0, 0x0,
	}
	packet := new(FollowUp)
	err := FromBytes(raw, packet)
	require.Nil(t, err)
	want := FollowUp{
		Header: Header{
			SdoIDAndMsgType: NewSdoIDAndMsgType(MessageFollowUp, 0),
			Version:         Version,
			MessageLength:   uint16(binary.Size(FollowUp{})),
			DomainNumber:    0,
			FlagField:       FlagUnicast,
			SequenceID:      0,
			SourcePortIdentity: PortIdentity{
				PortNumber:    1,
				ClockIdentity: 36138748164966842,
			},
			LogMessageInterval: 0,
			ControlField:       2,
		},
		FollowUpBody: FollowUpBody{
			PreciseOriginTimestamp: Timestamp{
				Seconds:     [6]byte{0x0, 0x00, 0x45, 0xb1, 0x11, 0x5e},
				Nanoseconds: 73257582,
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

func Test_parsePDelayReq(t *testing.T) {
	raw := []uint8{
		0x12, 0x02, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x63, 0xff,
		0xff, 0x00, 0x09, 0xba, 0x00, 0x01, 0x9e, 0x57,
		0x05, 0x0f, 0x00, 0x00, 0x45, 0xb1, 0x11, 0x5e,
		0x04, 0x5d, 0xd2, 0x6e, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	}
	packet := new(PDelayReq)
	err := FromBytes(raw, packet)
	require.Nil(t, err)
	want := PDelayReq{
		Header: Header{
			SdoIDAndMsgType:     NewSdoIDAndMsgType(MessagePDelayReq, 1),
			Version:             2,
			MessageLength:       54,
			DomainNumber:        0,
			MinorSdoID:          0,
			FlagField:           0,
			CorrectionField:     0,
			MessageTypeSpecific: 0,
			SourcePortIdentity: PortIdentity{
				PortNumber:    1,
				ClockIdentity: 36138748164966842,
			},
			SequenceID:         40535,
			ControlField:       5,
			LogMessageInterval: 15,
		},
		PDelayReqBody: PDelayReqBody{
			OriginTimestamp: Timestamp{
				Seconds:     [6]byte{0x0, 0x00, 0x45, 0xb1, 0x11, 0x5e},
				Nanoseconds: 73257582,
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

func Test_parseAnnounce(t *testing.T) {
	raw := []uint8{
		0xb, 0x2, 0x0, 0x40, 0x0, 0x0, 0x4, 0x8, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x80, 0x63, 0xff, 0xff, 0x0,
		0x9, 0xba, 0x0, 0x1, 0x0, 0x0, 0x5, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x80, 0x6, 0x21, 0x59, 0xe0,
		0x80, 0x0, 0x80, 0x63, 0xff, 0xff, 0x0,
		0x9, 0xba, 0x0, 0x0, 0x20, 0x0, 0x0,
	}
	packet := new(Announce)
	err := FromBytes(raw, packet)
	require.Nil(t, err)
	want := Announce{
		Header: Header{
			SdoIDAndMsgType: NewSdoIDAndMsgType(MessageAnnounce, 0),
			Version:         Version,
			MessageLength:   uint16(binary.Size(Announce{})),
			DomainNumber:    0,
			FlagField:       FlagUnicast | FlagPTPTimescale,
			SequenceID:      0,
			SourcePortIdentity: PortIdentity{
				PortNumber:    1,
				ClockIdentity: 36138748164966842,
			},
			LogMessageInterval: 0,
			ControlField:       5,
		},
		AnnounceBody: AnnounceBody{
			CurrentUTCOffset:     0,
			Reserved:             0,
			GrandmasterPriority1: 128,
			GrandmasterClockQuality: ClockQuality{
				ClockClass:              6,
				ClockAccuracy:           33, // 0x21 - Time Accurate within 100ns
				OffsetScaledLogVariance: 23008,
			},
			GrandmasterPriority2: 128,
			GrandmasterIdentity:  36138748164966842,
			StepsRemoved:         0,
			TimeSource:           TimeSourceGNSS,
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

func Test_parseDelayResp(t *testing.T) {
	raw := []uint8{
		0x9, 0x2, 0x0, 0x36, 0x0, 0x0, 0x4, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x80, 0x63, 0xff, 0xff, 0x0,
		0x9, 0xba, 0x0, 0x1, 0x0, 0xa, 0x3, 0x7f,
		0x0, 0x0, 0x45, 0xb1, 0x11, 0x5e, 0x4, 0x5d,
		0xd2, 0x6e, 0xb8, 0x59, 0x9f, 0xff, 0xfe,
		0x55, 0xaf, 0x4e, 0x0, 0x1, 0x0, 0x0,
	}
	packet := new(DelayResp)
	err := FromBytes(raw, packet)
	require.Nil(t, err)
	want := DelayResp{
		Header: Header{
			SdoIDAndMsgType: NewSdoIDAndMsgType(MessageDelayResp, 0),
			Version:         Version,
			MessageLength:   uint16(binary.Size(DelayResp{})),
			DomainNumber:    0,
			FlagField:       FlagUnicast,
			SequenceID:      10,
			SourcePortIdentity: PortIdentity{
				PortNumber:    1,
				ClockIdentity: 36138748164966842,
			},
			LogMessageInterval: 0x7f,
			ControlField:       3,
			CorrectionField:    0,
		},
		DelayRespBody: DelayRespBody{
			ReceiveTimestamp: Timestamp{
				Seconds:     [6]byte{0x0, 0x00, 0x45, 0xb1, 0x11, 0x5e},
				Nanoseconds: 73257582,
			},
			RequestingPortIdentity: PortIdentity{
				PortNumber:    1,
				ClockIdentity: 13283824497738493774,
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

func BenchmarkReadSyncDelay(b *testing.B) {
	raw := []uint8{
		0x12, 0x02, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x63, 0xff,
		0xff, 0x00, 0x09, 0xba, 0x00, 0x01, 0x9e, 0x57,
		0x05, 0x0f, 0x00, 0x00, 0x45, 0xb1, 0x11, 0x5e,
		0x04, 0x5d, 0xd2, 0x6e, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	}
	p := &SyncDelayReq{}
	for n := 0; n < b.N; n++ {
		_ = p.UnmarshalBinary(raw)
	}
}

func BenchmarkWriteSync(b *testing.B) {
	p := &SyncDelayReq{
		Header: Header{
			SdoIDAndMsgType:     NewSdoIDAndMsgType(MessageSync, 1),
			Version:             2,
			MessageLength:       44,
			DomainNumber:        0,
			MinorSdoID:          0,
			FlagField:           0,
			CorrectionField:     0,
			MessageTypeSpecific: 0,
			SourcePortIdentity: PortIdentity{
				PortNumber:    1,
				ClockIdentity: 36138748164966842,
			},
			SequenceID:         116,
			ControlField:       0,
			LogMessageInterval: 0,
		},
		SyncDelayReqBody: SyncDelayReqBody{
			OriginTimestamp: Timestamp{
				Seconds:     [6]byte{0x0, 0x00, 0x45, 0xb1, 0x11, 0x5a},
				Nanoseconds: 174389936,
			},
		},
	}
	buf := make([]byte, 64)
	for n := 0; n < b.N; n++ {
		_, _ = BytesTo(p, buf)
	}
}

func BenchmarkWriteFollowup(b *testing.B) {
	p := &FollowUp{
		Header: Header{
			SdoIDAndMsgType: NewSdoIDAndMsgType(MessageFollowUp, 0),
			Version:         Version,
			MessageLength:   uint16(binary.Size(FollowUp{})),
			DomainNumber:    0,
			FlagField:       FlagUnicast,
			SequenceID:      0,
			SourcePortIdentity: PortIdentity{
				PortNumber:    1,
				ClockIdentity: 36138748164966842,
			},
			LogMessageInterval: 0,
			ControlField:       2,
		},
		FollowUpBody: FollowUpBody{
			PreciseOriginTimestamp: Timestamp{
				Seconds:     [6]byte{0x0, 0x00, 0x45, 0xb1, 0x11, 0x5e},
				Nanoseconds: 73257582,
			},
		},
	}
	buf := make([]byte, 64)
	for n := 0; n < b.N; n++ {
		_, _ = BytesTo(p, buf)
	}
}
