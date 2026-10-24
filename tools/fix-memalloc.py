#!/usr/bin/env python3
"""fix-memalloc.py — patch Mesa kbase_kmod.c: fall back to legacy KBASE_IOCTL_MEM_ALLOC
when MEM_ALLOC_EX returns ENOTTY (kernel advertises >=1.9 but lacks ioctl 59,
e.g. MediaTek CSF kernels on Dimensity 9500)."""

path = "/root/build/panvk-kbase-android/work/mesa/src/panfrost/lib/kmod/kbase_kmod.c"

with open(path) as f:
    src = f.read()

old = '''   if (kbase_dev->is_csf &&
       pan_kmod_driver_version_at_least(&dev->driver, 1, 9)) {
      /* Match current Arm userspace on modern CSF kernels.  Besides keeping
       * the allocation ABI aligned with kbase, this leaves room for fixed-VA
       * and future allocation parameters without another backend split. */
      union kbase_ioctl_mem_alloc_ex req = {
         .in = {
            .va_pages = va_pages,
            .commit_pages = commit_pages,
            .extension = extension,
            .flags = alloc_flags,
         },
      };

      if (ioctl(dev->fd, KBASE_IOCTL_MEM_ALLOC_EX, &req)) {
         mesa_loge("kbase: KBASE_IOCTL_MEM_ALLOC_EX failed: %s",
                   strerror(errno));
         goto err_free_bo;
      }

      alloc_flags = req.out.flags;
      alloc_gpu_va = req.out.gpu_va;
   } else {
      union kbase_ioctl_mem_alloc req = {
         .in = {
            .va_pages = va_pages,
            .commit_pages = commit_pages,
            .extension = extension,
            .flags = alloc_flags,
         },
      };

      if (ioctl(dev->fd, KBASE_IOCTL_MEM_ALLOC, &req)) {
         mesa_loge("kbase: KBASE_IOCTL_MEM_ALLOC failed: %s",
                   strerror(errno));
         goto err_free_bo;
      }

      alloc_flags = req.out.flags;
      alloc_gpu_va = req.out.gpu_va;
   }'''

new = '''   /* Dimensity 9500 (MTK CSF kernel): version reports >= 1.9 but the kernel
    * lacks ioctl 59 (MEM_ALLOC_EX) -> ENOTTY.  Retry with legacy MEM_ALLOC,
    * whose in/out ABI is identical for this purpose. */
   bool want_ex = kbase_dev->is_csf &&
                  pan_kmod_driver_version_at_least(&dev->driver, 1, 9);
   bool alloc_ok = false;

   while (!alloc_ok) {
      if (want_ex) {
         union kbase_ioctl_mem_alloc_ex req = {
            .in = {
               .va_pages = va_pages,
               .commit_pages = commit_pages,
               .extension = extension,
               .flags = alloc_flags,
            },
         };

         if (ioctl(dev->fd, KBASE_IOCTL_MEM_ALLOC_EX, &req) == 0) {
            alloc_flags = req.out.flags;
            alloc_gpu_va = req.out.gpu_va;
            alloc_ok = true;
            break;
         }
         if (errno == ENOTTY) {
            mesa_logw("kbase: MEM_ALLOC_EX unsupported (ENOTTY), "
                      "falling back to legacy MEM_ALLOC");
            want_ex = false;
            continue;
         }
         mesa_loge("kbase: KBASE_IOCTL_MEM_ALLOC_EX failed: %s",
                   strerror(errno));
         break;
      } else {
         union kbase_ioctl_mem_alloc req = {
            .in = {
               .va_pages = va_pages,
               .commit_pages = commit_pages,
               .extension = extension,
               .flags = alloc_flags,
            },
         };

         if (ioctl(dev->fd, KBASE_IOCTL_MEM_ALLOC, &req)) {
            mesa_loge("kbase: KBASE_IOCTL_MEM_ALLOC failed: %s",
                      strerror(errno));
            break;
         }

         alloc_flags = req.out.flags;
         alloc_gpu_va = req.out.gpu_va;
         alloc_ok = true;
      }
   }

   if (!alloc_ok)
      goto err_free_bo;'''

if "falling back to legacy MEM_ALLOC" in src:
    print("already patched, skip")
else:
    start_marker = ("   if (kbase_dev->is_csf &&\n"
                    "       pan_kmod_driver_version_at_least(&dev->driver, 1, 9)) {")
    end_marker = "   kbase_bo->same_va"
    try:
        start = src.index(start_marker)
        end = src.index(end_marker, start)
    except ValueError:
        print("ANCHOR NOT FOUND - aborting, no changes made")
        raise SystemExit(1)
    src = src[:start] + new + "\n\n" + src[end:]
    with open(path, "w") as f:
        f.write(src)
    print("PATCHED: MEM_ALLOC_EX -> ENOTTY fallback to legacy MEM_ALLOC")
