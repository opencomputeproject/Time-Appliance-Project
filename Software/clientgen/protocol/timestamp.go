/*
Copyright (c) Facebook, Inc. and its affiliates.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package protocol

// Here we have basic HW and SW timestamping support

import (
	"encoding/binary"
	"fmt"
	"net"
	"time"
	"unsafe"

	"golang.org/x/sys/unix"
)

const (
	// HWTSTAMP_TX_ON int 1
	hwtstampTXON = 0x00000001
	// HWTSTAMP_FILTER_ALL int 1
	hwtstampFilterAll = 0x00000001
	// Control is a socket control message containing TX/RX timestamp
	// If the read fails we may endup with multiple timestamps in the buffer
	// which is best to read right away
	ControlSizeBytes = 128
	// ptp packets usually up to 66 bytes
	PayloadSizeBytes = 128
	// look only for X sequential TS
	maxTXTS = 100
	// Socket Control Message Header Offset on Linux
	SocketControlMessageHeaderOffset = 16
)

const (
	// HWTIMESTAMP is a hardware timestamp
	HWTIMESTAMP = "hardware"
	// SWTIMESTAMP is a software timestmap
	SWTIMESTAMP = "software"
)

var timestamping = unix.SO_TIMESTAMPING_NEW

func init() {
	// if kernel is older than 5, it doesn't support unix.SO_TIMESTAMPING_NEW
	var uname unix.Utsname
	if err := unix.Uname(&uname); err == nil {
		if uname.Release[0] < '5' {
			// reading such timestamps on 32bit machines will not work, but we can't support everything
			timestamping = unix.SO_TIMESTAMPING
		}
	}
}

func Timestamping() int {
	return timestamping
}

// Ifreq is a struct for ioctl ethernet manipulation syscalls.
type ifreq struct {
	name [unix.IFNAMSIZ]byte
	data uintptr
}

type hwtstampСonfig struct {
	flags    int
	txType   int
	rxFilter int
}

// ConnFd returns file descriptor of a connection
func ConnFd(conn *net.UDPConn) (int, error) {
	sc, err := conn.SyscallConn()
	if err != nil {
		return -1, err
	}
	var intfd int
	err = sc.Control(func(fd uintptr) {
		intfd = int(fd)
	})
	if err != nil {
		return -1, err
	}
	return intfd, nil
}

func IoctlTimestamp(fd int, ifname string) error {
	hw := &hwtstampСonfig{
		flags:    0,
		txType:   hwtstampTXON,
		rxFilter: hwtstampFilterAll,
	}
	i := &ifreq{data: uintptr(unsafe.Pointer(hw))}
	copy(i.name[:unix.IFNAMSIZ-1], ifname)

	if _, _, errno := unix.Syscall(unix.SYS_IOCTL, uintptr(fd), unix.SIOCSHWTSTAMP, uintptr(unsafe.Pointer(i))); errno != 0 {
		return fmt.Errorf("failed to run ioctl SIOCSHWTSTAMP: %d", errno)
	}
	return nil
}

// EnableHWTimestampsSocket enables HW timestamps on the socket
func EnableHWTimestampsSocket(connFd int, iface string) error {
	if err := IoctlTimestamp(connFd, iface); err != nil {
		return err
	}

	// Enable hardware timestamp capabilities on socket
	flags := unix.SOF_TIMESTAMPING_TX_HARDWARE |
		unix.SOF_TIMESTAMPING_RX_HARDWARE |
		unix.SOF_TIMESTAMPING_RAW_HARDWARE |
		unix.SOF_TIMESTAMPING_OPT_TSONLY // Makes the kernel return the timestamp as a cmsg alongside an empty packet, as opposed to alongside the original packet.
	// Allow reading of HW timestamps via socket
	if err := unix.SetsockoptInt(connFd, unix.SOL_SOCKET, timestamping, flags); err != nil {
		return err
	}

	if err := unix.SetsockoptInt(connFd, unix.SOL_SOCKET, unix.SO_SELECT_ERR_QUEUE, 1); err != nil {
		return err
	}
	return nil
}

// EnableSWTimestampsSocket enables SW timestamps on the socket
func EnableSWTimestampsSocket(connFd int) error {
	flags := unix.SOF_TIMESTAMPING_TX_SOFTWARE |
		unix.SOF_TIMESTAMPING_RX_SOFTWARE |
		unix.SOF_TIMESTAMPING_SOFTWARE |
		unix.SOF_TIMESTAMPING_OPT_TSONLY // Makes the kernel return the timestamp as a cmsg alongside an empty packet, as opposed to alongside the original packet.
	// Allow reading of SW timestamps via socket
	if err := unix.SetsockoptInt(connFd, unix.SOL_SOCKET, timestamping, flags); err != nil {
		return err
	}

	if err := unix.SetsockoptInt(connFd, unix.SOL_SOCKET, unix.SO_SELECT_ERR_QUEUE, 1); err != nil {
		return err
	}
	return nil
}

// byteToTime converts LittleEndian bytes into a timestamp
func byteToTime(data []byte) (time.Time, error) {
	// __kernel_timespec from linux/time_types.h
	// can't use unix.Timespec which is old timespec that uses 32bit ints on 386 platform.
	sec := int64(binary.LittleEndian.Uint64(data[0:8]))
	nsec := int64(binary.LittleEndian.Uint64(data[8:]))
	return time.Unix(sec, nsec), nil
}

// SocketControlMessageTimestamp is a very optimised version of ParseSocketControlMessage
// https://github.com/golang/go/blob/2ebe77a2fda1ee9ff6fd9a3e08933ad1ebaea039/src/syscall/sockcmsg_unix.go#L40
// which only parses the timestamp message type.
func SocketControlMessageTimestamp(b []byte) (time.Time, error) {
	mlen := 0
	for i := 0; i < len(b); i += mlen {
		h := (*unix.Cmsghdr)(unsafe.Pointer(&b[i]))
		mlen = int(h.Len)

		// depending on the kernel version, when we ask for SO_TIMESTAMPING_NEW we still might get messages with type SO_TIMESTAMPING
		if h.Level == unix.SOL_SOCKET && int(h.Type) == unix.SO_TIMESTAMPING_NEW || int(h.Type) == unix.SO_TIMESTAMPING {
			return scmDataToTime(b[i+SocketControlMessageHeaderOffset : i+mlen])
		}
	}
	return time.Time{}, fmt.Errorf("failed to find timestamp in socket control message")
}

/*
scmDataToTime parses SocketControlMessage Data field into time.Time.
The structure can return up to three timestamps. This is a legacy
feature. Only one field is non-zero at any time. Most timestamps
are passed in ts[0]. Hardware timestamps are passed in ts[2].
*/
func scmDataToTime(data []byte) (ts time.Time, err error) {
	// 2 x 64bit ints
	size := 16
	// first, try to use hardware timestamps
	ts, err = byteToTime(data[size*2 : size*3])
	if err != nil {
		return ts, err
	}
	// if hw timestamps aren't present, use software timestamps
	// we can't use ts.IsZero because for some crazy reason timestamp parsed using time.Unix()
	// reports IsZero() == false, even if seconds and nanoseconds are zero.
	if ts.UnixNano() == 0 {
		ts, err = byteToTime(data[0:size])
		if err != nil {
			return ts, err
		}
		if ts.UnixNano() == 0 {
			return ts, fmt.Errorf("got zero timestamp")
		}
	}

	return ts, nil
}

func waitForHWTS(connFd int) error {
	// Wait until TX timestamp is ready
	fds := []unix.PollFd{{Fd: int32(connFd), Events: unix.POLLPRI, Revents: 0}}
	if _, err := unix.Poll(fds, 1); err != nil { // Timeout is 1 ms
		return err
	}
	return nil
}

// recvoob receives only OOB message from the socket
// This is used for TX timestamp read of MSG_ERRQUEUE where we couldn't care less about other data.
// This is partially based on Recvmsg
// https://github.com/golang/go/blob/2ebe77a2fda1ee9ff6fd9a3e08933ad1ebaea039/src/syscall/syscall_linux.go#L647
func recvoob(connFd int, oob []byte) (oobn int, err error) {
	var msg unix.Msghdr
	msg.Control = &oob[0]
	msg.SetControllen(len(oob))
	_, _, e1 := unix.Syscall(unix.SYS_RECVMSG, uintptr(connFd), uintptr(unsafe.Pointer(&msg)), uintptr(unix.MSG_ERRQUEUE))
	if e1 != 0 {
		return 0, e1
	}
	return int(msg.Controllen), nil
}

// ReadTXtimestampBuf returns HW TX timestamp, needs to be provided 2 buffers which all can be re-used after ReadTXtimestampBuf finishes.
func ReadTXtimestampBuf(connFd int, oob, toob []byte) (time.Time, int, error) {
	// Accessing hw timestamp
	var boob int

	txfound := false

	// Sometimes we end up with more than 1 TX TS in the buffer.
	// We need to empty it and completely otherwise we end up with a shifted queue read:
	// Sync is out -> read TS from the previous Sync
	// Because we always perform at least 2 tries we start with 0 so on success we are at 1.
	attempts := 0
	for ; attempts < maxTXTS; attempts++ {
		if !txfound {
			// Wait for the poll event, ignore the error
			_ = waitForHWTS(connFd)
		}

		tboob, err := recvoob(connFd, toob)
		if err != nil {
			// We've already seen the valid TX TS and now we have an empty queue.
			// All good
			if txfound {
				break
			}
			// Keep looking for a valid TX TS
			continue
		}
		// We found a valid TX TS. Still check more if there is a newer one
		txfound = true
		boob = tboob
		copy(oob, toob)
	}

	if !txfound {
		return time.Time{}, attempts, fmt.Errorf("no TX timestamp found after %d tries", maxTXTS)
	}
	timestamp, err := SocketControlMessageTimestamp(oob[:boob])
	return timestamp, attempts, err
}

// ReadTXtimestamp returns HW TX timestamp
func ReadTXtimestamp(connFd int) (time.Time, int, error) {
	// Accessing hw timestamp
	oob := make([]byte, ControlSizeBytes)
	// TMP buffers
	toob := make([]byte, ControlSizeBytes)

	return ReadTXtimestampBuf(connFd, oob, toob)
}

// ReadPacketWithRXTimestamp returns byte packet and HW RX timestamp
func ReadPacketWithRXTimestamp(connFd int) ([]byte, unix.Sockaddr, time.Time, error) {
	// Accessing hw timestamp
	buf := make([]byte, PayloadSizeBytes)
	oob := make([]byte, ControlSizeBytes)

	bbuf, sa, t, err := ReadPacketWithRXTimestampBuf(connFd, buf, oob)
	return buf[:bbuf], sa, t, err
}

// ReadPacketWithRXTimestampBuf writes byte packet into provide buffer buf, and returns number of bytes copied to the buffer, client ip and HW RX timestamp.
// oob buffer can be reaused after ReadPacketWithRXTimestampBuf call.
func ReadPacketWithRXTimestampBuf(connFd int, buf, oob []byte) (int, unix.Sockaddr, time.Time, error) {
	bbuf, boob, _, saddr, err := unix.Recvmsg(connFd, buf, oob, 0)
	if err != nil {
		return 0, nil, time.Time{}, fmt.Errorf("failed to read timestamp: %v", err)
	}

	timestamp, err := SocketControlMessageTimestamp(oob[:boob])
	return bbuf, saddr, timestamp, err
}

// IPToSockaddr converts IP + port into a socket address
// Somewhat copy from https://github.com/golang/go/blob/16cd770e0668a410a511680b2ac1412e554bd27b/src/net/ipsock_posix.go#L145
func IPToSockaddr(ip net.IP, port int) unix.Sockaddr {
	if ip.To4() != nil {
		sa := &unix.SockaddrInet4{Port: port}
		copy(sa.Addr[:], ip.To4())
		return sa
	} else {
		sa := &unix.SockaddrInet6{Port: port}
		copy(sa.Addr[:], ip.To16())
		return sa
	}
}

// SockaddrToIP converts socket address to an IP
// Somewhat copy from https://github.com/golang/go/blob/658b5e66ecbc41a49e6fb5aa63c5d9c804cf305f/src/net/udpsock_posix.go#L15
func SockaddrToIP(sa unix.Sockaddr) net.IP {
	switch sa := sa.(type) {
	case *unix.SockaddrInet4:
		return sa.Addr[0:]
	case *unix.SockaddrInet6:
		return sa.Addr[0:]
	}
	return nil
}
