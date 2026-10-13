# I/O Multiplexing with `select()` in the TCP Server

How [`TCPServer::start()`](../src/tcp_server.cpp#L107-L254) serves many clients from a single thread.

## The problem it solves

`recv()` and `accept()` **block**: they wait until something arrives. With one thread and four clients, you can't call `recv()` on client A if client B might be the next one to speak. You'd sit waiting on A while B waits for a reply.

You could start one thread per client. Instead, you hand the OS **all** your sockets at once and ask: *"Wake me up when any of these has something for me."* That's `select()`. One thread watches many sockets, and you only call `recv()`/`accept()` on sockets that are **guaranteed not to block**.

## The four stages of one loop pass

Every pass through `while (true)` in [tcp_server.cpp:107](../src/tcp_server.cpp#L107) goes through the same four stages:

```
┌───────────────────────────────────────────────────────────┐
│ 1. BUILD   fd_set = { listening socket } + every client   │
│ 2. WAIT    select() sleeps until ≥1 socket is ready       │
│            or the 1-second timeout runs out               │
│            → on return, fd_set holds ONLY ready sockets   │
│ 3. ACCEPT  if the listening socket is ready → accept()    │
│ 4. SERVE   for each client: if ready → recv, parse, reply │
└───────────────────────────────────────────────────────────┘
              ↑                                  │
              └──────────── repeat ──────────────┘
```

### Stage 1: build the watch list ([L108-117](../src/tcp_server.cpp#L108-L117))

```cpp
fd_set read_fds;
FD_ZERO(&read_fds);
FD_SET(server_socket, &read_fds);          // "tell me about new connections"
for (const Client& client : clients)
    FD_SET(client.socket, &read_fds);      // "tell me when a client sends data"
```

An `fd_set` is just a set of sockets. You're asking to be told when any of these becomes **readable**.

**Why rebuild it every pass?** `select()` **overwrites** the set. It goes in as "sockets to watch" and comes back as "sockets that are ready". If you reused it, you'd only be watching last round's ready sockets.

### Stage 2: wait ([L119-141](../src/tcp_server.cpp#L119-L141))

```cpp
timeval timeout{1, 0};
int result = select(0, &read_fds, nullptr, nullptr, &timeout);
```

| Argument | Meaning |
|---|---|
| `0` | `nfds`. Ignored on Windows (it matters on Linux). |
| `&read_fds` | Sockets to check for *readability* |
| `nullptr, nullptr` | Not checking writability or errors |
| `&timeout` | Wait at most 1 second |

The return value:
- **`> 0`**: that many sockets are ready, and `read_fds` now contains only those.
- **`0`**: timed out and nothing happened. The loop does `continue`. The timeout exists so the server gets a chance to check `shutdown_requested` about once a second.
- **`SOCKET_ERROR`**: something broke.

**"Readable" means a call won't block**, which covers three different situations:

| Socket | Readable because… | What your call returns |
|---|---|---|
| Listening socket | A client has finished connecting and is queued | `accept()` gives you a new socket |
| Client socket | Data has arrived | `recv()` returns > 0 bytes |
| Client socket | The client closed the connection | `recv()` returns **0** |
| Client socket | The connection reset or errored | `recv()` returns `SOCKET_ERROR` |

### Stage 3: accept new connections ([L161-175](../src/tcp_server.cpp#L161-L175))

```cpp
if (FD_ISSET(server_socket, &read_fds)) {
    SOCKET client_socket = accept(server_socket, nullptr, nullptr);
    clients.push_back({client_socket, ""});
    continue;   // go back to Stage 1
}
```

`FD_ISSET` asks whether this socket is in the "ready" set. The new client goes into the vector, so the **next** Stage 1 will include it.

### Stage 4: serve ready clients ([L178-253](../src/tcp_server.cpp#L178-L253))

```cpp
for (auto it = clients.begin(); it != clients.end();) {
    if (FD_ISSET(it->socket, &read_fds)) {
        recv(...)                    // guaranteed not to block
        0 bytes / error → close, erase, continue
        append to it->buffer
        while buffer has '\n':       // could be 0, 1, or several commands
            cut out one line → parser.execute → send_all(reply + '\n')
    }
    ++it;                            // ALWAYS advance
}
```

**Why each client has its own buffer:** TCP is a byte stream, not a message stream. A single `recv()` might return half a command, or two and a half commands. The buffer keeps leftover bytes until the `\n` arrives.

> **Bug history:** `++it` used to sit *inside* the `if (FD_ISSET(...))` block. As soon as the loop reached a client that wasn't ready, `it` never advanced and the server spun forever on that client. This only showed up with 2+ clients where an earlier one in the vector was idle.

---

## Walkthrough with 4 clients

Clients A, B, C, D get sockets **180, 268, 300, 312**. Windows socket numbers are arbitrary handles, not 0, 1, 2… `L` is the listening socket.

### Pass 1: nobody is connected yet
```
BUILD:  {L}
WAIT:   1 second passes, nothing happens → result = 0 → continue
```

### Pass 2: A connects
```
BUILD:  {L}
WAIT:   returns 1, read_fds = {L}
ACCEPT: accept() → 180      clients = [A:180]
        continue
```

### Passes 3–4: B and C connect at almost the same moment
```
Pass 3  BUILD:  {L, 180}
        WAIT:   returns 1, read_fds = {L}     ← 2 connections are queued, but L is ONE socket
        ACCEPT: accept() → 268    clients = [A, B]
        continue

Pass 4  BUILD:  {L, 180, 268}
        WAIT:   returns IMMEDIATELY, read_fds = {L}   ← C is still queued
        ACCEPT: accept() → 300    clients = [A, B, C]
```
**Key idea: `select()` is level-triggered.** It keeps reporting a socket as ready for as long as the condition is true, not just once when it changes. You don't have to handle everything at once, because anything you leave unhandled gets reported again next pass.

### Pass 5: A and C send commands, B stays idle
A sends `SET name div\n`, C sends `GET x\n`.
```
BUILD:  {L, 180, 268, 300}
WAIT:   returns 2, read_fds = {180, 300}
ACCEPT: L not set → skip
SERVE:  it → A(180): ready   → recv "SET name div\n" → "OK\n" sent      → ++it
        it → B(268): NOT set → skip                                     → ++it
        it → C(300): ready   → recv "GET x\n"        → value sent       → ++it
        it == end → loop ends
```
B is skipped, but `++it` still runs. If `++it` were inside the `if`, `it` would get stuck on B forever (see the bug history above).

### Pass 6: D connects while A sends at the same moment
```
BUILD:  {L, 180, 268, 300}
WAIT:   returns 2, read_fds = {L, 180}
ACCEPT: accept() → 312    clients = [A, B, C, D]
        continue          ← A's data is NOT read in this pass!

Pass 7  BUILD:  {L, 180, 268, 300, 312}
        WAIT:   returns IMMEDIATELY, read_fds = {180}   ← A's data is still waiting
        SERVE:  A handled
```
The `continue` after `accept` puts A off by one pass, and level-triggering makes sure A isn't lost.

### Pass 8: B's command arrives in two pieces
TCP can split `GET name\n` into `GET na` and `me\n`.
```
Pass 8  read_fds = {268}  recv → "GET na"   B.buffer = "GET na"    no '\n' → reply nothing yet
Pass 9  read_fds = {268}  recv → "me\n"     B.buffer = "GET name\n" → found '\n' → execute → reply
```
The buffer stitches the pieces back together.

### Pass 10: C types EXIT and closes its socket
```
WAIT:   read_fds = {300}        ← a closed connection also counts as "readable"
SERVE:  recv() returns 0 → closesocket(300), it = clients.erase(it)  → continue (no ++it)
        clients = [A, B, D]
```
`erase` returns an iterator to the next element, so the loop does `continue` **without** `++it`. Otherwise it would skip D.

### Pass 11: Ctrl+C
```
signal handler sets shutdown_requested = true
select() returns (data or the 1s timeout) → check flag → stop() closes every socket → break
```
This is why there's a timeout: with an infinite wait, an idle server would never notice Ctrl+C.

---

## Timeline view

```
time →     P2     P3   P4    P5        P6   P7    P8   P9    P10
L        [acc A][acc B][acc C]        [acc D]
A:180                          [SET]        [recv]
B:268                          (idle)                [GET na][me\n→reply]
C:300                          [GET]                               [close]
D:312                                        (new)
```
A single thread interleaves all of them, and it **never blocks on any one client**.

## Things to keep in mind

1. **The `fd_set` is in/out.** Rebuild it every pass.
2. **Ready means one call won't block.** That's one `recv()`, not a whole message, so the buffer and `\n` framing are still needed.
3. **Level-triggered.** Unhandled readiness is reported again on the next pass, so work can safely be deferred.
4. **Closing counts as readable.** `recv() == 0` is how a disconnect is detected.
5. **Erase-while-iterating:** after `erase`, use `continue`; otherwise always `++it`.
6. **What `select()` doesn't do:**
   - `send_all()` still blocks if a client stops reading. The full fix is to also watch a write set and use non-blocking sockets.
   - Windows caps an `fd_set` at 64 sockets by default (`FD_SETSIZE`).
   - `select()` scans every socket on every call. That's fine for a handful of clients, but large servers use IOCP on Windows or epoll on Linux.
