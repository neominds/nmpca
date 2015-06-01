#!/bin/bash


sudo rmmod pca_tdma_code.ko
sudo rmmod post_ring_buf.ko
sudo rmmod HIP.ko
sudo rmmod uart_driver_x86.ko
sudo rmmod ring_buf.ko
sudo dmesg > ./sendmsg
