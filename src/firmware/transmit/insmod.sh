#!/bin/bash
sudo dmesg -C
sudo insmod ring_buf.ko
sudo insmod uart_driver_x86.ko
sudo insmod HIP.ko
sudo insmod post_ring_buf.ko
sudo insmod pca_tdma_code.ko


