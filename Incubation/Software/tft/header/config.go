package header

import (
	"errors"
	"os"
)

type Config struct {
	Apply            bool
	InputPath        string
	OutputPath       string
	VendorId         int
	DeviceId         int
	HardwareRevision int
	InputFile        *os.File
	OutputFile       *os.File
}

// CheckConfig checks config structure for valid values
// PCI Vendor ID, Device ID should fit into uint16 and be positive
// PCI Hardware Revision should be any uint16
func CheckConfig(c *Config) error {
	if c.InputPath == "" {
		return errors.New("No input file provided")
	}

	if c.Apply && c.OutputPath == "" {
		return errors.New("No output file provided")
	}

	if c.Apply && (c.VendorId <= 0 || c.VendorId > 65535) {
		return errors.New("Empty or incorrect PCI Vendor ID specified")
	}

	if c.Apply && (c.DeviceId <= 0 || c.DeviceId > 65535) {
		return errors.New("Empty or incorrect PCI Device ID specified")
	}

	if c.Apply && (c.HardwareRevision < 0 || c.HardwareRevision > 65535) {
		return errors.New("Incorrect PCI Device Revision ID specified")
	}

	return nil
}

// OpenFiles actually opens files provided in config. Returns any error
func OpenFiles(c *Config) error {
	var err error
	c.InputFile, err = os.Open(c.InputPath)
	if err != nil {
		return err
	}

	if c.Apply {
		c.OutputFile, err = os.Create(c.OutputPath)
		return err
	}

	return nil
}

// CloseFiles closes previously opened files
func CloseFiles(c *Config) {
	c.InputFile.Close()
	if c.Apply {
		c.OutputFile.Close()
	}
}
