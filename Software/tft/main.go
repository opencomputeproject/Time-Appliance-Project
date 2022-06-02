package main

import (
	"flag"

	h "github.com/opencomputeproject/Time-Appliance-Project/Software/tft/header"
	log "github.com/sirupsen/logrus"
)

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

	if err := h.WriteHeader(c, hdr); err != nil {
		log.Fatal(err)
	}

}
