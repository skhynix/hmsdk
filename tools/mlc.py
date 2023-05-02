#!/usr/bin/python3
# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

import os
import shutil
import subprocess
import sys


def run_check_output(cmd):
    out = subprocess.check_output(cmd.split())
    out = out.decode("utf-8")
    return out


def run_with_shell(cmd):
    proc = subprocess.Popen(
        cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    out, err = proc.communicate()
    return out


class MLC:
    def __init__(self):
        self.work_dir = os.path.dirname(os.path.realpath(__file__))
        self.mlc = self.work_dir + "/mlc/Linux/mlc"
        if os.path.isfile(self.mlc) == False:
            self.mlc_dir = self.work_dir + "/mlc/"
            if os.path.isdir(self.mlc_dir):
                shutil.rmtree(self.mlc_dir)
            os.mkdir(self.mlc_dir)

            print("Download MLC...")
            cmd = (
                f"wget -P {self.mlc_dir} "
                "https://downloadmirror.intel.com/736634/mlc_v3.9a.tgz "
                "--no-check-certificate"
            )
            out = run_check_output(cmd)
            cmd = f"tar -xvf {self.mlc_dir}mlc_v3.9a.tgz -C {self.mlc_dir}"
            run_with_shell(cmd)

    def run_bandwidth_matrix(self):
        print("\nMeasuring Bandwidth... It takes a few minutes..")

        # -W2 means 2:1 read-write ratio
        command = f"{self.mlc} --bandwidth_matrix -W2"

        out = run_with_shell(command)
        return out.decode("utf-8")

    def parse_bandwidth_matrix(self, log):
        log_parsed = log.split("\n")
        while True:
            if "Numa node" not in log_parsed[0]:
                del log_parsed[0]
            else:
                break
        del log_parsed[0]
        del log_parsed[0]
        return log_parsed

    def create_bandwidth_matrix(self, log, num_nodes):
        bandwidth_matrix = [[0 for col in range(num_nodes)] for row in range(num_nodes)]
        for line in log:
            if len(line) == 0:
                continue
            row = int(line.split()[0])
            for i in range(num_nodes):
                val = line.split()[i + 1]
                if val == "-":
                    val = 0
                bandwidth_matrix[row][i] = val

        return bandwidth_matrix

    def create_bandwidth_ratio_matrix(self, matrix, num_nodes):
        ratio_matrix = [[0 for col in range(num_nodes)] for row in range(num_nodes)]
        for row in range(num_nodes):
            for col in range(num_nodes):
                baseline = matrix[row][0]
                if baseline == 0:
                    break
                ratio_matrix[row][col] = round(
                    float(matrix[row][col]) / float(baseline), 2
                )
        return ratio_matrix
