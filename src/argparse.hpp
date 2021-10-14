#pragma once

#include <algorithm>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <numeric>

#include "structs.hpp"
#include "utils.hpp"

const uint32_t default_clone_flags = CLONE_NEWUSER | CLONE_NEWNS;

void handle_volume_mount_parsing(arg &args) {
  string opt_arg(optarg);
  auto src_dest_perm = split_string(opt_arg, ':');
  if (src_dest_perm.size() >= 2) {
    args.mounts.emplace_back(std::move(src_dest_perm[0]),
                             std::move(src_dest_perm[1]));
    if (src_dest_perm.size() == 3) {
      if (src_dest_perm[2] == "ro") {
        args.mounts.back().mount_flags |= MS_RDONLY;
      } else if (src_dest_perm[2] == "rw") {
        /*
         * No need to do anything,default is rw
         */
      } else {
        arg_err("Invalid permission provided for --volume/-v, Permisson can "
                "only be ro|rw");
      }
    }
  } else {
    arg_err("Invalid parameters for -v or --volume. Expected format "
            "<src>:<dest>(:<perm_string>)?");
  }
}

arg parse_args(int argc, char *argv[]) {
  /*
   * getopt.h exposes the variables optarg,optind
   */

  arg args = {
      .new_root = nullptr,
      .argv = nullptr,
      .mounts = {},
      .clone_flags = default_clone_flags,
      .sr_type = switch_root_type::CHROOT,
      .read_only_root = false,
  };

  while (true) {
    int option_index = 0;
    static option long_options[] = {
        {"help", no_argument, nullptr, 'h'},
        {"volume", required_argument, nullptr, 'v'},
        {"read-only", no_argument, nullptr, 'r'},
        {"net", no_argument, nullptr, 'n'},
        {"ipc", no_argument, nullptr, 'i'},
    };
    /*
     * man 3 getopt_long
     */
    int c = getopt_long(argc, argv, "hv:rU:G:ni", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
    case 'h':
      cout << "Help!!!!\n";
      break;
    case 'v':
      handle_volume_mount_parsing(args);
      break;
    case 'r':
      args.read_only_root = true;
      break;
    case 'n':
      args.clone_flags |= CLONE_NEWNET;
      break;
    case 'i':
      args.clone_flags |= CLONE_NEWIPC;
      break;
    default:
      break;
    }
  }
  if (optind == argc) {
    args.new_root = ".";
  } else {
    args.new_root = argv[optind];
    optind++;
  }
  if (optind < argc) {
    args.argv = new char *[argc - optind + 1];
    int i = 0;
    while (optind != argc) {
      args.argv[i++] = argv[optind++];
    }
    args.argv[i] = NULL;
  } else {
    args.argv = new char *[2];
    args.argv[0] = get_shell();
    args.argv[1] = NULL;
  }

  return args;
}
