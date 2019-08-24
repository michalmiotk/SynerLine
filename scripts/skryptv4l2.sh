#!/bin/bash
#uruchomienie sudomodprobe ktore trzeba robic zeby uruchomic kamere
sudo modprobe bcm2835-v4l2
v4l2-ctl -p 90
