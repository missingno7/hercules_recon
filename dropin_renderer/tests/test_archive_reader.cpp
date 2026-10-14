#include "../core/archive_reader.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" int name_hash(const char *name);

namespace {

constexpr std::size_t kDirectorySize = 0x3000;
constexpr std::size_t kRowSize = 12;
int checks = 0;
int failures = 0;
unsigned int archive_serial = 0;

void check(bool condition, const char *description)
{
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

std::uint32_t path_key(const std::string &path)
{
    return static_cast<std::uint32_t>(name_hash(path.c_str()));
}

void put_u32_le(std::vector<unsigned char> &bytes, std::size_t offset,
                std::uint32_t value)
{
    bytes[offset] = static_cast<unsigned char>(value & 0xffU);
    bytes[offset + 1] = static_cast<unsigned char>((value >> 8) & 0xffU);
    bytes[offset + 2] = static_cast<unsigned char>((value >> 16) & 0xffU);
    bytes[offset + 3] = static_cast<unsigned char>((value >> 24) & 0xffU);
}

void put_row(std::vector<unsigned char> &bytes, std::size_t index,
             std::uint32_t key, std::uint32_t offset, std::uint32_t length)
{
    const std::size_t start = index * kRowSize;
    put_u32_le(bytes, start, key);
    put_u32_le(bytes, start + 4, offset);
    put_u32_le(bytes, start + 8, length);
}

std::filesystem::path next_archive_path()
{
    const auto path = std::filesystem::temp_directory_path() /
                      ("hercules_archive_reader_" + std::to_string(++archive_serial) + ".fs");
    return path;
}

void write_bytes(const std::filesystem::path &path,
                 const std::vector<unsigned char> &bytes)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file || (!bytes.empty() &&
                  !file.write(reinterpret_cast<const char *>(bytes.data()),
                              static_cast<std::streamsize>(bytes.size()))))
        throw std::runtime_error("could not write synthetic archive fixture");
}

struct TempArchive {
    std::filesystem::path path = next_archive_path();
    ~TempArchive()
    {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
    }
};

using hercules::dropin::ArchiveReadStatus;
using hercules::dropin::read_archive_member;

void test_valid_member_and_empty_member()
{
    const std::string payload_path = "M:\\DATA\\PAYLOAD.BIN";
    const std::string empty_path = "M:\\DATA\\EMPTY.BIN";
    const std::vector<unsigned char> payload = {0x11, 0x00, 0x7f, 0x80, 0xaa};
    const std::uint32_t payload_key = path_key(payload_path);
    const std::uint32_t empty_key = path_key(empty_path);
    check(payload_key != empty_key, "fixture paths have distinct actual helper keys");

    struct Entry { std::uint32_t key; const std::string *path; };
    std::vector<Entry> entries = {{payload_key, &payload_path}, {empty_key, &empty_path}};
    std::sort(entries.begin(), entries.end(), [](const Entry &left, const Entry &right) {
        return left.key < right.key;
    });

    const std::uint32_t payload_offset = static_cast<std::uint32_t>(kDirectorySize);
    const std::uint32_t empty_offset = payload_offset + static_cast<std::uint32_t>(payload.size());
    std::vector<unsigned char> bytes(kDirectorySize + payload.size(), 0);
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (entries[index].key == payload_key)
            put_row(bytes, index, entries[index].key, payload_offset,
                    static_cast<std::uint32_t>(payload.size()));
        else
            put_row(bytes, index, entries[index].key, empty_offset, 0);
    }
    /* A zero key ends the table; later rows are outside the recovered prefix. */
    put_row(bytes, entries.size(), 0, 0xffffffffU, 0xffffffffU);
    put_row(bytes, entries.size() + 1, 1, 0xffffffffU, 0xffffffffU);
    std::copy(payload.begin(), payload.end(), bytes.begin() + payload_offset);

    TempArchive archive;
    write_bytes(archive.path, bytes);

    const auto payload_result = read_archive_member(archive.path.string(), payload_path);
    check(payload_result.status == ArchiveReadStatus::ok, "valid member status is ok");
    check(payload_result.key == payload_key, "result key is the actual historical helper key");
    const auto payload_row = std::find_if(entries.begin(), entries.end(), [payload_key](const Entry &entry) {
        return entry.key == payload_key;
    });
    check(payload_result.index == static_cast<std::uint32_t>(payload_row - entries.begin()),
          "result reports the zero-based directory row");
    check(payload_result.offset == payload_offset && payload_result.length == payload.size(),
          "result reports the stored payload offset and length");
    check(payload_result.bytes == payload, "valid member bytes are returned exactly and owned");

    const auto empty_result = read_archive_member(archive.path.string(), empty_path);
    check(empty_result.status == ArchiveReadStatus::ok, "valid empty member status is ok");
    check(empty_result.key == empty_key && empty_result.length == 0 && empty_result.bytes.empty(),
          "empty member retains key and zero length with owned empty bytes");
}

void test_missing_and_invalid_names()
{
    const std::string existing = "M:\\DATA\\PRESENT.BIN";
    const std::string absent = "M:\\DATA\\ABSENT.BIN";
    check(path_key(existing) != path_key(absent), "missing fixture keys do not collide");

    std::vector<unsigned char> bytes(kDirectorySize, 0);
    put_row(bytes, 0, path_key(existing), static_cast<std::uint32_t>(kDirectorySize), 0);
    TempArchive archive;
    write_bytes(archive.path, bytes);
    check(read_archive_member(archive.path.string(), absent).status == ArchiveReadStatus::not_found,
          "missing member reports not_found");

    check(read_archive_member(archive.path.string(), "m:\\DATA\\PRESENT.BIN").status ==
              ArchiveReadStatus::unsupported_name,
          "lowercase drive form is rejected");
    check(read_archive_member(archive.path.string(), "DATA\\PRESENT.BIN").status ==
              ArchiveReadStatus::unsupported_name,
          "relative form is rejected");
    check(read_archive_member(archive.path.string(), "M:/DATA/PRESENT.BIN").status ==
              ArchiveReadStatus::unsupported_name,
          "forward-slash form is rejected without normalization");
    check(read_archive_member(archive.path.string(), "M:\\DATA\\..\\PRESENT.BIN").status ==
              ArchiveReadStatus::unsupported_name,
          "parent path component is rejected without normalization");
    std::string non_ascii = "M:\\DATA\\BAD";
    non_ascii.push_back(static_cast<char>(0x80));
    non_ascii += ".BIN";
    check(read_archive_member(archive.path.string(), non_ascii).status ==
              ArchiveReadStatus::unsupported_name,
          "non-ASCII member name is rejected");
}

void test_directory_validation()
{
    {
        std::vector<unsigned char> bytes(kDirectorySize, 0);
        put_row(bytes, 0, 200, static_cast<std::uint32_t>(kDirectorySize), 0);
        put_row(bytes, 1, 100, static_cast<std::uint32_t>(kDirectorySize), 0);
        TempArchive archive;
        write_bytes(archive.path, bytes);
        check(read_archive_member(archive.path.string(), "M:\\DATA\\ANY.BIN").status ==
                  ArchiveReadStatus::corrupt,
              "descending keys report corrupt directory");
    }
    {
        std::vector<unsigned char> bytes(kDirectorySize, 0);
        put_row(bytes, 0, 100, static_cast<std::uint32_t>(kDirectorySize), 0);
        put_row(bytes, 1, 100, static_cast<std::uint32_t>(kDirectorySize), 0);
        TempArchive archive;
        write_bytes(archive.path, bytes);
        check(read_archive_member(archive.path.string(), "M:\\DATA\\ANY.BIN").status ==
                  ArchiveReadStatus::corrupt,
              "duplicate keys report corrupt directory");
    }
    {
        std::vector<unsigned char> bytes(kDirectorySize, 0);
        put_row(bytes, 0, 100, 0xfffffffeU, 4);
        TempArchive archive;
        write_bytes(archive.path, bytes);
        check(read_archive_member(archive.path.string(), "M:\\DATA\\ANY.BIN").status ==
                  ArchiveReadStatus::corrupt,
              "a member range that wraps 32-bit addition is rejected against file size");
    }
    {
        std::vector<unsigned char> bytes(kDirectorySize - 1, 0);
        TempArchive archive;
        write_bytes(archive.path, bytes);
        check(read_archive_member(archive.path.string(), "M:\\DATA\\ANY.BIN").status ==
                  ArchiveReadStatus::corrupt,
              "truncated directory header reports corrupt");
    }
    {
        std::vector<unsigned char> bytes(kDirectorySize, 0);
        for (std::size_t index = 0; index < kDirectorySize / kRowSize; ++index)
            put_row(bytes, index, static_cast<std::uint32_t>(index + 1), 0, 0);
        TempArchive archive;
        write_bytes(archive.path, bytes);
        check(read_archive_member(archive.path.string(), "M:\\DATA\\ANY.BIN").status ==
                  ArchiveReadStatus::corrupt,
              "directory without a first zero-key terminator is rejected");
    }
}

void test_limit_and_io_errors()
{
    const std::string path = "M:\\DATA\\LIMIT.BIN";
    const std::vector<unsigned char> payload = {1, 2, 3, 4};
    std::vector<unsigned char> bytes(kDirectorySize + payload.size(), 0);
    put_row(bytes, 0, path_key(path), static_cast<std::uint32_t>(kDirectorySize),
            static_cast<std::uint32_t>(payload.size()));
    std::copy(payload.begin(), payload.end(), bytes.begin() + kDirectorySize);
    TempArchive archive;
    write_bytes(archive.path, bytes);
    check(read_archive_member(archive.path.string(), path, 3).status ==
              ArchiveReadStatus::limit_exceeded,
          "caller member limit rejects before allocation and reading");

    const auto missing_archive = next_archive_path();
    std::error_code ignored;
    std::filesystem::remove(missing_archive, ignored);
    check(read_archive_member(missing_archive.string(), path).status == ArchiveReadStatus::io_error,
          "unopenable archive reports io_error");
}

} // namespace

int main()
{
    test_valid_member_and_empty_member();
    test_missing_and_invalid_names();
    test_directory_validation();
    test_limit_and_io_errors();
    std::cout << "archive reader checks: " << checks << ", failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
