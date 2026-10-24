/* kbase-gpuid.c — read GPU_ID and core props directly from /dev/mali0 (no root)
 * Reuses the kbase UAPI header from the patched Mesa tree.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>

#include "kbase_uapi.h"

static uint64_t
prop_get(const uint8_t *buf, size_t buf_size, uint32_t target_key, uint64_t default_val)
{
   size_t offset = 0;
   while (offset + 4 <= buf_size) {
      uint32_t hdr;
      memcpy(&hdr, buf + offset, 4);
      offset += 4;
      uint32_t key = hdr >> 2;
      uint32_t size_code = hdr & 0x3;
      uint32_t val_size = 1u << size_code;
      if (offset + val_size > buf_size)
         break;
      if (key == target_key) {
         uint64_t val = 0;
         memcpy(&val, buf + offset, val_size);
         return val;
      }
      offset += val_size;
   }
   return default_val;
}

int main(void)
{
   int fd = open("/dev/mali0", O_RDWR | O_CLOEXEC);
   if (fd < 0) { perror("open /dev/mali0"); return 1; }

   /* size probe */
   struct kbase_ioctl_get_gpuprops q = { .buffer = 0, .size = 0, .flags = 0 };
   long needed = ioctl(fd, KBASE_IOCTL_GET_GPUPROPS, &q);
   if (needed < 0) { perror("gpuprops size probe"); return 1; }
   fprintf(stderr, "gpuprops buffer needed: %ld bytes\n", needed);

   size_t buf_size = (size_t)needed;
   uint8_t *buf = calloc(1, buf_size ? buf_size : 4096);
   if (!buf_size) buf_size = 4096;
   q.buffer = (uintptr_t)buf;
   q.size = (uint32_t)buf_size;
   q.flags = 0;
   long got = ioctl(fd, KBASE_IOCTL_GET_GPUPROPS, &q);
   if (got < 0) { perror("gpuprops read"); return 1; }
   fprintf(stderr, "gpuprops stored: %ld bytes\n", got);

   uint32_t raw_gpu_id = (uint32_t)prop_get(buf, buf_size, KBASE_GPUPROP_RAW_GPU_ID, 0);
   printf("RAW_GPU_ID      = 0x%08x\n", raw_gpu_id);
   printf("  product_id    = 0x%04x (major 0x%x, minor 0x%x)\n",
          raw_gpu_id >> 16, (raw_gpu_id >> 12) & 0xf, raw_gpu_id & 0xf);
   printf("  revision      = r%up%u\n",
          (raw_gpu_id >> 20) & 0xf, raw_gpu_id & 0xf);

   /* arch_major/minor/revison props if present (keys near RAW_GPU_ID) */
   uint64_t core = prop_get(buf, buf_size, KBASE_GPUPROP_RAW_CORE_FEATURES, 0);
   printf("CORE_FEATURES   = 0x%016llx\n", (unsigned long long)core);
   uint64_t tlsz = prop_get(buf, buf_size, KBASE_GPUPROP_RAW_THREAD_MAX_WORKGROUP_SIZE, 0);
   printf("MAX_WG_SIZE     = %llu\n", (unsigned long long)tlsz);
   uint64_t l2 = prop_get(buf, buf_size, KBASE_GPUPROP_RAW_L2_FEATURES, 0);
   printf("L2_FEATURES     = 0x%016llx\n", (unsigned long long)l2);
   uint64_t mem = prop_get(buf, buf_size, KBASE_GPUPROP_RAW_MEM_FEATURES, 0);
   printf("MEM_FEATURES    = 0x%016llx\n", (unsigned long long)mem);

   close(fd);
   return 0;
}
