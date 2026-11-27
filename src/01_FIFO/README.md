# FIFO server

The accepter thread reads each complete request and appends it to a shared,
mutex-protected linked-list queue. Worker threads remove requests from the head
in FIFO order, run the brute-force search, send the result to the request's
client socket, and close that socket.

Each queue entry retains the request hash, start and exclusive end values,
priority byte, and client socket descriptor. The queue currently appends every
request at the tail; the stored priority can be used if the queue is later
changed to prioritize requests.

## Running the experiment

Run `./src/01_FIFO/run.sh` from the repository root (or `./run.sh` from this
directory). The script builds the FIFO server, then runs the client three times
for each of the five default seeds (`1 2 3 4 5`). It uses the workload settings
from `run-client-final.sh` and restarts the server process between every client
run. Client and server output are saved under `src/01_FIFO/raw/`.

The run count and seeds can be changed with environment variables, for example:

```sh
SEEDS="10 20" REPS=4 ./src/01_FIFO/run.sh
```

The final-run workload parameters can also be overridden through `TOTAL`,
`START`, `DIFFICULTY`, `REP_PROB_PERCENT`, `DELAY_US`, and `PRIO_LAMBDA`.

## Changes from `00_bruteforce`

In `00_bruteforce`, the server accepts one connection and handles that request
completely in the main server loop: it reads the request, searches the candidate
range, sends the response, and closes the socket. Only after that work finishes
does it accept another connection. Since the brute-force search can take a long
time, one request can prevent the server from making progress on later
connections.

In `01_FIFO`, accepting and processing are separated. The server creates an
accepter thread and a pool of worker threads (based on the number of online CPU
cores, with a minimum of one). The accepter reads and queues requests while
workers independently remove and process queued requests. The search algorithm
is still the same brute-force search; the main change is how requests are
scheduled and how the work is shared.
