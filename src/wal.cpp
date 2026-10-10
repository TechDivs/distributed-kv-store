#include "../include/wal.hpp"
#include "../include/kv_store.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

using std::string;

WAL::WAL(const string& path)
    : file_path(path),
    file_handle(INVALID_HANDLE_VALUE),
    next_lsn(1) {

    file_handle = CreateFileA(
        file_path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (file_handle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error(
            "Failed to open WAL file"
        );
    }

    // Determine the next LSN from the existing WAL.
    std::ifstream input(file_path);

    if (!input) {
        CloseHandle(file_handle);
        file_handle = INVALID_HANDLE_VALUE;

        throw std::runtime_error(
            "Failed to read WAL file"
        );
    }

    string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream iss(line);

        uint64_t lsn;
        char delimiter;

        if (!(iss >> lsn >> delimiter) || delimiter != '|') {
            // For now, ignore malformed trailing data.
            // We will make recovery validation stricter shortly.
            continue;
        }

        next_lsn = next_lsn>lsn+1 ? next_lsn:lsn+1;
    }
}

WAL::~WAL() {
    if (file_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(file_handle);
        file_handle = INVALID_HANDLE_VALUE;
    }
}

uint64_t WAL::get_next_lsn() const {
    return next_lsn;
}

void WAL::flush_to_disk() {
    if (!FlushFileBuffers(file_handle)) {
        throw std::runtime_error(
            "Failed to flush WAL to disk"
        );
    }
}

uint64_t WAL::append_set(const string& key, const string& value) {
    uint64_t lsn = next_lsn;

    string record =
        std::to_string(lsn) +
        "|SET|" +
        key +
        "|" +
        value +
        "\n";

    LARGE_INTEGER offset;
    offset.QuadPart = 0;

    if (!SetFilePointerEx(
            file_handle,
            offset,
            nullptr,
            FILE_END
        )) {

        throw std::runtime_error(
            "Failed to seek to end of WAL"
        );
    }
    DWORD bytes_written = 0;

    BOOL result = WriteFile(
        file_handle,
        record.data(),
        static_cast<DWORD>(record.size()),
        &bytes_written,
        nullptr
    );

    if (!result || bytes_written != record.size()) {
        throw std::runtime_error("Failed to write SET record to WAL");
    }

    flush_to_disk();
    next_lsn++;

    return lsn;
}

uint64_t WAL::append_delete(const string& key) {
    uint64_t lsn = next_lsn;

    string record =
        std::to_string(lsn) +
        "|DELETE|" +
        key +
        "|\n";

    LARGE_INTEGER offset;
    offset.QuadPart = 0;

    if (!SetFilePointerEx(
            file_handle,
            offset,
            nullptr,
            FILE_END
        )) {

        throw std::runtime_error(
            "Failed to seek to end of WAL"
        );
    }
    DWORD bytes_written = 0;

    BOOL result = WriteFile(
        file_handle,
        record.data(),
        static_cast<DWORD>(record.size()),
        &bytes_written,
        nullptr
    );

    if (!result || bytes_written != record.size()) {
        throw std::runtime_error("Failed to write DELETE record to WAL");
    }

    flush_to_disk();
    next_lsn++;

    return lsn;
}

void WAL::recover(KVStore& store) {
    std::ifstream input(file_path);

    if (!input) {
        throw std::runtime_error(
            "WAL recovery failed: could not open WAL"
        );
    }

    std::string line;
    uint64_t expected_lsn = 1;

    while (std::getline(input, line)) {

        if (line.empty()) {
            continue;
        }

        std::istringstream iss(line);

        uint64_t lsn;
        char delimiter;
        std::string operation;
        std::string key;
        std::string value;

        // LSN
        if (!(iss >> lsn)) {
            throw std::runtime_error(
                "WAL recovery failed: invalid LSN"
            );
        }

        // First '|'
        if (!(iss >> delimiter) || delimiter != '|') {
            throw std::runtime_error(
                "WAL recovery failed: invalid delimiter"
            );
        }

        // LSN must be strictly sequential.
        if (lsn != expected_lsn) {
            throw std::runtime_error(
                "WAL recovery failed: invalid LSN sequence"
            );
        }

        // Operation
        if (!std::getline(iss, operation, '|')) {
            throw std::runtime_error(
                "WAL recovery failed: missing operation"
            );
        }

        if (operation == "SET") {

            if (!std::getline(iss, key, '|')) {
                throw std::runtime_error(
                    "WAL recovery failed: incomplete SET record"
                );
            }

            if (!std::getline(iss, value)) {
                throw std::runtime_error(
                    "WAL recovery failed: incomplete SET record"
                );
            }

            if (key.empty()) {
                throw std::runtime_error(
                    "WAL recovery failed: empty SET key"
                );
            }

            store.set_in_memory(key, value);
        }

        else if (operation == "DELETE") {

            if (!std::getline(iss, key, '|')) {
                throw std::runtime_error(
                    "WAL recovery failed: incomplete DELETE record"
                );
            }

            if (key.empty()) {
                throw std::runtime_error(
                    "WAL recovery failed: empty DELETE key"
                );
            }

            store.remove_in_memory(key);
        }

        else {
            throw std::runtime_error(
                "WAL recovery failed: unknown operation"
            );
        }

        expected_lsn++;
    }
}