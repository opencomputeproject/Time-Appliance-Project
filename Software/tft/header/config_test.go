package header

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestConfigCheck(t *testing.T) {
	c := &Config{
		Apply:            false,
		InputPath:        "",
		OutputPath:       "",
		VendorId:         0,
		DeviceId:         0,
		HardwareRevision: -1,
		InputFile:        nil,
		OutputFile:       nil,
	}

	err := CheckConfig(c)
	require.Error(t, err, "expect error on empty input filename")

	c.InputPath = "/tmp"
	c.Apply = true
	err = CheckConfig(c)
	require.Error(t, err, "expect error on empty output filename")

	c.OutputPath = "/tmp"
	err = CheckConfig(c)
	require.Error(t, err, "expect error on zero Vendor ID")

	c.VendorId = -1
	err = CheckConfig(c)
	require.Error(t, err, "expect error on negative Vendor ID")

	c.VendorId = 1313311
	err = CheckConfig(c)
	require.Error(t, err, "expect error - Vendor ID doesn't fit into uint16")

	c.VendorId = 10
	err = CheckConfig(c)
	require.Error(t, err, "expect error on zero Device ID")

	c.DeviceId = -1
	err = CheckConfig(c)
	require.Error(t, err, "expect error on negstive Device ID")

	c.DeviceId = 1313133
	err = CheckConfig(c)
	require.Error(t, err, "expect error - Device ID doesn't fit into uint16")

	c.DeviceId = 10
	err = CheckConfig(c)
	require.Error(t, err, "expect error on negative Hardware Revision ID")

	c.HardwareRevision = 131313
	err = CheckConfig(c)
	require.Error(t, err, "expect error - Hardware Revision ID doesn't fit into uint16")

	c.HardwareRevision = 13
	err = CheckConfig(c)
	require.NoError(t, err, "expect no error - Config is correctly populated")
}
