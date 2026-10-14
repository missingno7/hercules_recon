#ifndef HERCULES_DROPIN_CORE_ARCHIVE_READER_HPP
#define HERCULES_DROPIN_CORE_ARCHIVE_READER_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace hercules::dropin {

enum class ArchiveReadStatus : std::uint8_t {
    ok,
    not_found,
    corrupt,
    unsupported_name,
    io_error,
    limit_exceeded
};

struct ArchiveMember {
    ArchiveReadStatus status = ArchiveReadStatus::io_error;
    std::uint32_t key = 0;
    std::uint32_t index = 0; // Zero-based directory row.
    std::uint64_t offset = 0;
    std::uint64_t length = 0;
    std::vector<std::uint8_t> bytes;
    std::string message;
};

constexpr std::uint64_t kDefaultArchiveMemberLimit = 16ULL * 1024ULL * 1024ULL;

/* Read one member from a Hercules FS archive. member_path must already be a
 * canonical uppercase ASCII M:\\ path; this function performs no normalization.
 * The returned byte vector owns its contents. The archive is opened read-only. */
ArchiveMember read_archive_member(
    const std::string &archive_path,
    const std::string &member_path,
    std::uint64_t max_member_bytes = kDefaultArchiveMemberLimit);

} // namespace hercules::dropin

#endif
