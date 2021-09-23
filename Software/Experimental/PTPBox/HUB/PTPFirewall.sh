#!/bin/bash

firewall-cmd --zone=public --add-port=319/udp --add-port=320/udp
firewall-cmd --permanent --zone=public --add-port=319/udp --add-port=320/udp
