package header

import (
	"bytes"
	"encoding/binary"
	"io"

	crc "github.com/sigurn/crc16"
)

type Header struct {
	Magic            [4]byte
	VendorId         uint16
	DeviceId         uint16
	ImageSize        uint32
	HardwareRevision uint16
	CRC              uint16
}

func firmwareImageSize(c *Config) (uint32, error) {
	stat, err := c.InputFile.Stat()
	if err != nil {
		return 0, err
	}

	return uint32(stat.Size()), nil

}

// PrepareHeader creates header structure from Config values
func PrepareHeader(c *Config) (*Header, error) {
	imageSize, err := firmwareImageSize(c)
	if err != nil {
		return nil, err
	}

	hdr := &Header{
		Magic:            [4]byte{'O', 'C', 'T', 'C'},
		VendorId:         uint16(c.VendorId),
		DeviceId:         uint16(c.DeviceId),
		ImageSize:        imageSize,
		HardwareRevision: uint16(c.HardwareRevision),
	}

	return hdr, nil
}

// WriteHeader writes header structure at the beginning of the output file
func WriteHeader(c *Config, hdr *Header) error {
	if !c.Apply {
		return nil
	}
	h := new(bytes.Buffer)
	binary.Write(h, binary.BigEndian, hdr)
	_, err := c.OutputFile.WriteAt(h.Bytes(), 0)

	return err
}

// CalcCRC calculates CRC16 of input file and copies data to output file if specified
func CalcCRC(c *Config) (uint16, error) {
	crcTable := crc.MakeTable(crc.CRC16_ARC)
	buf := make([]byte, 16384)
	crc16 := uint16(0)

	n, err := c.InputFile.Read(buf)
	for ; n > 0 && (err == nil || err == io.EOF); n, err = c.InputFile.Read(buf) {
		crc16 = crc.Update(crc16, buf[:n], crcTable)
		if c.Apply {
			_, err = c.OutputFile.Write(buf[:n])
			if err != nil {
				break
			}
		}
	}

	if err != nil && err != io.EOF {
		return 0, err
	}

	crc16 = crc.Complete(crc16, crcTable)

	return crc16, nil
}
