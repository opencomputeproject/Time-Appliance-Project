package header

import (
	"bytes"
	"encoding/binary"
	"os"
	"testing"

	"github.com/stretchr/testify/require"
)

func TestFirmwareHeader(t *testing.T) {
	b := []byte{'O', 'C', 'P', 'C', 0, 0x10, 0, 0x10, 0, 0, 0, 4, 0, 0, 0, 0}
	tmp, err := os.CreateTemp("", "fw.bin")
	require.NoError(t, err)
	defer os.Remove(tmp.Name())
	err = os.WriteFile(tmp.Name(), []byte{'0', '0', '0', '0'}, 0644)

	c := &Config{
		Apply:            false,
		InputPath:        tmp.Name(),
		OutputPath:       "",
		VendorId:         0x10,
		DeviceId:         0x10,
		HardwareRevision: 0x00,
		InputFile:        nil,
		OutputFile:       nil,
	}

	err = OpenFiles(c)
	require.NoError(t, err)

	hdr, err := PrepareHeader(c)
	require.NoError(t, err)

	h := new(bytes.Buffer)
	binary.Write(h, binary.BigEndian, hdr)

	require.Equal(t, b, h.Bytes())

	crc, err := CalcCRC(c)

	require.NoError(t, err)
	require.Equal(t, uint16(0x3f1b), crc)
	CloseFiles(c)
}

func TestFirmwareReadHeader(t *testing.T) {
	b := []byte{'O', 'C', 'P', 'C', 0, 0x10, 0, 0x10, 0, 0, 0, 4, 0, 0, 0x3f, 0x1b, '0', '0', '0', '0'}
	tmp, err := os.CreateTemp("", "fw.bin")
	require.NoError(t, err)
	defer os.Remove(tmp.Name())
	err = os.WriteFile(tmp.Name(), b, 0644)

	c := &Config{
		Apply:            false,
		InputPath:        tmp.Name(),
		OutputPath:       "",
		VendorId:         0x10,
		DeviceId:         0x10,
		HardwareRevision: 0x00,
		InputFile:        nil,
		OutputFile:       nil,
	}

	hdr := &Header{
		Magic:            [4]byte{'O', 'C', 'P', 'C'},
		VendorId:         uint16(c.VendorId),
		DeviceId:         uint16(c.DeviceId),
		ImageSize:        uint32(4),
		HardwareRevision: uint16(c.HardwareRevision),
		CRC:			  uint16(0x3f1b),
	}
	err = OpenFiles(c)
	require.NoError(t, err)

	nHdr, err := ReadHeader(c)
	require.NoError(t, err)

	require.Equal(t, hdr, nHdr)

	CloseFiles(c)
}

func TestFirmwareReadNoHeader(t *testing.T) {
	b := []byte{'0', '0', '0', '0'}
	tmp, err := os.CreateTemp("", "fw.bin")
	require.NoError(t, err)
	defer os.Remove(tmp.Name())
	err = os.WriteFile(tmp.Name(), b, 0644)

	c := &Config{
		Apply:            false,
		InputPath:        tmp.Name(),
		OutputPath:       "",
		VendorId:         0x10,
		DeviceId:         0x10,
		HardwareRevision: 0x00,
		InputFile:        nil,
		OutputFile:       nil,
	}

	err = OpenFiles(c)
	require.NoError(t, err)

	nHdr, err := ReadHeader(c)
	require.Error(t, err)
	require.Nil(t, nHdr)
	require.Equal(t, err, ErrNoHeader)

	CloseFiles(c)

	for i := 0; i < 3; i++ {
		b = append(b, b...)
	}
	err = os.WriteFile(tmp.Name(), b, 0644)
	c.InputFile = nil

	err = OpenFiles(c)
	require.NoError(t, err)

	nHdr, err = ReadHeader(c)
	require.Error(t, err)
	require.Nil(t, nHdr)
	require.Equal(t, err, ErrNoHeader)

	CloseFiles(c)
}
