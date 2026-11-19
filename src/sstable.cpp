
#include "../include/sstable.hpp"

#include <fstream>
#include <stdexcept>
#include <limits>
#include <filesystem>
#include <algorithm>
#include <utility>

namespace {

// Write a fixed-width integer in little-endian format.
template <typename T>
void write_number(std::ostream& out, T value) {
    for (size_t i = 0; i < sizeof(T); i++) {
        uint8_t byte = static_cast<uint8_t>(
            value & 0xFF
        );

        out.put(static_cast<char>(byte));
        value >>= 8;
    }

    if (!out) {
        throw std::runtime_error("Failed to write SSTable");
    }
}

// Read a fixed-width integer in little-endian format.
template <typename T>
T read_number(std::istream& in) {
    T value = 0;

    for (size_t i = 0; i < sizeof(T); i++) {
        int ch = in.get();

        if (ch == EOF) {
            throw std::runtime_error("Corrupted SSTable");
        }

        value |= static_cast<T>(
            static_cast<uint8_t>(ch)
        ) << (8 * i);
    }

    return value;
}

// Write a length-prefixed string.
void write_string(
    std::ostream& out,
    const std::string& str
) {
    if (str.size() > std::numeric_limits<uint32_t>::max()) {
        throw std::runtime_error("String too large");
    }

    write_number<uint32_t>(
        out,
        static_cast<uint32_t>(str.size())
    );

    out.write(
        str.data(),
        static_cast<std::streamsize>(str.size())
    );

    if (!out) {
        throw std::runtime_error("Failed to write string");
    }
}

// Read a length-prefixed string.
std::string read_string(std::istream& in) {
    uint32_t len = read_number<uint32_t>(in);

    // Initial safety limit: 64 MB per field.
    if (len > 64 * 1024 * 1024) {
        throw std::runtime_error("Invalid string length");
    }

    std::string str(len, '\0');

    in.read(
        str.data(),
        static_cast<std::streamsize>(len)
    );

    if (!in) {
        throw std::runtime_error("Incomplete SSTable record");
    }

    return str;
}
}


void SSTable::create(const std::string& path,const MemTable& mem) {
    namespace fs = std::filesystem;

    if (mem.size() > std::numeric_limits<uint32_t>::max()) {
        throw std::runtime_error("Too many records");
    }

    fs::path final_path(path);
    fs::path temp_path = path + ".tmp";

    if (fs::exists(final_path)) {
        throw std::runtime_error("SSTable already exists");
    }

    std::ofstream out(
        temp_path,
        std::ios::binary | std::ios::trunc
    );

    if (!out) {
        throw std::runtime_error("Cannot create SSTable");
    }

    try {
        // 1. Write header.
        write_number<uint32_t>(out, MAGIC);
        write_number<uint32_t>(out, VERSION);
        write_number<uint64_t>(out, mem.size());

        // 2. Write sorted records.
        for (const auto& [key, entry] : mem.get_entries()) {
            write_string(out, key);
            write_string(out, entry.value);

            write_number<uint64_t>(out, entry.lsn);

            write_number<uint8_t>(
                out,
                entry.is_deleted ? 1 : 0
            );
        }

        out.flush();

        if (!out) {
            throw std::runtime_error("SSTable flush failed");
        }

        out.close();

        if (!out) {
            throw std::runtime_error("SSTable close failed");
        }

        // 3. Publish the completed file.
        fs::rename(temp_path, final_path);
    }
    catch (...) {
        out.close();

        std::error_code ec;
        fs::remove(temp_path, ec);

        throw;
    }
}



void SSTable::open(const std::string& path) {
    std::ifstream in(path, std::ios::binary);

    if (!in) {
        throw std::runtime_error("Cannot open SSTable");
    }

    // Validate header.
    uint32_t magic = read_number<uint32_t>(in);
    uint32_t version = read_number<uint32_t>(in);

    uint64_t count = read_number<uint64_t>(in);

    if (magic != MAGIC || version != VERSION) {
        throw std::runtime_error("Invalid SSTable header");
    }

    if (count > 100000000) {
        throw std::runtime_error("Invalid record count");
    }

    std::vector<IndexEntry> new_index;

    std::string previous_key;

    for (uint64_t i = 0; i < count; i++) {
        auto pos = in.tellg();

        if (pos < 0) {
            throw std::runtime_error("Invalid record offset");
        }

        std::string key = read_string(in);
        std::string value = read_string(in);

        uint64_t lsn = read_number<uint64_t>(in);
        uint8_t deleted = read_number<uint8_t>(in);

        if (key.empty() || deleted > 1) {
            throw std::runtime_error("Invalid SSTable record");
        }

        if (deleted && !value.empty()) {
            throw std::runtime_error("Invalid tombstone");
        }

        // SSTable keys must be strictly increasing.
        if (i > 0 && key <= previous_key) {
            throw std::runtime_error("SSTable is not sorted");
        }

        // Store one index entry every 16 records.
        if (i % INDEX_INTERVAL == 0) {
            new_index.push_back({
                key,
                static_cast<uint64_t>(pos),
                i
            });
        }

        previous_key = std::move(key);
    }

    // Reject unexpected trailing data.
    if (in.peek() != EOF) {
        throw std::runtime_error("Unexpected SSTable trailing data");
    }

    file_path = path;
    index = std::move(new_index);
    record_count = count;
}



std::optional<Entry> SSTable::get(const std::string& key) const {
    if (record_count == 0 || index.empty()) {
        return std::nullopt;
    }

    // Find the last index key <= requested key.
    auto it = std::upper_bound(
        index.begin(),
        index.end(),
        key,
        [](const std::string& k, const IndexEntry& e) {
            return k < e.key;
        }
    );

    // Requested key is smaller than the first key.
    if (it == index.begin()) {
        return std::nullopt;
    }

    --it;

    std::ifstream in(file_path, std::ios::binary);

    if (!in) {
        throw std::runtime_error("Cannot open SSTable");
    }

    in.seekg(static_cast<std::streamoff>(it->offset));

    if (!in) {
        throw std::runtime_error("Invalid SSTable offset");
    }

    uint64_t remaining = std::min(
        static_cast<uint64_t>(INDEX_INTERVAL),
        record_count - it->record_index
    );


    // Scan the selected block.
    for (uint64_t i = 0; i < remaining; i++) {
        std::string current_key = read_string(in);
        std::string value = read_string(in);

        uint64_t lsn = read_number<uint64_t>(in);
        uint8_t deleted = read_number<uint8_t>(in);

        if (current_key == key) {
            return Entry{
                std::move(value),
                lsn,
                deleted == 1
            };
        }

        // Keys are sorted, so no need to continue.
        if (current_key > key) {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

uint64_t SSTable::size() const {
    return record_count;
}
