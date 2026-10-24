#!/usr/bin/env python3
"""fix-memimport.py — patch Mesa kbase_kmod.c for MediaTek 40-byte MEM_IMPORT.

MT6993 (Dimensity 9500) CSF DDK uAPI 1.21:
  - MEM_IMPORT union is 40 bytes (upstream: 24) -> upstream cmd word misses
    the kernel dispatch table (ENOTTY).
  - out layout: flags@0x00 (input|NEED_MMAP), gpu_va cookie@0x08, va_pages@0x10.
  - vendor libGLES_mali ORs BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP (bit26) into flags.
  - ENOMEM can occur transiently; retry.
"""
import sys

path = "/root/build/panvk-kbase-android/work/mesa/src/panfrost/lib/kmod/kbase_kmod.c"
src = open(path).read()

if "MTK_KBASE_IOCTL_MEM_IMPORT" in src:
    print("already patched, skip")
    sys.exit(0)

old = """   int import_fd = kbase_bo->dmabuf_fd;
   union kbase_ioctl_mem_import req = {
      .in = {
         .flags = import_flags,
         .phandle = (uintptr_t)&import_fd,
         .type = BASE_MEM_IMPORT_TYPE_UMM,
      },
   };

   if (ioctl(dev->fd, KBASE_IOCTL_MEM_IMPORT, &req)) {
      mesa_loge("kbase: KBASE_IOCTL_MEM_IMPORT failed: %s", strerror(errno));
      goto err_close_dmabuf;
   }

   const uint64_t bo_size = req.out.va_pages * page_size;"""

new = """   int import_fd = kbase_bo->dmabuf_fd;
   /* MediaTek CSF DDK (MT6993, uAPI 1.21) extends the MEM_IMPORT union from
    * 24 to 40 bytes; the upstream 24-byte cmd word misses the kernel's
    * dispatch entry (ENOTTY).  Vendor libGLES_mali uses IOWR(0x80,22,40) and
    * ORs BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP (bit26) into the flags.  MTK out
    * layout: flags@0x00 (input|NEED_MMAP), gpu_va cookie@0x08, va_pages@0x10.
    * Transient ENOMEM observed on the device -> retry a few times.
    */
   union mtk_ioctl_mem_import {
      struct {
         __u64 flags;
         __u64 phandle;
         __u32 type;
         __u32 ext1;
         __u64 ext2;
         __u64 ext3;
      } in;
      struct {
         __u64 flags;
         __u64 gpu_va;
         __u64 va_pages;
         __u64 ext4;
         __u64 ext5;
      } out;
   } req;
#define MTK_KBASE_IOCTL_MEM_IMPORT _IOWR(0x80, 22, union mtk_ioctl_mem_import)
   memset(&req, 0, sizeof(req));
   req.in.flags = import_flags | BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP;
   req.in.phandle = (uintptr_t)&import_fd;
   req.in.type = BASE_MEM_IMPORT_TYPE_UMM;

   bool imported = false;
   for (int attempt = 0; attempt < 8; attempt++) {
      if (ioctl(dev->fd, MTK_KBASE_IOCTL_MEM_IMPORT, &req) == 0) {
         imported = true;
         break;
      }
      if (errno != ENOMEM)
         break;
      usleep(10000);
   }
   if (!imported) {
      mesa_loge("kbase: KBASE_IOCTL_MEM_IMPORT failed: %s", strerror(errno));
      goto err_close_dmabuf;
   }

   /* MTK out.va_pages is a VA-span counter, not this BO's page count; take
    * the real size from the dma-buf itself. */
   off_t dma_size = lseek(kbase_bo->dmabuf_fd, 0, SEEK_END);
   const uint64_t bo_size = (dma_size > 0)
      ? (uint64_t)dma_size
      : (uint64_t)req.out.va_pages * page_size;"""

if old not in src:
    print("ANCHOR NOT FOUND — dumping context for inspection")
    i = src.find("int import_fd = kbase_bo->dmabuf_fd;")
    print(src[i-200:i+700] if i >= 0 else "(no import_fd anchor either)")
    sys.exit(1)

src = src.replace(old, new)
with open(path, "w") as f:
    f.write(src)
print("PATCHED: MTK 40-byte MEM_IMPORT + SYNC26 flag + ENOMEM retry + lseek bo_size")
