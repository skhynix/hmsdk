#!/usr/bin/env python3
# Copyright (c) 2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

import argparse
import json
import os
import subprocess as sp
import sys

# common options
monitoring_intervals = "--monitoring_intervals 100ms 2s 20s"
monitoring_nr_regions_range = "--monitoring_nr_regions_range 100 10000"

# common damos options
damos_sz_region = "--damos_sz_region 4096 max"
damos_filter = "--damos_filter memcg nomatching /hmsdk"


def parse_argument():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-d",
        "--dram_nodes",
        dest="dram_nodes",
        nargs="+",
        default=[],
        help="NUMA node for DRAM. Multiple NUMA nodes can be provided.",
    )
    parser.add_argument(
        "-c",
        "--cxl_nodes",
        dest="cxl_nodes",
        nargs="+",
        default=[],
        help="NUMA node for CXL. Multiple NUMA nodes can be provided.",
    )
    parser.add_argument(
        "-g",
        "--global",
        dest="nofilter",
        action="store_true",
        help="Apply 2-tier migration schemes globally not limited to PIDs under 'hmsdk' cgroup.",
    )
    parser.add_argument(
        "-o", "--output", dest="output", default=None, help="Set the output json file."
    )
    return parser.parse_args()


def run_command(cmd):
    with sp.Popen(cmd.split(), stdout=sp.PIPE, stderr=sp.PIPE) as p:
        stdout, stderr = p.communicate()
        stdout_lines = stdout.decode(errors="ignore")
        stderr_lines = stderr.decode(errors="ignore")
        if len(stderr_lines) > 0:
            print(stderr_lines)
        return stdout_lines


def is_valid_node(node_json):
    return len(node_json["kdamonds"][0]["contexts"][0]["targets"][0]["regions"]) > 0


def parent_dir_of_file(file):
    return os.path.dirname(os.path.dirname(os.path.abspath(file)))


def main():
    args = parse_argument()

    if os.geteuid() != 0:
        print("error: Run as root")
        sys.exit(1)

    damo = parent_dir_of_file(__file__) + "/damo/damo"
    node_jsons = []

    common_opts = f"{monitoring_intervals} {monitoring_nr_regions_range}"
    common_damos_opts = f"{damos_sz_region}"
    if not args.nofilter:
        common_damos_opts += f" {damos_filter}"
    for dram_node in args.dram_nodes:
        numa_node = f"--numa_node {dram_node}"
        damos_action = f"--damos_action demote"
        damos_access_rate = "--damos_access_rate 0% 0%"
        damos_age = "--damos_age 30s max"
        damos_quotas = "--damos_quotas 1s 50G 20s 0 0 1%"
        cmd = f"{damo} fmt_json {numa_node} {common_opts} {damos_action} {common_damos_opts} {damos_access_rate} {damos_age} {damos_quotas}"
        json_str = run_command(cmd)
        node_json = json.loads(json_str)
        if not is_valid_node(node_json):
            print(f"error: DRAM node {dram_node} is invalid")
            sys.exit(1)
        node_jsons.append(node_json)

    for cxl_node in args.cxl_nodes:
        numa_node = f"--numa_node {cxl_node}"
        damos_action = f"--damos_action promote"
        damos_access_rate = "--damos_access_rate 5% 100%"
        damos_age = "--damos_age 0 max"
        damos_quotas = "--damos_quotas 2s 50G 20s 0 0 1%"
        cmd = f"{damo} fmt_json {numa_node} {common_opts} {damos_action} {common_damos_opts} {damos_access_rate} {damos_age} {damos_quotas}"
        json_str = run_command(cmd)
        node_json = json.loads(json_str)
        if not is_valid_node(node_json):
            print(f"error: CXL node {cxl_node} is invalid")
            sys.exit(1)
        node_jsons.append(node_json)

    nodes = {"kdamonds": []}

    for node_json in node_jsons:
        nodes["kdamonds"].append(node_json["kdamonds"][0])

    config = json.dumps(nodes, indent=4)
    if args.output:
        with open(args.output, "w") as f:
            f.write(config + "\n")
    else:
        print(config)

    return 0


if __name__ == "__main__":
    sys.exit(main())
