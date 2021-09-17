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

func Test_parseTimeStatusNP(t *testing.T) {
	raw := []uint8{
		13, 2, 0, 104, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		72, 87, 221, 255, 254, 8, 100, 136, 0, 0, 0, 5, 4, 127, 0, 0, 0, 0, 0, 0, 0, 0,
		26, 22, 0, 0, 2, 0, 0, 1, 0, 52, 192, 0, 0, 0, 0, 0, 1, 118, 101, 201, 22, 107,
		79, 96, 119, 80, 118, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 36, 138, 7, 255, 254, 63, 48, 154, 0, 0,
	}
	packet := new(Management)
	err := FromBytes(raw, packet)
	require.Nil(t, err)
	want := Management{
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
				SequenceID:         5,
				ControlField:       4,
				LogMessageInterval: 0x7f,
			},
			TargetPortIdentity: PortIdentity{
				PortNumber:    6678,
				ClockIdentity: 0,
			},
			ActionField: RESPONSE,
		},
		TLV: &TimeStatusNPTLV{
			ManagementTLVHead: ManagementTLVHead{
				TLVHead: TLVHead{
					TLVType:     TLVManagement,
					LengthField: 52,
				},
				ManagementID: IDTimeStatusNP,
			},
			MasterOffsetNS:             24536521,
			IngressTimeNS:              1615472167079671311,
			CumulativeScaledRateOffset: 0,
			ScaledLastGmPhaseChange:    0,
			GMTimeBaseIndicator:        0,
			LastGmPhaseChange: ScaledNS{
				NanosecondsMSB:        0,
				NanosecondsLSB:        0,
				FractionalNanoseconds: 0,
			},
			GMPresent:  1,
			GMIdentity: 2632925728215085210, // 248a07.fffe.3f309a,
		},
	}
	require.Equal(t, want, *packet)
	b, err := Bytes(packet)
	require.Nil(t, err)
	assert.Equal(t, raw, b)
}
