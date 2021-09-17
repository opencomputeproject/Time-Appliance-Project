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
)

// ManagementID is type for Management IDs
type ManagementID uint16

// Management IDs we support, from Table 59 managementId values
const (
	IDNullPTPManagement        ManagementID = 0x0000
	IDClockDescription         ManagementID = 0x0001
	IDUserDescription          ManagementID = 0x0002
	IDSaveInNonVolatileStorage ManagementID = 0x0003
	IDResetNonVolatileStorage  ManagementID = 0x0004
	IDInitialize               ManagementID = 0x0005
	IDFaultLog                 ManagementID = 0x0006
	IDFaultLogReset            ManagementID = 0x0007

	IDDefaultDataSet        ManagementID = 0x2000
	IDCurrentDataSet        ManagementID = 0x2001
	IDParentDataSet         ManagementID = 0x2002
	IDTimePropertiesDataSet ManagementID = 0x2003
	IDPortDataSet           ManagementID = 0x2004
	// rest of Management IDs that we don't implement yet
)

// ManagementTLV abstracts away any ManagementTLV
type ManagementTLV interface {
	TLV
	MgmtID() ManagementID
}

// MgmtTLVDecoderFunc is the function we use to decode management TLV from bytes
type MgmtTLVDecoderFunc func(data []byte) (ManagementTLV, error)

// default decoders for TLVs we implemented ourselves
var mgmtTLVDecoder = map[ManagementID]MgmtTLVDecoderFunc{
	IDDefaultDataSet: func(data []byte) (ManagementTLV, error) {
		r := bytes.NewReader(data)
		tlv := &DefaultDataSetTLV{}
		if err := binary.Read(r, binary.BigEndian, tlv); err != nil {
			return nil, err
		}
		return tlv, nil
	},
	IDCurrentDataSet: func(data []byte) (ManagementTLV, error) {
		r := bytes.NewReader(data)
		tlv := &CurrentDataSetTLV{}
		if err := binary.Read(r, binary.BigEndian, tlv); err != nil {
			return nil, err
		}
		return tlv, nil
	},
	IDParentDataSet: func(data []byte) (ManagementTLV, error) {
		r := bytes.NewReader(data)
		tlv := &ParentDataSetTLV{}
		if err := binary.Read(r, binary.BigEndian, tlv); err != nil {
			return nil, err
		}
		return tlv, nil
	},
	IDPortStatsNP: func(data []byte) (ManagementTLV, error) {
		r := bytes.NewReader(data)
		tlv := &PortStatsNPTLV{}
		if err := binary.Read(r, binary.BigEndian, &tlv.ManagementTLVHead); err != nil {
			return nil, err
		}
		if err := binary.Read(r, binary.BigEndian, &tlv.PortIdentity); err != nil {
			return nil, err
		}
		// fun part that cost me few hours, this is sent over wire as LittlEndian, while EVERYTHING ELSE is BigEndian.
		if err := binary.Read(r, binary.LittleEndian, &tlv.PortStats); err != nil {
			return nil, err
		}
		return tlv, nil
	},
	IDTimeStatusNP: func(data []byte) (ManagementTLV, error) {
		r := bytes.NewReader(data)
		tlv := &TimeStatusNPTLV{}
		if err := binary.Read(r, binary.BigEndian, tlv); err != nil {
			return nil, err
		}
		return tlv, nil
	},
}

// RegisterMgmtTLVDecoder registers function we'll use to decode particular custom management TLV.
// IEEE1588-2019 specifies that range C000 – DFFF should be used for implementation-specific identifiers,
// and E000 – FFFE is to be assigned by alternate PTP Profile.
func RegisterMgmtTLVDecoder(id ManagementID, decoder MgmtTLVDecoderFunc) {
	mgmtTLVDecoder[id] = decoder
}

// CurrentDataSetTLV Spec Table 84 - CURRENT_DATA_SET management TLV data field
type CurrentDataSetTLV struct {
	ManagementTLVHead

	StepsRemoved     uint16
	OffsetFromMaster TimeInterval
	MeanPathDelay    TimeInterval
}

// DefaultDataSetTLV Spec Table 69 - DEFAULT_DATA_SET management TLV data field
type DefaultDataSetTLV struct {
	ManagementTLVHead

	SoTSC         uint8
	Reserved0     uint8
	NumberPorts   uint16
	Priority1     uint8
	ClockQuality  ClockQuality
	Priority2     uint8
	ClockIdentity ClockIdentity
	DomainNumber  uint8
	Reserved1     uint8
}

// ParentDataSetTLV Spec Table 85 - PARENT_DATA_SET management TLV data field
type ParentDataSetTLV struct {
	ManagementTLVHead

	ParentPortIdentity                    PortIdentity
	PS                                    uint8
	Reserved                              uint8
	ObservedParentOffsetScaledLogVariance uint16
	ObservedParentClockPhaseChangeRate    uint32
	GrandmasterPriority1                  uint8
	GrandmasterClockQuality               ClockQuality
	GrandmasterPriority2                  uint8
	GrandmasterIdentity                   ClockIdentity
}

// CurrentDataSetRequest prepares request packet for CURRENT_DATA_SET request
func CurrentDataSetRequest() *Management {
	headerSize := uint16(binary.Size(ManagementMsgHead{}))
	size := uint16(binary.Size(CurrentDataSetTLV{}))
	tlvHeadSize := uint16(binary.Size(TLVHead{}))
	return &Management{
		ManagementMsgHead: ManagementMsgHead{
			Header: Header{
				SdoIDAndMsgType:    NewSdoIDAndMsgType(MessageManagement, 0),
				Version:            Version,
				MessageLength:      headerSize + size,
				SourcePortIdentity: identity,
				LogMessageInterval: MgmtLogMessageInterval,
			},
			TargetPortIdentity:   DefaultTargetPortIdentity,
			StartingBoundaryHops: 0,
			BoundaryHops:         0,
			ActionField:          GET,
		},
		TLV: &CurrentDataSetTLV{
			ManagementTLVHead: ManagementTLVHead{
				TLVHead: TLVHead{
					TLVType:     TLVManagement,
					LengthField: size - tlvHeadSize,
				},
				ManagementID: IDCurrentDataSet,
			},
		},
	}
}

// DefaultDataSetRequest prepares request packet for DEFAULT_DATA_SET request
func DefaultDataSetRequest() *Management {
	headerSize := uint16(binary.Size(ManagementMsgHead{}))
	size := uint16(binary.Size(DefaultDataSetTLV{}))
	tlvHeadSize := uint16(binary.Size(TLVHead{}))
	return &Management{
		ManagementMsgHead: ManagementMsgHead{
			Header: Header{
				SdoIDAndMsgType:    NewSdoIDAndMsgType(MessageManagement, 0),
				Version:            Version,
				MessageLength:      headerSize + size,
				SourcePortIdentity: identity,
				LogMessageInterval: MgmtLogMessageInterval,
			},
			TargetPortIdentity:   DefaultTargetPortIdentity,
			StartingBoundaryHops: 0,
			BoundaryHops:         0,
			ActionField:          GET,
		},
		TLV: &DefaultDataSetTLV{
			ManagementTLVHead: ManagementTLVHead{
				TLVHead: TLVHead{
					TLVType:     TLVManagement,
					LengthField: size - tlvHeadSize,
				},
				ManagementID: IDDefaultDataSet,
			},
		},
	}
}

// ParentDataSetRequest prepares request packet for PARENT_DATA_SET request
func ParentDataSetRequest() *Management {
	headerSize := uint16(binary.Size(ManagementMsgHead{}))
	size := uint16(binary.Size(ParentDataSetTLV{}))
	tlvHeadSize := uint16(binary.Size(TLVHead{}))
	return &Management{
		ManagementMsgHead: ManagementMsgHead{
			Header: Header{
				SdoIDAndMsgType:    NewSdoIDAndMsgType(MessageManagement, 0),
				Version:            Version,
				MessageLength:      headerSize + size,
				SourcePortIdentity: identity,
				LogMessageInterval: MgmtLogMessageInterval,
			},
			TargetPortIdentity:   DefaultTargetPortIdentity,
			StartingBoundaryHops: 0,
			BoundaryHops:         0,
			ActionField:          GET,
		},
		TLV: &ParentDataSetTLV{
			ManagementTLVHead: ManagementTLVHead{
				TLVHead: TLVHead{
					TLVType:     TLVManagement,
					LengthField: size - tlvHeadSize,
				},
				ManagementID: IDParentDataSet,
			},
		},
	}
}
