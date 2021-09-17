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

func Test_parseMsgErrorStatus(t *testing.T) {
	raw := []uint8{0x0d, 0x02, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x48, 0x57, 0xdd, 0xff, 0xfe, 0x08, 0x64, 0x88, 0x00, 0x00,
		0x00, 0x01, 0x04, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0xdc, 0x6c, 0x00, 0x00, 0x02, 0x00, 0x00, 0x02,
		0x00, 0x08, 0x00, 0x06, 0x20, 0x01, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00,
	}
	packet := new(ManagementMsgErrorStatus)
	err := FromBytes(raw, packet)
	require.Nil(t, err)
	want := ManagementMsgErrorStatus{
		ManagementMsgHead: ManagementMsgHead{
			Header: Header{
				SdoIDAndMsgType:     NewSdoIDAndMsgType(MessageManagement, 0),
				Version:             2,
				MessageLength:       uint16(len(raw) - 2),
				DomainNumber:        0,
				MinorSdoID:          0,
				FlagField:           0,
				CorrectionField:     0,
				MessageTypeSpecific: 0,
				SourcePortIdentity: PortIdentity{
					PortNumber:    0,
					ClockIdentity: 5212879185253000328,
				},
				SequenceID:         1,
				ControlField:       4,
				LogMessageInterval: 0x7f,
			},
			TargetPortIdentity: PortIdentity{
				PortNumber:    56428,
				ClockIdentity: 0,
			},
			ActionField: RESPONSE,
		},
		ManagementErrorStatusTLV: ManagementErrorStatusTLV{
			TLVHead: TLVHead{
				TLVType:     TLVManagementErrorStatus,
				LengthField: 8,
			},
			ManagementErrorID: ErrorNotSupported,
			ManagementID:      IDCurrentDataSet,
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
