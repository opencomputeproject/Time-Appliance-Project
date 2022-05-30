package header

import (
	"bytes"
	"encoding/binary"
	"io/ioutil"
	"os"
	"testing"

	"github.com/stretchr/testify/require"
)

func TestFirmwareHeader(t *testing.T) {
	b := []byte{'O', 'C', 'T', 'C', 0, 0x10, 0, 0x10, 0, 0, 0, 4, 0, 0, 0, 0}
	tmp, err := ioutil.TempFile("", "fw.bin")
	require.NoError(t, err)
	defer os.Remove(tmp.Name())
	err = ioutil.WriteFile(tmp.Name(), []byte{'0', '0', '0', '0'}, 0644)

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
	require.Equal(t, crc, uint16(0x1b1b))
	CloseFiles(c)
}
