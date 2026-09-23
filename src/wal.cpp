#include "../include/wal.hpp"
#include "../include/kv_store.hpp"

#include <filesystem>
#include <sstream>
#include <stdexcept>

using std::string;

WAL::WAL(const string& path)
    : file_path(path), next_lsn(1) {

    // Create the file if it doesn't exist.
    {
        std::ofstream create_file(file_path, std::ios::app);

        if (!create_file) {
            throw std::runtime_error(
                "Failed to create WAL file"
            );
        }
    }

    // Open for reading and appending.
    file.open(
        file_path,
        std::ios::in |
        std::ios::out |
        std::ios::app
    );

    if (!file.is_open()) {
        throw std::runtime_error(
            "Failed to open WAL file"
        );
    }

    // Find the next available LSN.
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);

        uint64_t lsn;
        char delimiter;

        if (iss >> lsn >> delimiter) {
            if (delimiter == '|') {
                next_lsn = std::max(next_lsn, lsn + 1);
            }
        }
    }

    // Clear EOF state after reading.
    file.clear();
}

uint64_t WAL::append_set(const string& key, const string& value) {
    uint64_t lsn = next_lsn++;
    file.clear();

    file << lsn
        << "|SET|"
        << key
        << "|"
        << value
        << "\n";

    file.flush();

    if (!file) {
        throw std::runtime_error("Failed to append SET to WAL");
    }

    return lsn;
}

uint64_t WAL::append_delete(const std::string& key) {
    uint64_t lsn = next_lsn++;
    file.clear();

    file << lsn
        << "|DELETE|"
        << key 
        << "|"
        << "\n";

    file.flush();

    if (!file) {
        throw std::runtime_error("Failed to append DELETE to WAL");
    }

    return lsn;
}

void WAL::recover(KVStore& store) {
    file.clear();

    file.seekg(0,std::ios::beg);
    std::string line;

    while(std::getline(file,line)) {
        if(line.empty()) continue;

        std::istringstream iss(line);

        uint64_t lsn;
        char delimiter;

        std::string op,key,value;

        if(!(iss>>lsn)) {
            throw std::runtime_error("Invalid WAL Record: LSN");
        }
        if(!(iss>>delimiter) || (delimiter != '|')) {
            throw std::runtime_error("Invalid WAL Record: delimiter");
        }

        std::getline(iss,op,'|');
        if(op == "SET") {
            std::getline(iss,key,'|');
            std::getline(iss,value);

            if(key.empty()) {
                throw std::runtime_error("Invalid SET record");
            }

            store.set(key,value);
        }
        else if(op == "DELETE") {
            std::getline(iss,key);
            
            if(key.empty()) {
                throw std::runtime_error("Invalid DELETE record");
            }

            store.remove(key);
        }
        else {
            throw std::runtime_error("Unknown WAL operation");
        }
    }

    file.clear();
}