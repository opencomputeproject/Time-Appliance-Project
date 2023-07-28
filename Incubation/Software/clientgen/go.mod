module clientgen

go 1.16

replace clientgenlib => ./clientgenlib

replace github.com/facebook/time/ptp/protocol => ./protocol

require (
	clientgenlib v0.0.0-00010101000000-000000000000
	github.com/fatih/color v1.13.0 // indirect
	github.com/golang/mock v1.6.0 // indirect
	github.com/jsimonetti/rtnetlink v0.0.0-20211213041634-9dff439f7e79 // indirect
	github.com/olekukonko/tablewriter v0.0.5 // indirect
	github.com/pkg/errors v0.9.1 // indirect
	github.com/sirupsen/logrus v1.8.1
	github.com/spf13/cobra v1.2.1 // indirect
	github.com/vtolstov/go-ioctl v0.0.0-20151206205506-6be9cced4810 // indirect
	golang.org/x/sync v0.0.0-20210220032951-036812b2e83c
)
