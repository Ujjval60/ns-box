#include "mounts.hpp"

const int STACK_SIZE = 1024 * 1024;

static uint8_t child_stack[STACK_SIZE];

static int run_child(void *arguments) {
  arg *args = static_cast<arg *>(arguments);

  auto new_root_abs =
      filesystem::absolute(args->new_root).make_preferred().lexically_normal();

  switch (args->sr_type) {
  case switch_root_type::CHROOT:
    mounts::handle_chroot_mount(new_root_abs, args);
    break;
  case switch_root_type::PIVOT_ROOT:
    mounts::handle_pivot_root_mount(new_root_abs, args);
    break;
  }

  if (execvp(args->argv[0], args->argv) != 0) {
    util::fail("execvp");
  }
  return -1;
}

int run(arg &args) {
  //
  pid_t child_pid = clone(run_child, child_stack + STACK_SIZE,
                          args.clone_flags | SIGCHLD, (void *)&args);
  if (child_pid == -1) {
    util::fail("clone() failed");
  }
  close(args.fd[1]);
  return waitpid(child_pid, nullptr, 0);
}

int main(int argc, char *argv[]) {
  arg args = argparse::parse_args(argc, argv);
  return run(args);
}
