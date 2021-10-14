#include <cassert>
#include <climits>
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>

#include "argparse.hpp"
#include "structs.hpp"
#include "utils.hpp"

const int STACK_SIZE = 1024 * 1024;

static uint8_t child_stack[STACK_SIZE];

static int run_child(void *arguments) {
  arg *args = static_cast<arg *>(arguments);

  char root_abs[PATH_MAX]; // PATH_MAX is from climits

  if (realpath(args->new_root, root_abs) == NULL) {
    fail("realname() failed");
  }

  switch (args->sr_type) {
  case switch_root_type::CHROOT:
    if (chroot(root_abs)) {
      fail("chroot");
    }
    if (chdir("/")) {
      fail("chdir");
    }
    break;
  case switch_root_type::PIVOT_ROOT:
    // TODO
    break;
  }

  if (execvp(args->argv[0], args->argv)) {
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

  //
}
