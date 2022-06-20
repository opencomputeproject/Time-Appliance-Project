---
sidebar_position: 2
---

# Raspberry Pi Compute Module

**Currently a work in progress**

### Build

Build parameters,

```
CONFIG_NET_DEVLINK=y CONFIG_SERIAL_8250_NR_UARTS=8 CONFIG_IRQ_POLL=y
```

### Kconfig updates

Change `Kconfig` in `net` to have `DEVLINK` enabled

### Make

`make oldconfig` before doing `build` but after manually adding `DEVLINK`
