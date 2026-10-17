#!/bin/sh
# Boot KEEL OS in QEMU:  tools/run-qemu.sh /path/to/vmlinuz [extra qemu args]
# Uses KVM when available; under pure emulation (TCG) expect a slideshow.
set -e
cd "$(dirname "$0")/.."
kernel=${1:?usage: run-qemu.sh /path/to/vmlinuz}
shift
accel=tcg
[ -w /dev/kvm ] && accel=kvm
exec qemu-system-x86_64 -machine accel=$accel -cpu max -smp 4 -m 2048 \
    -kernel "$kernel" -initrd build/keel-initramfs.cpio.gz \
    -append "vga=0x318 quiet loglevel=3 console=ttyS0" \
    -vga std -serial stdio "$@"
