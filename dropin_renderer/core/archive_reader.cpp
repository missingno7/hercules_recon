#include "archive_reader.hpp"

#include <array>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

extern "C" int name_hash(const char *name);

namespace hercules::dropin {
namespace {

constexpr std::uint64_t kDirectoryBytes = 0x3000ULL;
constexpr std::size_t kDirectoryRows = 1024;
constexpr std::size_t kDirectoryRowBytes = 12;

struct DirectoryRow {
    std::uint32_t key;
    std::uint32_t offset;
    std::uint32_t length;
};

ArchiveMember failure(ArchiveReadStatus status, const char *message)
{
    ArchiveMember result;
    result.status = status;
    result.message = message;
    return result;
}

std::uint32_t load_u32_le(const unsigned char *bytes)
{
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8) |
           (static_cast<std::uint32_t>(bytes[2]) << 16) |
           (static_cast<std::uint32_t>(bytes[3]) << 24);
}

bool is_supported_member_path(const std::string &path)
{
    if (path.size() <= 3 || path[0] != 'M' || path[1] != ':' || path[2] != '\\')
        return false;

    std::size_t component_start = 3;
    for (std::size_t i = 3; i < path.size(); ++i) {
        const unsigned char ch = static_cast<unsigned char>(path[i]);
        if (ch < 0x20 || ch > 0x7e || (ch >= 'a' && ch <= 'z') ||
            ch == '/' || ch == ':')
            return false;

        if (ch == '\\') {
            if (i == component_start)
                return false;
            const std::string component = path.substr(component_start, i - component_start);
            if (component == "." || component == "..")
                return false;
            component_start = i + 1;
        }
    }

    if (component_start == path.size())
        return false;
    const std::string last_component = path.substr(component_start);
    return last_component != "." && last_component != "..";
}

bool read_exact(std::ifstream &stream, char *destination, std::size_t count)
{
    if (count > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
        return false;
    stream.read(destination, static_cast<std::streamsize>(count));
    return stream.good() && stream.gcount() == static_cast<std::streamsize>(count);
}

} // namespace

ArchiveMember read_archive_member(const std::string &archive_path,
                                  const std::string &member_path,
                                  std::uint64_t max_member_bytes)
{
    if (!is_supported_member_path(member_path))
        return failure(ArchiveReadStatus::unsupported_name,
                       "member path must be canonical uppercase ASCII M:\\ form");

    std::ifstream stream(archive_path.c_str(), std::ios::binary);
    if (!stream.is_open())
        return failure(ArchiveReadStatus::io_error, "could not open archive for reading");

    stream.seekg(0, std::ios::end);
    const std::streamoff end_position = stream.tellg();
    if (!stream.good() || end_position < 0)
        return failure(ArchiveReadStatus::io_error, "could not determine archive length");
    const std::uint64_t file_size = static_cast<std::uint64_t>(end_position);
    if (file_size < kDirectoryBytes)
        return failure(ArchiveReadStatus::corrupt, "archive is shorter than its directory header");

    stream.seekg(0, std::ios::beg);
    if (!stream.good())
        return failure(ArchiveReadStatus::io_error, "could not seek to archive directory");

    std::array<unsigned char, static_cast<std::size_t>(kDirectoryBytes)> raw_directory{};
    if (!read_exact(stream, reinterpret_cast<char *>(raw_directory.data()), raw_directory.size()))
        return failure(ArchiveReadStatus::corrupt, "archive directory header is truncated");

    std::vector<DirectoryRow> rows;
    rows.reserve(kDirectoryRows);
    std::uint32_t previous_key = 0;
    bool terminated = false;
    for (std::size_t index = 0; index < kDirectoryRows; ++index) {
        const unsigned char *row_bytes = raw_directory.data() + index * kDirectoryRowBytes;
        const DirectoryRow row = {load_u32_le(row_bytes),
                                  load_u32_le(row_bytes + 4),
                                  load_u32_le(row_bytes + 8)};
        if (row.key == 0) {
            terminated = true;
            break;
        }
        if (!rows.empty() && row.key <= previous_key)
            return failure(ArchiveReadStatus::corrupt,
                           "archive directory keys are not strictly increasing");

        const std::uint64_t offset = static_cast<std::uint64_t>(row.offset);
        const std::uint64_t length = static_cast<std::uint64_t>(row.length);
        if (offset > std::numeric_limits<std::uint64_t>::max() - length)
            return failure(ArchiveReadStatus::corrupt, "archive member range overflows");
        const std::uint64_t range_end = offset + length;
        if (range_end > file_size)
            return failure(ArchiveReadStatus::corrupt,
                           "archive member range extends beyond the complete file");

        rows.push_back(row);
        previous_key = row.key;
    }
    if (!terminated)
        return failure(ArchiveReadStatus::corrupt, "archive directory has no zero-key terminator");

    /* The recovered backend hashes the result of its normalizer. This reader
     * accepts only already-canonical names, so it passes the supplied bytes to
     * the actual historical helper without duplicating either operation. */
    const std::uint32_t requested_key = static_cast<std::uint32_t>(name_hash(member_path.c_str()));
    for (std::size_t index = 0; index < rows.size(); ++index) {
        const DirectoryRow &row = rows[index];
        if (row.key != requested_key)
            continue;

        if (static_cast<std::uint64_t>(row.length) > max_member_bytes ||
            static_cast<std::uint64_t>(row.length) >
                static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) ||
            static_cast<std::uint64_t>(row.length) >
                static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())) {
            return failure(ArchiveReadStatus::limit_exceeded,
                           "member length exceeds the caller's read limit");
        }

        ArchiveMember result;
        result.status = ArchiveReadStatus::ok;
        result.key = row.key;
        result.index = static_cast<std::uint32_t>(index);
        result.offset = row.offset;
        result.length = row.length;
        result.bytes.resize(static_cast<std::size_t>(row.length));

        stream.clear();
        stream.seekg(static_cast<std::streamoff>(row.offset), std::ios::beg);
        if (!stream.good())
            return failure(ArchiveReadStatus::io_error, "could not seek to archive member");
        if (!result.bytes.empty() &&
            !read_exact(stream, reinterpret_cast<char *>(result.bytes.data()), result.bytes.size()))
            return failure(ArchiveReadStatus::io_error, "could not read complete archive member");
        return result;
    }

    return failure(ArchiveReadStatus::not_found, "member key is absent from archive directory");
}

} // namespace hercules::dropin
