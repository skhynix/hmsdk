#!/usr/bin/env python3
# Copyright (c) 2023-2024 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

import argparse
import json
import os
import subprocess as sp
import sys

import yaml

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
        "--demote",
        dest="migrate_cold",
        action="append",
        nargs=2,
        metavar=("SRC", "DEST"),
        default=[],
        help="source and destination NUMA nodes for demotion.",
    )
    parser.add_argument(
        "-p",
        "--promote",
        dest="migrate_hot",
        action="append",
        nargs=2,
        metavar=("SRC", "DEST"),
        default=[],
        help="source and destination NUMA nodes for promotion.",
    )
    parser.add_argument(
        "-g",
        "--global",
        dest="nofilter",
        action="store_true",
        help="Apply tiered migration schemes globally not limited to PIDs under 'hmsdk' cgroup.",
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


def parent_dir_of_file(file):
    return os.path.dirname(os.path.dirname(os.path.abspath(file)))


class CheckNodes:
    def __init__(self):
        self.handled_node = set()

    def __call__(self, src_node, dest_node, node_json):
        if src_node in self.handled_node:
            return f"node {src_node} cannot be used multiple times for source node"
        self.handled_node.add(src_node)

        if src_node == dest_node:
            return f"node {src_node} cannot be used for both SRC and DEST node"

        nr_regions = len(
            node_json["kdamonds"][0]["contexts"][0]["targets"][0]["regions"]
        )
        if nr_regions <= 0:
            return f"node {src_node} has no valid regions"

        return None


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

    check_nodes = CheckNodes()

    for src_node, dest_node in args.migrate_cold:
        numa_node = f"--numa_node {src_node}"
        damos_action = f"--damos_action migrate_cold {dest_node}"
        damos_access_rate = "--damos_access_rate 0% 0%"
        damos_age = "--damos_age 30s max"
        damos_quotas = "--damos_quotas 1s 50G 20s 0 0 1%"
        damos_young_filter = "--damos_filter young matching"
        cmd = f"{damo} fmt_json {numa_node} {common_opts} {damos_action} {common_damos_opts} {damos_young_filter} {damos_access_rate} {damos_age} {damos_quotas}"
        json_str = run_command(cmd)
        node_json = json.loads(json_str)
        node_jsons.append(node_json)
        err = check_nodes(src_node, dest_node, node_json)
        if err:
            print(f"error: {err}")
            sys.exit(1)

    for src_node, dest_node in args.migrate_hot:
        numa_node = f"--numa_node {src_node}"
        damos_action = f"--damos_action migrate_hot {dest_node}"
        damos_access_rate = "--damos_access_rate 5% 100%"
        damos_age = "--damos_age 0 max"
        damos_quotas = "--damos_quotas 2s 50G 20s 0 0 1%"
        damos_young_filter = "--damos_filter young nomatching"
        cmd = f"{damo} fmt_json {numa_node} {common_opts} {damos_action} {common_damos_opts} {damos_young_filter} {damos_access_rate} {damos_age} {damos_quotas}"
        json_str = run_command(cmd)
        node_json = json.loads(json_str)
        node_jsons.append(node_json)
        err = check_nodes(src_node, dest_node, node_json)
        if err:
            print(f"error: {err}")
            sys.exit(1)

    nodes = {"kdamonds": []}

    for node_json in node_jsons:
        nodes["kdamonds"].append(node_json["kdamonds"][0])

    config = yaml.dump(nodes, default_flow_style=False, sort_keys=False)
    if args.output:
        with open(args.output, "w") as f:
            f.write(config + "\n")
    else:
        print(config)

    return 0


if __name__ == "__main__":
    sys.exit(main())
