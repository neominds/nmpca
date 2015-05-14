#!/bin/bash

dmesg -C
sudo insmod ring_buf.ko
sudo insmod uart_driver_x86.ko
sudo insmod test.ko
sudo insmod pca_tdma_code.ko
sudo insmod pull_ring_buf.ko


