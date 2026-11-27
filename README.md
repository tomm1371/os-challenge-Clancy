# os-challenge-Clancy

This project has been developed solo, by Tommy Krogh Andersen(s245569).

## File structure

The file structure has all the files from the `os-challenge-common.git` repository, and are for the most part untouched. The source code is located in `src/` and its structure is explained in *experiments*.

``` txt
.
├── arm64
│   └── ...
├── bootstrap.sh
├── get-bin-path.sh
├── Makefile
├── README.md
├── run-all.sh
├── run-client-x.sh
├── run-reference-server.sh
├── src
│   ├── 00_bruteforce
│   │   └── ...
│   ├── 01_FIFO
│   │   └── ...
│   └── 02_EXP2
│       └── ..
├── submission.py
└── x86_64
    └── *Vagrant config and execution*

```

## Experiments

The project has been developed in a singular git branch, where the experiments are organized, in their own folders in `src/`. Each experiment and represent a snapshot, of what the implementation was like when developed.
Each experiment has its own:

- Source Code (`*.C` & `*.h`).
- `README.md` explaining the experiment and what has changed from the previous one.
- `run.sh` that runs the experiment on the `run-client-final.sh` settings on 5 predetermined seeds so they can be compared.
- `results.csv` shows the results of `run.sh`.
- `raw/` containing the raw logs of the tests.

Along with those are the `.c` and `.h` used in the experiment, file structure is shown below.

``` txt
.
└── src
    ├── 00_bruteforce
    │   ├── raw/
    │   │   ├── seed[x]_rep[y]_server.log
    │   │   └── seed[x]_rep[y].log
    │   ├── README.md
    │   ├── results.csv
    │   ├── run.sh
    │   ├── server.c
    │   ├── messages.h
    │   └── ...
    ├── 01_FIFO
    │   └── ...
    └── 02_EXP2
        └── ...
```

`00` represents the baseline program, which linearly handles each request from the client after the previous, by brute forcing(BF) the results with `lonesha256`.

`01` adds `n` threads, matching the ones on the given system `1` thread acts as an accepter which handles incoming requests from the client, by formatting them and appending a them in a linked list. the remaining `n-1` threads act as workers, each handling the next request with BF, closes it and pop's it from then list.

## Run experiments

From the repository root, run all experiments sequentially with:

```sh
./run-all.sh
```

To run a single experiment, use its `src/<experiment>/run.sh`. Logs are saved
under `run-logs/` for the master run and `src/<experiment>/raw/` for individual
runs. Set `SEEDS` or `REPS` to adjust the workload, for example:

```sh
SEEDS="10 20" REPS=2 ./run-all.sh
```