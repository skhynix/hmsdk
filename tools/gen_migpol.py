#!/usr/bin/env python3
# Copyright (c) 2023-2025 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

import argparse
import copy
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
        "-m",
        "--migrate",
        action="append",
        nargs=2,
        metavar=("FAST_TIER", "SLOW_TIER"),
        default=[],
        help="migration between fast and slow tier NUMA nodes.",
    )

    parser.add_argument(
        "-g",
        "--global",
        dest="nofilter",
        action="store_true",
        help="Apply tiered migration schemes globally not limited to PIDs under 'hmsdk' cgroup.",
    )
    parser.add_argument(
        "-s",
        "--region-max-size",
        type=parse_size,
        default=None,
        help="Maximum region size (e.g. 1G, 512M). Regions larger than this will be split.",
    )

    parser.add_argument(
        "-o", "--output", dest="output", default=None, help="Set the output json file."
    )
    return parser.parse_args()


def make_opt(option, value, num_schemes):
    values = " ".join(str(value) for _ in range(num_schemes))
    return f"{option} {values}"


def parse_size(size_str):
    units = {"k": 1024, "m": 1024**2, "g": 1024**3, "t": 1024**4}
    size_str = size_str.strip().lower()
    if size_str[-1] in units:
        return int(float(size_str[:-1]) * units[size_str[-1]])
    else:
        return int(size_str)


def split_region(region, max_size):
    regions = []
    unsplit_regions = []
    for r in region:
        start = comma_addr_to_int(r["start"])
        end = comma_addr_to_int(r["end"])
        if end - start > max_size:
            # Split the region into chunks of max_size
            while start < end:
                sub_end = min(start + max_size, end)
                regions.append(
                    [
                        {
                            "start": int_to_comma_addr(start),
                            "end": int_to_comma_addr(sub_end),
                        }
                    ]
                )
                start = sub_end
        else:
            unsplit_regions.append(r)

    if unsplit_regions:
        regions.append(unsplit_regions)

    return regions


def comma_addr_to_int(addr):
    return int(addr.replace(",", ""))


def int_to_comma_addr(addr):
    return f"{addr:,}"


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

    def __call__(self, nodes, node_json):
        for src_node, dest_node in nodes:
            if src_node == dest_node:
                return f"node {src_node} cannot be used for both SRC and DEST node"

            if src_node in self.handled_node:
                return f"node {src_node} cannot be used multiple times for source node"
            self.handled_node.add(src_node)

        for kdamond in node_json["kdamonds"]:
            nr_regions = len(kdamond["contexts"][0]["targets"][0]["regions"])

            if nr_regions <= 0:
                return f"node {src_node} has no valid regions"

        return None


def main():
    args = parse_argument()

    if os.geteuid() != 0:
        print("error: Run as root")
        sys.exit(1)

    damo = parent_dir_of_file(__file__) + "/damo"
    nodes = []
    node_jsons = []

    common_opts = f"{monitoring_intervals} {monitoring_nr_regions_range}"
    common_damos_opts = f"{damos_sz_region}"
    if not args.nofilter:
        common_damos_opts += f" {damos_filter}"

    check_nodes = CheckNodes()
    cmd_migrate_hot = []
    cmd_migrate_cold = []
    num_schemes = 0

    for fast, slow in args.migrate:
        args.migrate_hot.append([slow, fast])
        args.migrate_cold.append([fast, slow])

    for src_node, dest_node in args.migrate_cold:
        nodes.append((src_node, dest_node))
        num_schemes += 1
        numa_node = f"--numa_node {src_node}"
        damos_action = f"--damos_action migrate_cold {dest_node}"
        damos_access_rate = "--damos_access_rate 0% 0%"
        damos_age = "--damos_age 30s max"
        damos_quotas = "--damos_quotas 1s 50G 20s 0 0 1%"
        damos_young_filter = "--damos_filter young matching"
        cmd_migrate_cold.append(
            f"{numa_node} {common_opts} {damos_action} {common_damos_opts} {damos_young_filter} "
            f"{damos_access_rate} {damos_age} {damos_quotas}"
        )

    for src_node, dest_node in args.migrate_hot:
        nodes.append((src_node, dest_node))
        num_schemes += 1
        numa_node = f"--numa_node {src_node}"
        damos_action = f"--damos_action migrate_hot {dest_node}"
        damos_access_rate = "--damos_access_rate 5% 100%"
        damos_age = "--damos_age 0 max"
        damos_quotas = "--damos_quotas 2s 50G 20s 0 0 1%"
        damos_young_filter = "--damos_filter young nomatching"
        cmd_migrate_hot.append(
            f"{numa_node} {common_opts} {damos_action} {common_damos_opts} {damos_young_filter} "
            f"{damos_access_rate} {damos_age} {damos_quotas}"
        )

    command = f"{damo} args damon --format json "
    for cmd in cmd_migrate_cold:
        command = f"{command} {cmd}"
    for cmd in cmd_migrate_hot:
        command = f"{command} {cmd}"

    if not args.nofilter:
        nr_filters = make_opt("--damos_nr_filters", 2, num_schemes)
    else:
        nr_filters = make_opt("--damos_nr_filters", 1, num_schemes)
    nr_targets = make_opt("--nr_targets", 1, num_schemes)
    nr_schemes = make_opt("--nr_schemes", 1, num_schemes)
    nr_ctxs = make_opt("--nr_ctxs", 1, num_schemes)

    command = f"{command} {nr_filters} {nr_targets} {nr_schemes} {nr_ctxs}"

    # run 'damo args damon' and return json type
    json_str = run_command(command)
    # convert json string to 'dict'
    node_json = json.loads(json_str)
    err = check_nodes(nodes, node_json)
    if err:
        print(f"error: {err}")
        sys.exit(1)

    if args.region_max_size is not None:
        new_kdamonds = []
        for kdamond in node_json["kdamonds"]:
            region = kdamond["contexts"][0]["targets"][0]["regions"]
            split = split_region(region, args.region_max_size)
            for region in split:
                new_kdamond = copy.deepcopy(kdamond)
                new_kdamond["contexts"][0]["targets"][0]["regions"] = region
                new_kdamonds.append(new_kdamond)

        node_json["kdamonds"] = new_kdamonds

    config = yaml.dump(node_json, default_flow_style=False, sort_keys=False)
    if args.output:
        with open(args.output, "w") as f:
            f.write(config + "\n")
    else:
        print(config)

    return 0


if __name__ == "__main__":
    sys.exit(main())
