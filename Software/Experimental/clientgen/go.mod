module clientgen

go 1.16

replace clientgenlib => ./clientgenlib

replace github.com/facebookincubator/ptp/protocol => ./protocol

require (
	clientgenlib v0.0.0-00010101000000-000000000000
	github.com/sirupsen/logrus v1.8.1
	golang.org/x/sync v0.0.0-20210220032951-036812b2e83c
)
