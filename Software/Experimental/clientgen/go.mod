module clientgen

go 1.16

replace clientgenlib => ./clientgenlib

replace github.com/facebook/time/ptp/protocol => ./protocol

require (
	clientgenlib v0.0.0-00010101000000-000000000000
	github.com/facebook/time v0.0.0-20211214114534-9de45ff94725 // indirect
	github.com/sirupsen/logrus v1.8.1
	golang.org/x/sync v0.0.0-20210220032951-036812b2e83c
)
