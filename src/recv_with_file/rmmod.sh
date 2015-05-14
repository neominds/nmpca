#!/bin/bash


sudo rmmod pull_ring_buf.ko
sudo rmmod pca_tdma_code.ko
sudo rmmod test.ko
sudo rmmod ring_buf.ko
sudo rmmod uart_driver_x86.ko
sudo dmesg > ./recvmsg
