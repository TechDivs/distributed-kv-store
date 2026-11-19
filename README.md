# Distributed KV Store

A key-value store built from scratch in C++.

The project will progressively evolve from a simple single-node
in-memory store into a distributed key-value database.

## Current Features

- In-memory key-value storage
- SET operation
- GET operation
- DELETE operation
- Unit tests

## Tech Stack

- C++
- CMake
- Git

## Roadmap

### 1. Single-Node Key-Value Store
- [x] Implement in-memory key-value store
- [x] Support SET, GET and DELETE operations
- [x] Command parser with input validation
- [x] Unit testing with GoogleTest

### 2. Networking & Concurrency
- [x] TCP server using Winsock
- [x] Handle multiple concurrent clients using `select()`
- [x] Non-blocking socket handling
- [x] Request buffering and command parsing over TCP
- [x] Graceful server shutdown

### 3. Persistence & Crash Recovery
- [x] Write-Ahead Logging (WAL)
- [x] Log Sequence Numbers (LSN)
- [x] WAL durability and disk flushing
- [x] Snapshot-based persistence
- [x] Crash recovery using snapshots and WAL replay
- [x] Recovery validation and error handling

### 4. LSM-Tree Storage Engine
- [x] Implement MemTable using an ordered map
- [x] Support versioned entries using LSNs
- [x] Implement tombstones for deletions
- [x] MemTable memory usage tracking
- [x] Implement SSTable binary file format
- [x] SSTable serialization and deserialization
- [x] Implement in-memory sparse indexing for SSTables
- [x] SSTable point lookups
- [ ] Integrate MemTable, SSTables and WAL
- [ ] Automatic MemTable flushing to SSTables
- [ ] SSTable metadata management and recovery
- [ ] Update read path to search MemTable and SSTables
- [ ] Implement SSTable compaction
- [ ] Handle obsolete versions and tombstone cleanup
- [ ] Implement Bloom filters
- [ ] Implement leveled compaction
- [ ] Revisit snapshot strategy for the LSM-tree architecture

### 5. Performance & Optimization
- [ ] Optimize SSTable indexing and disk reads
- [ ] Add block-based SSTable storage
- [ ] Implement caching for frequently accessed data
- [ ] Optimize memory usage
- [ ] Benchmark read/write throughput and latency
- [ ] Stress testing with large datasets and concurrent clients

### 6. Distributed Architecture
- [ ] Multi-node cluster architecture
- [ ] Node discovery and cluster membership
- [ ] Inter-node communication
- [ ] Data replication
- [ ] Implement Raft consensus
- [ ] Leader election and log replication
- [ ] Handle node failures and recovery
- [ ] Implement data sharding
- [ ] Consistent hashing and partition distribution
- [ ] Cluster-wide request routing

### 7. Final Testing & Reliability
- [ ] Integration testing across storage and networking layers
- [ ] Fault injection and crash testing
- [ ] Distributed consistency testing
- [ ] Performance benchmarking under load
- [ ] Documentation and architecture diagrams