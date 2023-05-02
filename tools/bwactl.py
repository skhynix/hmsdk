#!/usr/bin/env python3
# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

import argparse
import os
import re
import subprocess as sp
import sys
from enum import Enum, auto

from iw_ratio import *
from mlc import *

INTERLEAVE_WEIGHT_DIR = "/sys/kernel/mm/interleave_weight"
INTERLEAVE_WEIGHT_POSSIBLE_PATH = f"{INTERLEAVE_WEIGHT_DIR}/possible"


def check_root_perm():
    if os.geteuid() != 0:
        print("error: bwactl.py can only be executed with root permission.")
        sys.exit(-1)


def run_lstopo():
    cmd = "lstopo-no-graphics -p"
    err = None
    try:
        with sp.Popen(cmd.split(), stdout=sp.PIPE) as p:
            lines = p.communicate()[0].decode(errors="ignore")
    except:
        err = "lstopo not found!"
        return None, err
    return lines.split("\n"), None


def read_lstopo(lstopo_file):
    with open(lstopo_file, "r") as f:
        lines = f.readlines()
    lines = list(map(lambda x: x[:-1], lines))  # remove trailing '\n'
    return lines, None


def get_node_to_package_map(lstopo_file):
    if lstopo_file != None:
        lines, err = read_lstopo(lstopo_file)
    else:
        lines, err = run_lstopo()
    if err != None:
        print(err)
        return None
    nodes = {}

    rp = re.compile("^  Package P#(\d+)")
    rn = re.compile("^    NUMANode P#(\d+)")
    rn2 = re.compile("^      NUMANode P#(\d+)")
    rmon = re.compile("^  NUMANode P#(\d+)")  # memory only node

    current_package = None
    for line in lines:
        if "Package" in line:
            m = rp.match(line)
            current_package = int(m.group(1))
        elif "NUMANode" in line:
            m = rn.match(line)
            if not m:
                m = rn2.match(line)
            if m:
                node = int(m.group(1))
                nodes[node] = current_package
            else:
                m2 = rmon.match(line)
                node = int(m2.group(1))
                nodes[node] = None
    return nodes


def get_possible_numa_nodes():
    # Read possible nodes to avoid including memory only numa nodes.
    possible = []
    with open(f"{INTERLEAVE_WEIGHT_POSSIBLE_PATH}", "r") as f:
        possible = f.read()[:-1].split(",")
        possible = list(map(lambda x: int(x), possible))
    return possible


# Parse topology information in python 2-level nested list.
# e.g. If a string "[[0,2],[1,3]]" is given,
#      then it return [[0,2],[1,3]] in python data.
def topology_to_pydata(topology):
    assert topology[0:2] == "[["
    assert topology[-2:] == "]]"

    rn = re.compile("^(\d+)")
    pytopo = []
    i = 1
    nested = False

    while i < len(topology) - 1:
        if topology[i] == "[":
            assert not nested
            pytopo.append([])
            nested = True
            i += 1
        elif topology[i] == "]":
            assert nested
            assert len(pytopo[-1]) > 0
            # check if pytopo[-1] has memory only nodes.
            has_cpu_node_in_package = False
            for nid in pytopo[-1]:
                has_cpu_node_in_package = True
            assert has_cpu_node_in_package
            nested = False
            i += 1
        elif topology[i] == "," or topology[i] == " ":
            i += 1
        else:
            assert nested
            m = rn.match(topology[i:])
            nid = m.group(1)

            # check if the nid is in another package
            for package in pytopo:
                for node in package:
                    assert node != int(nid)

            pytopo[-1].append(int(nid))
            i += len(nid)
    return pytopo


def display_multi_ratio(matrix, possible, args):
    # nodes is a dict that contains
    #   key  : node ID
    #   value: package ID
    nodes = {}
    if args.topology:
        try:
            pytopo = topology_to_pydata(args.topology)
            nr_nodes = 0
            for pack in pytopo:
                nr_nodes += len(pack)
            system_nodes = get_system_nodes()
            if nr_nodes != system_nodes:
                print(
                    f"error: topology has {nr_nodes} numa nodes but must have {system_nodes} nodes"
                )
                os._exit(-1)
        except:
            print(f"error: invalid topology '{args.topology}'")
            sys.exit(-1)
        for pack_idx in range(len(pytopo)):
            has_cpu_node_in_package = False
            for nid in pytopo[pack_idx]:
                for cid in possible:
                    if nid == cid:
                        has_cpu_node_in_package = True
                nodes[nid] = pack_idx
            if not has_cpu_node_in_package:
                print(
                    f"error: package '{pytopo[pack_idx]}' has no cpu numa nodes as in the file below."
                )
                print(f"       please check '{INTERLEAVE_WEIGHT_POSSIBLE_PATH}'")
                sys.exit(-1)
    else:
        nodes = get_node_to_package_map(args.lstopo_file)

    print("Bandwidth ratio for all NUMA nodes")

    # Write interleave weight value into sysfs file.
    updated_files = []
    for nid in possible:
        row = matrix[nid]
        ratio = ""
        nids = []
        weights = []
        for i in range(len(row)):
            if nodes == None:
                nids.append(i)
                weights.append(int(row[i]))
            elif nodes[i] == nodes[nid]:
                # Add ratio only when they are in the same package.
                nids.append(i)
                weights.append(int(row[i]))

        lgcd = list_gcd(weights)
        if lgcd != 0:
            weights = list(map(lambda x: int(x / lgcd), weights))
        for i in range(len(weights)):
            ratio += f"{nids[i]}*{weights[i]},"
        # Remove trailing comma(,)
        ratio = ratio[:-1]

        print(f"node{nid}: {ratio}")

        if "*0" in ratio:
            print(f"invalid ratio {ratio}: cpu node without memory is not allowed.")
            sys.exit(-1)

        filename = f"{INTERLEAVE_WEIGHT_DIR}/node/node{nid}/interleave_weight"
        with open(filename, "w") as f:
            f.write(ratio)
            updated_files.append(filename)

    print("\nBandwidth ratio is successfully updated at")
    for updated_file in updated_files:
        print(f"  {updated_file}")


def check_sysfs():
    if not os.path.exists(f"{INTERLEAVE_WEIGHT_DIR}/node/node0/interleave_weight"):
        print(f"error: sysfs not found at {INTERLEAVE_WEIGHT_DIR}")
        sys.exit(-1)


def get_system_nodes():
    nr_nodes = 0
    sys_node_dir = "/sys/devices/system/node/"
    for file in os.listdir(sys_node_dir):
        if file[:4] == "node":
            nr_nodes += 1
    return nr_nodes


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--mlc-file",
        type=str,
        required=False,
        help="Pass mlc result from file instead of running it",
    )
    parser.add_argument(
        "--lstopo-file",
        type=str,
        required=False,
        help="Pass lstopo result from file instead of running it",
    )
    parser.add_argument(
        "--topology",
        type=str,
        required=False,
        help="topology structure",
    )
    args = parser.parse_args()

    check_sysfs()

    check_root_perm()

    mlc = MLC()

    # Get number of system numa nodes
    numa_node_all = get_system_nodes()

    if not args.mlc_file:
        # execute mlc
        output = mlc.run_bandwidth_matrix()
    else:
        # read mlc result from mlc_file
        with open(args.mlc_file, "r") as f:
            output = f.read()

    # Save the output of mlc result for later reference.
    with open("mlc.out", "w") as f:
        f.write(output)

    # Parse and create bandwidth matrix using mlc log
    bandwidth_matrix_log = mlc.parse_bandwidth_matrix(output)
    bandwidth_matrix = mlc.create_bandwidth_matrix(bandwidth_matrix_log, numa_node_all)
    # Convert bandwidth value from float to int
    bandwidth_matrix = [
        [int(float(i)) for i in bandwidth_matrix[j]]
        for j in range(len(bandwidth_matrix))
    ]

    possible = get_possible_numa_nodes()

    # Calculate ratio_matrix based on the given bandwidth_matrix.
    ratio_matrix = get_iw_ratio_matrix(bandwidth_matrix, possible)

    # Print result to stdout and sysfs
    display_multi_ratio(ratio_matrix, possible, args)


if __name__ == "__main__":
    main()
