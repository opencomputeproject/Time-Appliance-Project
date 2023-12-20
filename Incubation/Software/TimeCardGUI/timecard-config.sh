#!/bin/sh

#if [ $(id -u) -ne 0 ]; then
#  printf "Script must be run as root. Try 'sudo timecard-config'\n"
#  exit 1
#fi

do_info() {
  whiptail --msgbox "\
This tool provides a straight-forward way of doing initial 
configuration of the Time Card. Although it can be run 
at any time, some of the options may have difficulties if 
you have heavily customised your installation.\
" 20 70 1
}

do_device() {
  lspci | grep 0x0400
}

do_clock_sources() {
  whiptail --msgbox "You will now be asked to enter a new password for the pi user" 20 60 1
  passwd pi &&
  whiptail --msgbox "Password changed successfully" 20 60 1
}

do_peripherals() {
  whiptail --yesno "What would you like to do with overscan" 20 60 2 \
    --yes-button Disable --no-button Enable 
  RET=$?
  if [ $RET -eq 0 ] || [ $RET -eq 1 ]; then
    ASK_TO_REBOOT=1
    set_overscan $RET;
  else
    return 1
  fi
}

do_sma_config() {
  whiptail --msgbox "You will now be asked to enter a new password for the pi user" 20 60 1
  passwd pi &&
  whiptail --msgbox "Password changed successfully" 20 60 1
}

do_signal_generators() {
  dpkg-reconfigure keyboard-configuration &&
  printf "Reloading keymap. This may take a short while\n" &&
  invoke-rc.d keyboard-setup start
}

do_other_settings() {
  dpkg-reconfigure locales
}

do_gnss_status() {
  # Stop if /boot is not a mountpoint
  if ! mountpoint -q /boot; then
    return 1
  fi
  MEMSPLIT=$(whiptail --menu "Set memory split" 20 60 10 \
    "224" "224MiB for ARM, 32MiB for VideoCore" \
    "192" "192MiB for ARM, 64MiB for VideoCore" \
    "128" "128MiB for ARM, 128MiB for VideoCore" \
    3>&1 1>&2 2>&3)
  if [ $? -eq 0 ]; then
    cp -a /boot/arm${MEMSPLIT}_start.elf /boot/start.elf
    sync
    ASK_TO_REBOOT=1
  fi
}

do_mac_status() {
  if [ -e /var/log/regen_ssh_keys.log ] && ! grep -q "^finished" /var/log/regen_ssh_keys.log; then
    whiptail --msgbox "Initial ssh key generation still running. Please wait and try again." 20 60 2
    return 1
  fi
  whiptail --yesno "Would you like the SSH server enabled or disabled?" 20 60 2 \
    --yes-button Enable --no-button Disable 
  RET=$?
  if [ $RET -eq 0 ]; then
    update-rc.d ssh enable &&
    invoke-rc.d ssh start &&
    whiptail --msgbox "SSH server enabled" 20 60 1
  elif [ $RET -eq 1 ]; then
    update-rc.d ssh disable &&
    whiptail --msgbox "SSH server disabled" 20 60 1
  else
    return $RET
  fi
}

do_timestampers() {
  whiptail --yesno "Should we boot straight to desktop?" 20 60 2
  RET=$?
  if [ $RET -eq 0 ]; then # yes
    update-rc.d lightdm enable 2
    sed /etc/lightdm/lightdm.conf -i -e "s/^#autologin-user=.*/autologin-user=pi/"
    ASK_TO_REBOOT=1
  elif [ $RET -eq 1 ]; then # no
    update-rc.d lightdm disable 2
    ASK_TO_REBOOT=1
  else # user hit escape
    return 1
  fi
}

do_update() {
  apt-get update &&
  apt-get install raspi-config &&
  printf "To start raspi-config again, do 'sudo raspi-config'. Now exiting\n"
  exit 0
}

do_finish() {
  if [ -e /etc/profile.d/raspi-config.sh ]; then
    rm -f /etc/profile.d/raspi-config.sh
    sed -i /etc/inittab \
      -e "s/^#\(.*\)#\s*RPICFG_TO_ENABLE\s*/\1/" \
      -e "/#\s*RPICFG_TO_DISABLE/d"
    telinit q
  fi
  if [ $ASK_TO_REBOOT -eq 1 ]; then
    whiptail --yesno "Would you like to reboot now?" 20 60 2
    if [ $? -eq 0 ]; then # yes
      sync
      reboot
    fi
  fi
  exit 0
}

while true; do
  FUN=$(whiptail --menu "Time Card Configurator" 20 80 12 --cancel-button Finish --ok-button Select \
    "info" "information about the Time Card Configurator" \
    "device" "enumeration of the time card device" \
    "clock_sources" "setting the clock source" \
    "peripherals" "show the pheripheral enumerations" \
    "sma_config" "configure the sma connectors" \
    "signal_generators" "configure the signal generators" \
    "other_settings" "set irig-b mode, utc_tai_offset, pci_delay, etc..." \
    "gnss_status" "show the status of the GNSS receiver" \
    "mac_status" "show the status of the mac" \
    "timestampers" "show the timestamper stack" \
    "update" "update the firmware of the Time Card" \
    3>&1 1>&2 2>&3)
  RET=$?
  if [ $RET -eq 1 ]; then
    do_finish
  elif [ $RET -eq 0 ]; then
    "do_$FUN" || whiptail --msgbox "There was an error running do_$FUN" 20 60 1
  else
    exit 1
  fi
done
