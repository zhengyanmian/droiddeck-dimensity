#!/usr/bin/env python3
p = "/root/build/panvk-kbase-android/work/mesa/src/panfrost/lib/kmod/kbase_kmod.c"
src = open(p).read()

old = """   struct dma_heap_allocation_data alloc = {
      .len = ALIGN_POT(size, page_size),
      .fd_flags = O_RDWR | O_CLOEXEC,
   };

   if (ioctl(kbase_dev->dma_heap_fd, DMA_HEAP_IOCTL_ALLOC, &alloc)) {
      mesa_loge("kbase: DMA_HEAP_IOCTL_ALLOC failed: %s", strerror(errno));
      return NULL;
   }

   struct pan_kmod_bo *bo = kbase_kmod_import_dmabuf(
      dev, NULL, alloc.fd, alloc.len, kmod_flags, false);
   close(alloc.fd);
   return bo;
}"""

new = """   /* MT6993 allocation strategy:
    *  - dma_heap/mtk_mm imports map into the GPU MMU but take a
    *    PERMISSION_FAULT_3 (0xcb) on the first GPU write.
    *  - legacy MEM_ALLOC memory is GPU-invisible (TRANSLATION_FAULT_3).
    *  - gralloc buffers (AHardwareBuffer) are the only proven-GPU-working
    *    source: every AHB round-trip renders and reads back correctly.
    * So allocate an AHB (BLOB when gralloc accepts it, square RGBA8
    * otherwise), grab its dma-buf fd from the native handle and run it
    * through the proven 40B import path.  PANVK_ALLOC_VIA_DMAHEAP=1
    * restores the dma-heap route for experiments. */
   if (!getenv("PANVK_ALLOC_VIA_DMAHEAP")) {
      uint64_t alloc_len = ALIGN_POT(size, page_size);
      AHardwareBuffer *ahb = NULL;
      const native_handle_t *nh = NULL;

      AHardwareBuffer_Desc blob = {
         .width = alloc_len, .height = 1, .layers = 1,
         .format = AHARDWAREBUFFER_FORMAT_BLOB,
         .usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
                  AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT,
      };
      if (AHardwareBuffer_isSupported(&blob) &&
          AHardwareBuffer_allocate(&blob, &ahb) == 0 && ahb)
         mesa_logi("kbase: AHB BLOB alloc %llu bytes ok",
                   (unsigned long long)alloc_len);

      if (!ahb) {
         uint32_t px = (uint32_t)((alloc_len + 3) / 4);
         uint32_t w = 1;
         while (w * w < px)
            w <<= 1;
         AHardwareBuffer_Desc sq = {
            .width = w, .height = w, .layers = 1,
            .format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            .usage = AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT |
                     AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE,
         };
         if (AHardwareBuffer_allocate(&sq, &ahb) == 0 && ahb)
            mesa_logi("kbase: AHB RGBA8 alloc %ux%u ok", w, w);
      }

      if (!ahb) {
         mesa_loge("kbase: AHB allocation failed for %llu bytes",
                   (unsigned long long)alloc_len);
         return NULL;
      }

      nh = AHardwareBuffer_getNativeHandle(ahb);
      if (nh) {
         for (int i = 0; i < nh->numFds; i++) {
            struct pan_kmod_bo *bo = kbase_kmod_import_dmabuf(
               dev, NULL, nh->data[i], alloc_len, kmod_flags, false);
            if (bo) {
               /* import holds its own dmabuf ref; drop the AHB */
               AHardwareBuffer_release(ahb);
               return bo;
            }
         }
      }
      AHardwareBuffer_release(ahb);
      mesa_loge("kbase: AHB import failed on all fds; falling back to dma-heap");
   }

   struct dma_heap_allocation_data alloc = {
      .len = ALIGN_POT(size, page_size),
      .fd_flags = O_RDWR | O_CLOEXEC,
   };

   if (ioctl(kbase_dev->dma_heap_fd, DMA_HEAP_IOCTL_ALLOC, &alloc)) {
      mesa_loge("kbase: DMA_HEAP_IOCTL_ALLOC failed: %s", strerror(errno));
      return NULL;
   }

   struct pan_kmod_bo *bo = kbase_kmod_import_dmabuf(
      dev, NULL, alloc.fd, alloc.len, kmod_flags, false);
   close(alloc.fd);
   return bo;
}"""

assert old in src, "alloc_dmabuf body not found"
src = src.replace(old, new)

# include AHB header if missing
if "#include <android/hardware_buffer.h>" not in src:
    src = src.replace("#include <fcntl.h>", "#include <fcntl.h>\n#include <android/hardware_buffer.h>", 1)

open(p, "w").write(src)
print("patched AHB-based allocation")
