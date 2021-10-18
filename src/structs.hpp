#pragma once

#include <filesystem>
#include <string>
#include <sys/mount.h>
#include <vector>
using namespace std;

/*
 * int mount(const char *source, const char *target,
 *           const char *filesystemtype, unsigned long mountflags,
 *           const void *data);
 */

struct mount_arg {
  filesystem::path src;
  filesystem::path dest;
  std::string fs_type;
  uint64_t mount_flags;
  const void *data;
  mount_arg(std::string source, std::string destination)
      : src(std::move(source)), dest(std::move(destination)), fs_type(""),
        mount_flags(MS_BIND), data(nullptr) {}
  ~mount_arg() = default;
} __attribute__((aligned(64))); // clang-tidy suggestion

enum switch_root_type { CHROOT, PIVOT_ROOT };

struct arg {
  int fd[2];
  filesystem::path new_root;
  filesystem::path old_root;
  char **argv;
  vector<mount_arg> mounts;
  uint32_t clone_flags;
  switch_root_type sr_type;
  bool read_only_root = false;
  bool keep_old_root = false;
  ~arg() { delete[] argv; }
} __attribute__((aligned(128)));
