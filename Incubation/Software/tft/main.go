package main

import (
	"flag"
	"fmt"

	h "github.com/opencomputeproject/Time-Appliance-Project/Software/tft/header"
	log "github.com/sirupsen/logrus"
)

func printHeader(hdr *h.Header) {
	fmt.Println("Input file has header:")
	fmt.Printf("PCI Vendor ID: 0x%04x\n", hdr.VendorId)
	fmt.Printf("PCI Device ID: 0x%04x\n", hdr.DeviceId)
	fmt.Printf("PCI HW Revision ID: 0x%04x\n", hdr.HardwareRevision)
	fmt.Printf("Image CRC16: 0x%04x\n", hdr.CRC)
	fmt.Printf("Image size: %d\n", hdr.ImageSize)
}

func main() {
	c := &h.Config{}

	flag.BoolVar(&c.Apply, "apply", false, "Create new firmware file with the header in the beginning")
	flag.StringVar(&c.InputPath, "input", "", "Path to raw firmware file")
	flag.StringVar(&c.OutputPath, "output", "", "Path to firmware file with header (will be overwritten")
	flag.IntVar(&c.VendorId, "vendor", 0, "PCI VEN_ID to add to header")
	flag.IntVar(&c.DeviceId, "device", 0, "PCI DEV_ID to add to header")
	flag.IntVar(&c.HardwareRevision, "hw", 0, "PCI REV_ID (Hardware revision) to add to header")
	flag.Parse()

	if err := h.CheckConfig(c); err != nil {
		log.Fatal(err)
	}

	if err := h.OpenFiles(c); err != nil {
		log.Fatal(err)
	}
	defer h.CloseFiles(c)

	oldHdr, err := h.ReadHeader(c)
	if err == nil {
		fmt.Println("Input file has header:")
		printHeader(oldHdr)
		if c.Apply {
			fmt.Println("Image header will be overwritten with new values")
		}
	}

	hdr, err := h.PrepareHeader(c)
	if err != nil {
		log.Fatal(err)
	}

	if err := h.WriteHeader(c, hdr); err != nil {
		log.Fatal(err)
	}

	hdr.CRC, err = h.CalcCRC(c)
	if err != nil {
		log.Fatal(err)
	}

	// we have to rewrite header as CRC is calculated
	if err := h.WriteHeader(c, hdr); err != nil {
		log.Fatal(err)
	}

	fmt.Println("New header:")
	printHeader(hdr)

}
