/* Copyright (c) 2024 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include <argp.h>
#include <numa.h>
#include <numaif.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef MPOL_PREFERRED_MANY
#define MPOL_PREFERRED_MANY 5
#endif

struct opts {
    int idx;
    char *exename;

    const char *membind;
    const char *preferred_many;
    const char *interleave;
    int preferred;
};

struct opts opts;

/* (a part of) output in --help option (generated by argp runtime) */
const char *argp_program_bug_address = "https://github.com/skhynix/hmsdk/issues";

static struct argp_option hmctl_options[] = {
    {.name = "preferred",
     .key = 'p',
     .arg = "node",
     .doc = "Preferably allocate memory on node for hmalloc family allocations"},
    {.name = "preferred-many",
     .key = 'P',
     .arg = "nodes",
     .doc = "Preferably allocate memory on nodes for hmalloc family allocations"},
    {.name = "membind",
     .key = 'm',
     .arg = "nodes",
     .doc = "Only allocate memory from nodes for hmalloc family allocations"},
    {.name = "interleave",
     .key = 'i',
     .arg = "nodes",
     .doc = "Set a memory interleave policy. Memory will be allocated using round robin on nodes"},
    {NULL},
};

static void fail_mpol_conflict(struct argp_state *state, char key) {
    fprintf(stderr, "Error: '-%c' conflicts with other memory policy.\n\n", key);
    argp_state_help(state, state->out_stream, ARGP_HELP_STD_HELP);
}

static error_t parse_option(int key, char *arg, struct argp_state *state) {
    struct opts *opts = state->input;

    switch (key) {
    case 'm':
        opts->membind = arg;
        if (opts->preferred > 0)
            fail_mpol_conflict(state, key);
        break;

    case 'P':
        opts->preferred_many = arg;
        if (opts->membind)
            fail_mpol_conflict(state, key);
        break;

    case 'p':
        opts->preferred = atoi(arg);
        if (opts->membind)
            fail_mpol_conflict(state, key);
        break;

    case 'i':
        opts->interleave = arg;
        break;

    case ARGP_KEY_ARG:
        if (state->arg_num)
            return ARGP_ERR_UNKNOWN;
        if (opts->exename == NULL) {
            /* remaining options will be processed in ARGP_KEY_ARGS */
            return ARGP_ERR_UNKNOWN;
        }
        break;

    case ARGP_KEY_ARGS:
        /* process remaining non-option arguments */
        opts->exename = state->argv[state->next];
        opts->idx = state->next;
        break;

    case ARGP_KEY_NO_ARGS:
    case ARGP_KEY_END:
        if (state->arg_num < 1)
            argp_usage(state);
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static void setup_child_environ(struct opts *opts) {
    unsigned long nodemask = 0;
    char buf[4096] = "0";
    struct bitmask *bm;

    if (opts->membind) {
        snprintf(buf, sizeof(buf), "%d", MPOL_BIND);
        setenv("HMALLOC_MPOL_MODE", buf, 1);

        bm = numa_parse_nodestring(opts->membind);
        if (bm) {
            snprintf(buf, sizeof(buf), "%lu", *bm->maskp);
            setenv("HMALLOC_NODEMASK", buf, 1);
        }
    } else if (opts->preferred_many) {
        snprintf(buf, sizeof(buf), "%d", MPOL_PREFERRED_MANY);
        setenv("HMALLOC_MPOL_MODE", buf, 1);

        bm = numa_parse_nodestring(opts->preferred_many);
        if (bm) {
            snprintf(buf, sizeof(buf), "%lu", *bm->maskp);
            setenv("HMALLOC_NODEMASK", buf, 1);
        }
    } else if (opts->preferred >= 0) {
        /* ignore when --membind is used */
        snprintf(buf, sizeof(buf), "%d", MPOL_PREFERRED);
        setenv("HMALLOC_MPOL_MODE", buf, 1);

        nodemask = 1 << opts->preferred;
        snprintf(buf, sizeof(buf), "%ld", nodemask);
        setenv("HMALLOC_NODEMASK", buf, 1);
    } else if (opts->interleave) {
        snprintf(buf, sizeof(buf), "%d", MPOL_INTERLEAVE);
        setenv("HMALLOC_MPOL_MODE", buf, 1);

        bm = numa_parse_nodestring(opts->interleave);
        if (bm) {
            snprintf(buf, sizeof(buf), "%lu", *bm->maskp);
            setenv("HMALLOC_NODEMASK", buf, 1);
        }
    }

    setenv("HMALLOC_JEMALLOC", "1", 1);
}

int main(int argc, char *argv[]) {
    struct argp argp = {
        .options = hmctl_options,
        .parser = parse_option,
        .args_doc = "[<program>]",
        .doc = "hmctl -- Control heterogeneous memory allocation policy",
    };

    /* default option values */
    opts.membind = NULL;
    opts.preferred = -1;

    argp_parse(&argp, argc, argv, ARGP_IN_ORDER, NULL, &opts);

    /* pass only non-hmctl options to execv() */
    argc -= opts.idx;
    argv += opts.idx;

    setup_child_environ(&opts);

    execv(opts.exename, argv);
    perror(opts.exename);

    return -1;
}
