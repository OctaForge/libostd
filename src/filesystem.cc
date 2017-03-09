/* Filesystem implementation bits.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#include "ostd/filesystem.hh"
#include "ostd/io.hh"

namespace ostd {

#ifdef OSTD_PLATFORM_WIN32
static inline time_t filetime_to_time_t(FILETIME const &ft) {
    ULARGE_INTEGER ul;
    ul.LowPart  = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    return static_cast<time_t>((ul.QuadPart / 10000000ULL) - 11644473600ULL);
}
#endif

void file_info::init_from_str(string_range path) {
    /* TODO: implement a version that will follow symbolic links */
    p_path = path;
#ifdef OSTD_PLATFORM_WIN32
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!GetFileAttributesEx(p_path.data(), GetFileExInfoStandard, &attr) ||
        attr.dwFileAttributes == INVALID_FILE_ATTRIBUTES)
#else
    struct stat st;
    if (lstat(p_path.data(), &st) < 0)
#endif
    {
        p_slash = p_dot = std::string::npos;
        p_type = file_type::UNKNOWN;
        p_path.clear();
        p_atime = p_mtime = p_ctime = 0;
        return;
    }
    string_range r = p_path;

    string_range found = find_last(r, PATH_SEPARATOR);
    if (found.empty()) {
        p_slash = std::string::npos;
    } else {
        p_slash = r.distance_front(found);
    }

    found = find(filename(), '.');
    if (found.empty()) {
        p_dot = std::string::npos;
    } else {
        p_dot = r.distance_front(found);
    }

#ifdef OSTD_PLATFORM_WIN32
    if (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        p_type = file_type::DIRECTORY;
    } else if (attr.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        p_type = file_type::SYMLINK;
    } else if (attr.dwFileAttributes & (
        FILE_ATTRIBUTE_ARCHIVE     | FILE_ATTRIBUTE_COMPRESSED |
        FILE_ATTRIBUTE_HIDDEN      | FILE_ATTRIBUTE_NORMAL     |
        FILE_ATTRIBUTE_SPARSE_FILE | FILE_ATTRIBUTE_TEMPORARY
    )) {
        p_type = file_type::REGULAR;
    } else {
        p_type = file_type::UNKNOWN;
    }

    p_atime = detail::filetime_to_time_t(attr.ftLastAccessTime);
    p_mtime = detail::filetime_to_time_t(attr.ftLastWriteTime);
    p_ctime = detail::filetime_to_time_t(attr.ftCreationTime);
#else
    if (S_ISREG(st.st_mode)) {
        p_type = file_type::REGULAR;
    } else if (S_ISDIR(st.st_mode)) {
        p_type = file_type::DIRECTORY;
    } else if (S_ISCHR(st.st_mode)) {
        p_type = file_type::CHARACTER;
    } else if (S_ISBLK(st.st_mode)) {
        p_type = file_type::BLOCK;
    } else if (S_ISFIFO(st.st_mode)) {
        p_type = file_type::FIFO;
    } else if (S_ISLNK(st.st_mode)) {
        p_type = file_type::SYMLINK;
    } else if (S_ISSOCK(st.st_mode)) {
        p_type = file_type::SOCKET;
    } else {
        p_type = file_type::UNKNOWN;
    }

    p_atime = st.st_atime;
    p_mtime = st.st_mtime;
    p_ctime = st.st_ctime;
#endif
}

} /* namespace ostd */
