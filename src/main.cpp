#include <cassert>
#include <climits>
#include <csignal>
#include <filesystem>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include "argparse.hpp"
#include "structs.hpp"
#include "utils.hpp"

const int STACK_SIZE = 1024 * 1024;

static uint8_t child_stack[STACK_SIZE];

static int pivot_root(const char *new_root, const char *put_old) {
  return syscall(SYS_pivot_root, new_root, put_old);
}

static int run_child(void *arguments) {
  arg *args = static_cast<arg *>(arguments);

  auto new_root_abs =
      filesystem::absolute(args->new_root).make_preferred().lexically_normal();

  switch (args->sr_type) {
  case switch_root_type::CHROOT:
    if (chroot(new_root_abs.c_str())) {
      fail("chroot");
    }
    if (chdir("/")) {
      fail("chdir");
    }
    break;
  case switch_root_type::PIVOT_ROOT:
    // TODO

    if (!args->old_root.is_absolute()) {
      fail2("Error: The old root path must be absolute");
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
    if (mount(new_root_abs.c_str(), new_root_abs.c_str(), NULL, MS_BIND,
              NULL) != 0) {
      fail("Mount new_root");
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
      if (mount("", new_root_abs.c_str(), NULL,
                MS_BIND | MS_RDONLY | MS_REMOUNT, NULL) != 0) {
        fail("Remounting read only root");
      }
    }
    if (pivot_root(new_root_abs.c_str(), old_root_abs_path.c_str()) != 0) {
      cout << new_root_abs << ":" << old_root_abs_path << '\n';
      fail("pivot_root");
    }
    if (chdir("/") != 0) {
      fail("chdir after pivot_root()");
    }
    if (!args->keep_old_root) {
      cout << args->old_root.make_preferred() << '\n';
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
        fail("private mount over old_root ");
      }
      if (umount2(args->old_root.make_preferred().c_str(), MNT_DETACH) != 0) {
        fail("umount old_root");
      }
    }
    break;
  }

  if (execvp(args->argv[0], args->argv) != 0) {
    fail("execvp");
  }
  return -1;
}

int run(arg &args) {
  //
  pid_t child_pid = clone(run_child, child_stack + STACK_SIZE,
                          args.clone_flags | SIGCHLD, (void *)&args);
  if (child_pid == -1) {
    fail("clone() failed");
  }
  close(args.fd[1]);
  return waitpid(child_pid, nullptr, 0);
}

int main(int argc, char *argv[]) {
  arg args = parse_args(argc, argv);
  return run(args);
}
