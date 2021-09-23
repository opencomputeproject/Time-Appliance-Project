module clientgenlib

go 1.16

replace github.com/facebookincubator/ptp/protocol => ../protocol

require (
	github.com/facebookincubator/ptp/protocol v0.0.0-00010101000000-000000000000
	github.com/google/gopacket v1.1.19
	github.com/jamiealquiza/tachymeter v2.0.0+incompatible
	github.com/kpango/fastime v1.0.17
	github.com/sirupsen/logrus v1.8.1
	github.com/stretchr/testify v1.7.0
	golang.org/x/sync v0.0.0-20210220032951-036812b2e83c
	golang.org/x/sys v0.0.0-20210915083310-ed5796bab164
)
