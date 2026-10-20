
#include "../include/snapshot.hpp"

#define NOMINMAX
#include <windows.h>
#include <fstream>
#include <stdexcept>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <utility>

Snapshot::Snapshot(const std::string& path)
    : file_path(path) {}

void Snapshot::save(const std::unordered_map<std::string, std::string>& data,uint64_t lsn) {
    const std::string temp_path = file_path + ".tmp";

    std::ofstream out(
        temp_path,
        std::ios::binary | std::ios::trunc
    );

    if (!out) {
        throw std::runtime_error("Failed to open temporary snapshot file");
    }

    // Header: magic bytes + format version.
    const char magic[] = {'K', 'V', 'S', 'N'};
    const uint32_t version = 1;
    const uint64_t record_count = data.size();

    out.write(magic, sizeof(magic));
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));
    out.write(reinterpret_cast<const char*>(&lsn), sizeof(lsn));
    out.write(
        reinterpret_cast<const char*>(&record_count),
        sizeof(record_count)
    );

    // Each entry: key length, key bytes, value length, value bytes.
    for (const auto& [key, value] : data) {
        const uint64_t key_size = key.size();
        const uint64_t value_size = value.size();

        out.write(
            reinterpret_cast<const char*>(&key_size),
            sizeof(key_size)
        );
        out.write(key.data(), static_cast<std::streamsize>(key.size()));

        out.write(
            reinterpret_cast<const char*>(&value_size),
            sizeof(value_size)
        );
        out.write(value.data(), static_cast<std::streamsize>(value.size()));
    }

    out.flush();

    if (!out) {
        out.close();
        throw std::runtime_error("Failed to write snapshot");
    }

    out.close();

    if (!out) {
        throw std::runtime_error("Failed to close snapshot file");
    }

    // Flush the temporary file to disk before replacing the final snapshot.
    HANDLE file = CreateFileA(
        temp_path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (file == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to reopen temporary snapshot");
    }

    const BOOL flushed = FlushFileBuffers(file);
    CloseHandle(file);

    if (!flushed) {
        throw std::runtime_error("Failed to flush snapshot to disk");
    }

    // Atomically replace the previous snapshot where supported.
    if (!MoveFileExA(
            temp_path.c_str(),
            file_path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        )) {
        throw std::runtime_error("Failed to replace snapshot file");
    }
}

SnapshotData Snapshot::load() const {
    std::ifstream in(file_path, std::ios::binary);

    if (!in) {
        throw std::runtime_error("Failed to open snapshot file");
    }

    // Determine the file size so we can validate lengths before allocating.
    in.seekg(0, std::ios::end);
    std::streampos end = in.tellg();

    if (end < 0) {
        throw std::runtime_error("Failed to determine snapshot size");
    }

    const uint64_t file_size = static_cast<uint64_t>(end);
    in.seekg(0, std::ios::beg);

    auto read_exact = [&in](char* dst, std::streamsize size) {
        in.read(dst, size);

        if (!in || in.gcount() != size) {
            throw std::runtime_error("Snapshot is truncated or unreadable");
        }
    };

    auto read_u32 = [&read_exact]() {
        uint32_t value;
        read_exact(reinterpret_cast<char*>(&value), sizeof(value));
        return value;
    };

    auto read_u64 = [&read_exact]() {
        uint64_t value;
        read_exact(reinterpret_cast<char*>(&value), sizeof(value));
        return value;
    };

    // Validate magic bytes.
    char magic[4];
    read_exact(magic, sizeof(magic));

    if (magic[0] != 'K' || magic[1] != 'V' ||
        magic[2] != 'S' || magic[3] != 'N') {
        throw std::runtime_error("Invalid snapshot magic");
    }

    // Validate format version.
    uint32_t version = read_u32();

    if (version != 1) {
        throw std::runtime_error("Unsupported snapshot version");
    }

    SnapshotData snapshot;
    snapshot.lsn = read_u64();

    uint64_t record_count = read_u64();

    // Every entry needs at least two uint64_t length fields.
    constexpr uint64_t HEADER_SIZE = 24;
    constexpr uint64_t MIN_ENTRY_SIZE = 16;

    if (file_size < HEADER_SIZE ||
        record_count > (file_size - HEADER_SIZE) / MIN_ENTRY_SIZE) {
        throw std::runtime_error("Invalid snapshot record count");
    }

    for (uint64_t i = 0; i < record_count; ++i) {
        uint64_t key_size = read_u64();

        auto pos = in.tellg();
        if (pos < 0 || static_cast<uint64_t>(pos) > file_size ||
            key_size > file_size - static_cast<uint64_t>(pos) ||
            key_size > static_cast<uint64_t>(
                std::numeric_limits<std::streamsize>::max())) {
            throw std::runtime_error("Invalid snapshot key length");
        }

        std::string key(static_cast<size_t>(key_size), '\0');
        read_exact(key.data(), static_cast<std::streamsize>(key_size));

        uint64_t value_size = read_u64();

        pos = in.tellg();
        if (pos < 0 || static_cast<uint64_t>(pos) > file_size ||
            value_size > file_size - static_cast<uint64_t>(pos) ||
            value_size > static_cast<uint64_t>(
                std::numeric_limits<std::streamsize>::max())) {
            throw std::runtime_error("Invalid snapshot value length");
        }

        std::string value(static_cast<size_t>(value_size), '\0');
        read_exact(value.data(), static_cast<std::streamsize>(value_size));

        if (!snapshot.data.emplace(std::move(key), std::move(value)).second) {
            throw std::runtime_error("Duplicate key in snapshot");
        }
    }

    // Reject trailing bytes instead of silently accepting a malformed file.
    if (in.peek() != std::char_traits<char>::eof()) {
        throw std::runtime_error("Unexpected trailing data in snapshot");
    }

    if (in.bad()) {
        throw std::runtime_error("Failed while reading snapshot");
    }

    return snapshot;
}
