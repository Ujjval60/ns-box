#pragma once
#include <cassert>
#include <climits>
#include <csignal>
#include <filesystem>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include "argparse.hpp"
#include "structs.hpp"
#include "utils.hpp"

namespace mounts {
static int pivot_root(const char *new_root, const char *put_old) {
  return syscall(SYS_pivot_root, new_root, put_old);
}

void remnt_dir_to_dest_chroot(const mount_arg &m_arg,
                              const filesystem::path &dest_prefix) {
  auto source = filesystem::path(string("/tmp/") + m_arg.dest.string())
                    .make_preferred()
                    .lexically_normal();
  auto dest = filesystem::path(dest_prefix.string() + m_arg.dest.string())
                  .make_preferred()
                  .lexically_normal();
  if (!filesystem::is_directory(dest)) {
    filesystem::create_directories(dest);
  }
  int ret;
  cout << source << ":" << dest << '\n';
  // mount tmp directories mount to a directory on rootfs
  if ((ret = mount(source.c_str(), dest.c_str(), m_arg.fs_type.c_str(),
                   m_arg.mount_flags, m_arg.data)) != 0) {
    util::fail("mount()");
  }

  if (umount2(source.c_str(), MNT_DETACH) != 0) {
    util::fail("umount2(/tmp/<dest>,MNT_DETACH) ");
  }

  if (m_arg.mount_flags & MS_RDONLY) {
    if ((ret = mount("", dest.c_str(), NULL, MS_REMOUNT | MS_RDONLY | MS_BIND,
                     NULL)) != 0) {
      util::fail("RDONLY mount()");
    }
  }
}

void mnt_dir_to_tmp_chroot(const mount_arg &m_arg,
                           const filesystem::path &dest_prefix) {
  auto source = filesystem::path("/" + m_arg.src.string())
                    .make_preferred()
                    .lexically_normal();
  auto dest = filesystem::path(dest_prefix.string() + string("/tmp/") +
                               m_arg.dest.string())
                  .make_preferred()
                  .lexically_normal();

  if (!filesystem::is_directory(dest)) {
    filesystem::create_directories(dest);
  }
  int ret;
  cout << "tmp>>" << source << ":" << dest << '\n';

  if ((ret = mount(source.c_str(), dest.c_str(), m_arg.fs_type.c_str(),
                   m_arg.mount_flags, m_arg.data)) != 0) {
    util::fail("mount to tmp directory");
  }
  //
}

void mnt_dir_to_dest_pivot_root(const mount_arg &m_arg,
                                const filesystem::path &source_prefix,
                                const filesystem::path &dest_prefix) {
  auto source = filesystem::path(source_prefix.string() + m_arg.src.string())
                    .make_preferred()
                    .lexically_normal();
  auto dest = filesystem::path(dest_prefix.string() + m_arg.dest.string())
                  .make_preferred()
                  .lexically_normal();
  if (!filesystem::is_directory(dest)) {
    filesystem::create_directories(dest);
  }
  if (mount(source.c_str(), dest.c_str(), m_arg.fs_type.c_str(),
            m_arg.mount_flags, m_arg.data) != 0) {
    util::fail("mount()");
  }

  if (m_arg.mount_flags & MS_RDONLY) {
    if (mount("", dest.c_str(), NULL, MS_REMOUNT | MS_RDONLY | MS_BIND, NULL) !=
        0) {
      util::fail("RDONLY mount()");
    }
  }
}

void handle_chroot_mount(filesystem::path &new_root_abs, arg *args) {

  for_each(args->mounts.cbegin(), args->mounts.cend(),
           [&](const mount_arg &m_arg) {
             mnt_dir_to_tmp_chroot(m_arg, new_root_abs);
           });
  if (chdir(new_root_abs.c_str()) != 0) {
    util::fail("chdir(new_root)");
  }

  if (chroot(".") != 0) {
    util::fail("chroot");
  }
  for_each(
      args->mounts.cbegin(), args->mounts.cend(),
      [&](const mount_arg &m_arg) { remnt_dir_to_dest_chroot(m_arg, "/"); });
}

void handle_pivot_root_mount(filesystem::path &new_root_abs, arg *args) {
  // TODO

  if (!args->old_root.is_absolute()) {
    util::fail2("Error: The old root path must be absolute");
  }

  /*
   *
   * This doesnt work because the args->old_root is absolute path and so is
   * new_root_abs, so this doesnt work. If new_root_abs was absolute/relative
   * and args->old_root was relative, this wouldve worked but since we expect
   * absolute path for the args->old_root, we cant use it this way.
   *
   * filesystem::path old_root_abs_path_not_working =
   *     (new_root_abs / args->old_root).make_preferred().lexically_normal();
   * cout << old_root_abs_path_not_working_in_gcc << '\n';
   */

  filesystem::path old_root_abs_path =
      filesystem::path(new_root_abs.string() + args->old_root.string())
          .make_preferred()
          .lexically_normal();

  /*
   * Mounting here so i can remount later if needed without any problems
   */
  if (mount(new_root_abs.c_str(), new_root_abs.c_str(), NULL, MS_BIND, NULL) !=
      0) {
    util::fail("Mount new_root");
  }
  if (args->read_only_root) {
    /*
     *  FROM man 2 mount
     *
     * An existing mount may be remounted by specifying MS_REMOUNT in
     * mountflags.  This allows you to change the mountflags and data
     * of an existing mount without having to unmount and remount the
     * filesystem.  target should be the same value specified in the
     * initial mount() call.
     *
     * The source and filesystemtype arguments are ignored.
     */
    if (mount("", new_root_abs.c_str(), NULL, MS_BIND | MS_RDONLY | MS_REMOUNT,
              NULL) != 0) {
      util::fail("Remounting read only root");
    }
  }

  /*
   *
   * man 2 pivot_root
   *
   *  The following restrictions apply:
   *
   *  -  new_root and put_old must be directories.
   *
   *  -  new_root and put_old must not be on the same mount as the
   *     current root.
   *
   *  -  put_old  must  be  at or underneath new_root; that is, adding
   *  some nonnegative number of "/.." prefixes to the pathname pointed to by
   *  put_old must yield the same directory as new_root.
   *
   *  -  new_root must be a path to a mount point, but can't be "/".  A
   *     path that is not already a mount point can  be  converted  into  one
   * by bind mounting the path onto itself.
   *
   *  -  The  propagation type of the parent mount of new_root and the
   *  	parent mount of the current root directory must not be
   * MS_SHARED; similarly, if put_old is an existing mount point, its
   * propagation type must not be MS_SHARED.  These restrictions ensure that
   * pivot_root() never propagates any changes to another mount namespace.
   *
   *  -  The current root directory must be a mount point.
   *
   *
   * pivot_root with new_root as "/" will fail because the new_root_abs will
   * be on the same mount as current root
   *
   *
   */

  if (pivot_root(new_root_abs.c_str(), old_root_abs_path.c_str()) != 0) {
    cout << new_root_abs << ":" << old_root_abs_path << '\n';
    util::fail("pivot_root");
  }
  if (chdir("/") != 0) {
    util::fail("chdir after pivot_root()");
  }

  for_each(args->mounts.cbegin(), args->mounts.cend(),
           [&](const mount_arg &m_arg) {
             mnt_dir_to_dest_pivot_root(m_arg, args->old_root, "/");
           });

  /*
   * We have changed our root to point to new_root_abs now.
   * Everything we do now should be relative to that now.
   */
  if (!args->keep_old_root) {
    /*
     *
     * man 2 mount
     *
     * If mountflags includes one of MS_SHARED, MS_PRIVATE, MS_SLAVE,
     * or MS_UNBINDABLE (all available since Linux 2.6.15), then the
     * propagation type of an existing mount is changed.  If more than
     * one of these flags is specified, an error results.
     *
     * The only other flags that can be specified while changing the
     * propagation type are MS_REC (described below) and MS_SILENT
     * (which is ignored).
     *
     * The source, filesystemtype, and data arguments are ignored.
     *
     */
    if (mount("", args->old_root.make_preferred().c_str(), NULL,
              MS_REC | MS_PRIVATE, NULL) != 0) {
      util::fail("private mount over old_root ");
    }
    if (umount2(args->old_root.make_preferred().c_str(), MNT_DETACH) != 0) {
      util::fail("umount old_root");
    }
  }

  //
}

} // namespace mounts
