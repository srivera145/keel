#!/bin/sh
# Build a bootable initramfs whose /init is KEEL OS.
#   boot:  qemu-system-x86_64 -kernel <vmlinuz> -initrd build/keel-initramfs.cpio.gz -append "vga=0x318"
# Any Linux kernel with a framebuffer (simpledrm/efifb/vesafb), evdev and
# devtmpfs works; KEEL mounts /proc, /sys, /dev itself and drives /dev/fb0.
set -e
cd "$(dirname "$0")/.."
[ -x build/keelos-static ] || make static
root=build/initramfs
rm -rf "$root"
mkdir -p "$root/dev" "$root/proc" "$root/sys" "$root/tmp" "$root/usr/share/fonts/keel"
cp build/keelos-static "$root/init"
# Typefaces are taken from the build machine (Inter preferred, DejaVu fallback).
for f in Inter-Regular.otf Inter-Medium.otf Inter-SemiBold.otf Inter-Bold.otf Inter-Light.otf \
         InterDisplay-Light.otf InterDisplay-Bold.otf InterDisplay-Black.otf DejaVuSans.ttf DejaVuSans-Bold.ttf; do
    p=$(find /usr/share/fonts -name "$f" 2>/dev/null | head -1)
    [ -n "$p" ] && cp "$p" "$root/usr/share/fonts/keel/"
done
(cd "$root" && find . | cpio -o -H newc --quiet | gzip -9) > build/keel-initramfs.cpio.gz
ls -la build/keel-initramfs.cpio.gz
