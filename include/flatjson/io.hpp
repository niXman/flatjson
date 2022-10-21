
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2022 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__IO_HPP
#define __FLATJSON__IO_HPP

#ifndef __FLATJSON__FLATJSON_HPP
#   include "flatjson.hpp"
#endif // __FLATJSON__FLATJSON_HPP

#include <cstdio>

// OS headers inclusion
#if defined(__linux__) || defined(__APPLE__)
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <sys/mman.h>
#   include <sys/uio.h>
#   include <fcntl.h>
#   include <unistd.h>
#elif defined(WIN32)
#   include <windows.h>
#else
#   error "UNKNOWN PLATFORM!"
#endif // OS detection

namespace flatjson {

/*************************************************************************************************/
// declarations

#if defined(__linux__) || defined(__APPLE__)
using file_handle = int;
using char_type = char;
using io_vector = iovec;
#   define __FJ_INIT_IO_VECTOR(vec, ptr, size) \
        vec.iov_base = ptr; vec.iov_len = size;
#   define __FJ_IO_VECTOR_PTR(vec) vec->iov_base
#   define __FJ_IO_VECTOR_SIZE(vec) vec->iov_len
#elif defined(WIN32)
using file_handle = HANDLE;
using char_type = TCHAR;
struct io_vector {
    const void *iov_base;
    std::size_t iov_len;
};
#   define __FJ_INIT_IO_VECTOR(vec, ptr, size) \
        vec.iov_base = ptr; vec.iov_len = size;
#   define __FJ_IO_VECTOR_PTR(vec) vec->iov_base
#   define __FJ_IO_VECTOR_SIZE(vec) vec->iov_len
#else
#   error "UNKNOWN PLATFORM!"
#endif // OS detection

inline bool file_exists(const char_type *fname);
inline std::size_t file_size(const char_type *fname, int *ec = nullptr);
inline std::size_t file_size(file_handle fd, int *ec = nullptr);
inline std::size_t file_chsize(file_handle fd, std::size_t fsize, int *ec = nullptr);
inline bool file_handle_valid(file_handle fh);
inline file_handle file_open(const char_type *fname, int *ec = nullptr);
inline file_handle file_create(const char_type *fname, int *ec = nullptr);
inline std::size_t file_read(file_handle fd, void *ptr, std::size_t size, int *ec = nullptr);
inline std::size_t file_write(file_handle fd, const void *ptr, std::size_t size, int *ec = nullptr);
inline std::size_t file_write(
     file_handle fd
    ,const io_vector *iovector
    ,std::size_t num
    ,std::size_t total_bytes
    ,int *ec = nullptr
);
inline bool file_close(file_handle fd, int *ec = nullptr);
inline const void* mmap_for_read(file_handle fd, std::size_t size, int *ec = nullptr);
inline const void* mmap_for_read(file_handle *fd, const char_type *fname, int *ec = nullptr);
inline void* mmap_for_write(file_handle fd, std::size_t size, int *ec = nullptr);
inline void* mmap_for_write(file_handle *fd, const char_type *fname, std::size_t size, int *ec = nullptr);
inline bool munmap_file(const void *addr, std::size_t size, int *ec = nullptr);
inline bool munmap_file_fd(const void *addr, file_handle fd, int *ec = nullptr);

/*************************************************************************************************/
// implementations

#if defined(__linux__) || defined(__APPLE__)
bool file_exists(const char_type *fname) {
    return ::access(fname, 0) == 0;
}

std::size_t file_size(const char_type *fname, int *ec) {
    struct ::stat st;
    if ( ::stat(fname, &st) != 0 ) {
        *ec = errno;
        return 0;
    }

    return st.st_size;
}

std::size_t file_size(file_handle fd, int *ec) {
    struct ::stat st;
    if ( ::fstat(fd, &st) != 0 ) {
        if ( ec ) { *ec = errno; }
        return 0;
    }

    return st.st_size;
}

std::size_t file_chsize(file_handle fd, std::size_t fsize, int *ec) {
    if ( ::ftruncate(fd, fsize) != 0 ) {
        if ( ec ) { *ec = errno; }
        return 0;
    }

    int lec{};
    fsize = file_size(fd, &lec);
    if ( lec ) {
        if ( ec ) { *ec = lec; }
        return 0;
    }

    return fsize;
}

bool file_handle_valid(file_handle fh) {
    return fh != -1;
}

file_handle file_open(const char_type *fname, int *ec) {
    int fd = ::open(fname, O_RDONLY);
    if ( fd == -1 ) {
        if ( ec ) { *ec = errno; }
        return -1;
    }

    return fd;
}

file_handle file_create(const char_type *fname, int *ec) {
    int fd = ::open(fname, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
    if ( fd == -1 ) {
        if ( ec ) { *ec = errno; }
        return fd;
    }

    return fd;
}

std::size_t file_read(file_handle fd, void *ptr, std::size_t size, int *ec) {
    auto rd = static_cast<std::size_t>(::read(fd, ptr, size));
    if ( rd != size ) {
        if ( ec ) { *ec = errno; }
    }

    return rd;
}

std::size_t file_write(file_handle fd, const void *ptr, std::size_t size, int *ec) {
    auto wr = static_cast<std::size_t>(::write(fd, ptr, size));
    if ( wr != size ) {
        if ( ec ) { *ec = errno; }
    }

    return wr;
}

std::size_t file_write(
     file_handle fd
    ,const io_vector *iovector
    ,std::size_t num
    ,std::size_t total_bytes
    ,int *ec)
{
    auto wr = ::writev(fd, iovector, num);
    if ( wr == -1 ) {
        if ( ec ) { *ec = errno; }

        return 0;
    }
    if ( static_cast<std::size_t>(wr) != total_bytes ) {
        if ( ec ) { *ec = errno; }
    }

    return wr;
}

bool file_close(file_handle fd, int *ec) {
    bool ok = ::close(fd) == 0;
    if ( !ok ) {
        if ( ec ) { *ec = errno; }

        return false;
    }

    return true;
}

const void* mmap_for_read(file_handle fd, std::size_t size, int *ec) {
    void *addr = ::mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
    if ( addr == MAP_FAILED ) {
        if ( ec ) { *ec = errno; }

        return nullptr;
    }

    bool ok = ::posix_madvise(addr, size, POSIX_MADV_SEQUENTIAL) == 0;
    if ( !ok ) {
        if ( ec ) { *ec = errno; }

        return nullptr;
    }

    return addr;
}

const void* mmap_for_read(file_handle *fd, const char_type *fname, int *ec) {
    int lec{};
    int lfd = file_open(fname, &lec);
    if ( lec ) {
        if ( ec ) { *ec = lec; }
        return nullptr;
    }

    auto fsize = file_size(lfd);
    auto *addr = mmap_for_read(lfd, fsize, &lec);
    if ( !addr || lec ) {
        if ( ec ) { *ec = lec; }
        file_close(lfd, &lec);
        *fd = -1;
        return nullptr;
    }
    *fd = lfd;

    return addr;
}

void* mmap_for_write(file_handle fd, std::size_t size, int *ec) {
    void *addr = ::mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if ( addr == MAP_FAILED ) {
        if ( ec ) { *ec = errno; }

        return nullptr;
    }

    bool ok = ::posix_madvise(addr, size, POSIX_MADV_SEQUENTIAL) == 0;
    if ( !ok ) {
        if ( ec ) { *ec = errno; }

        return nullptr;
    }

    return addr;
}

void* mmap_for_write(file_handle *fd, const char_type *fname, std::size_t size, int *ec) {
    int lec{};
    int lfd = file_create(fname, &lec);
    if ( lec ) {
        if ( ec ) { *ec = lec; }
        return nullptr;
    }

    auto fsize = file_chsize(lfd, size, &lec);
    if ( lec ) {
        if ( ec ) { *ec = lec;  }
        file_close(lfd, &lec);
        return nullptr;
    }

    auto *addr = mmap_for_write(lfd, fsize, &lec);
    if ( !addr ) {
        if ( ec ) { *ec = lec; }
        file_close(lfd, &lec);
        return nullptr;
    }
    *fd = lfd;

    return addr;
}

bool munmap_file(const void *addr, std::size_t size, int *ec) {
    bool ok = ::munmap(const_cast<void *>(addr), size) == 0;
    if ( !ok ) {
        if ( ec ) { *ec = errno; }

        return false;
    }

    return true;
}

bool munmap_file_fd(const void *addr, file_handle fd, int *ec) {
    int lec{};
    auto fsize = file_size(fd, &lec);
    if ( lec ) {
        if ( ec ) { *ec = lec; }
        return false;
    }
    if ( !munmap_file(addr, fsize, &lec) ) {
        if ( ec ) { *ec = lec; }
        return false;
    }
    if ( !file_close(fd, &lec) ) {
        if ( ec ) { *ec = lec; }
        return false;
    }

    return true;
}

#elif defined(WIN32)

bool file_exists(const char_type *fname) {
    auto attr = ::GetFileAttributes(fname);

    return (attr != INVALID_FILE_ATTRIBUTES &&
        !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

std::size_t file_size(const char_type *fname, int *ec) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if ( !::GetFileAttributesEx(fname, ::GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &fad) ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return 0;
    }

    std::size_t fsize = fad.nFileSizeLow;
    return fsize;
}

std::size_t file_size(file_handle fd, int *ec) {
    LARGE_INTEGER fsize;
    if ( !::GetFileSizeEx(fd, &fsize) ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return 0;
    }

    return static_cast<std::size_t>(fsize.QuadPart);
}

std::size_t file_chsize(file_handle fd, std::size_t fsize, int *ec) {
    auto ret = ::SetFilePointer(fd, static_cast<LONG>(fsize), nullptr, FILE_BEGIN);
    if ( ret == INVALID_SET_FILE_POINTER ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return 0;
    }
    bool ok = ::SetEndOfFile(fd);
    if ( !ok ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return 0;
    }

    return fsize;
}

bool file_handle_valid(file_handle fh) {
    return fh != INVALID_HANDLE_VALUE;
}

file_handle file_open(const char_type *fname, int *ec) {
    file_handle fd = ::CreateFile(
         fname
        ,GENERIC_READ
        ,FILE_SHARE_READ
        ,nullptr
        ,OPEN_EXISTING
        ,FILE_ATTRIBUTE_NORMAL
        ,nullptr
    );
    if ( fd == INVALID_HANDLE_VALUE ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return fd;
    }

    return fd;
}

file_handle file_create(const char_type *fname, int *ec) {
    file_handle fd = ::CreateFile(
         fname
        ,GENERIC_READ|GENERIC_WRITE
        ,FILE_SHARE_WRITE
        ,nullptr
        ,CREATE_ALWAYS
        ,FILE_ATTRIBUTE_NORMAL
        ,nullptr
    );
    if ( fd == INVALID_HANDLE_VALUE ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return fd;
    }

    return fd;
}

std::size_t file_read(file_handle fd, void *ptr, std::size_t size, int *ec) {
    if ( !::ReadFile(fd, ptr, static_cast<DWORD>(size), nullptr, nullptr) ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return 0;
    }

    return size;
}

std::size_t file_write(file_handle fd, const void *ptr, std::size_t size, int *ec) {
    if ( !::WriteFile(fd, ptr, static_cast<DWORD>(size), nullptr, nullptr) ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return 0;
    }

    return size;
}

std::size_t file_write(
     file_handle fd
    ,const io_vector *iovector
    ,std::size_t num
    ,std::size_t total_bytes
    ,int *ec)
{
    for ( const auto *it = iovector, *end = iovector + num; it != end; ++it ) {
        int lec{};
        file_write(fd, __FJ_IO_VECTOR_PTR(it), __FJ_IO_VECTOR_SIZE(it), &lec);
        if ( lec ) {
            if ( ec ) { *ec = lec; }

            return 0;
        }
    }

    return total_bytes;
}

bool file_close(file_handle fd, int *ec) {
    if ( !::CloseHandle(fd) ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return false;
    }

    return true;
}

const void* mmap_for_read(file_handle fd, std::size_t /*size*/, int *ec) {
    auto map = ::CreateFileMapping(fd, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if ( !map ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return nullptr;
    }

    void *addr = ::MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    if ( !addr ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        ::UnmapViewOfFile(map);

        return nullptr;
    }

    return addr;
}

const void* mmap_for_read(file_handle *fd, const char_type *fname, int *ec) {
    auto fdd = file_open(fname, ec);
    if ( fdd == INVALID_HANDLE_VALUE ) {
        return nullptr;
    }
    *fd = fdd;

    auto *addr = mmap_for_read(fdd, 0, ec);
    if ( !addr ) {
        return nullptr;
    }

    return addr;
}

void* mmap_for_write(file_handle fd, std::size_t /*size*/, int *ec) {
    auto map = ::CreateFileMapping(fd, nullptr, PAGE_READWRITE, 0, 0, nullptr);
    if ( !map ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return nullptr;
    }

    void *addr = ::MapViewOfFile(map, FILE_MAP_WRITE, 0, 0, 0);
    if ( !addr ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        ::UnmapViewOfFile(map);

        return nullptr;
    }

    return addr;
}

void* mmap_for_write(file_handle *fd, const char_type *fname, std::size_t size, int *ec) {
    int lec{};
    auto lfd = file_create(fname, &lec);
    if ( lec ) {
        if ( ec ) { *ec = lec; }
        return nullptr;
    }

    auto fsize = file_chsize(lfd, size, &lec);
    if ( lec ) {
        if ( ec ) { *ec = lec;  }
        file_close(lfd, &lec);
        return nullptr;
    }

    auto *addr = mmap_for_write(lfd, fsize, &lec);
    if ( !addr ) {
        if ( ec ) { *ec = lec; }
        file_close(lfd, &lec);
        return nullptr;
    }
    *fd = lfd;

    return addr;
}

bool munmap_file(const void *addr, std::size_t /*size*/, int *ec) {
    if ( !::UnmapViewOfFile(addr) ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return false;
    }

    return true;
}

bool munmap_file_fd(const void *addr, file_handle fd, int *ec) {
    int lec{};
    if ( !munmap_file(addr, std::size_t{}, &lec) ) {
        if ( ec ) { *ec = lec; }
        return false;
    }
    if ( !file_close(fd, &lec) ) {
        if ( ec ) { *ec = lec; }
        return false;
    }

    return true;
}

#else
#   error "UNKNOWN PLATFORM!"
#endif // OS detection

/*************************************************************************************************/

inline std::size_t serialize(
     file_handle fd
    ,const iterator &beg
    ,const iterator &end
    ,std::size_t indent = 0
    ,int *ec = nullptr)
{
    static const auto cb = []
    (    void *userdata
        ,const void *ptr0
        ,std::size_t size0
        ,const void *ptr1
        ,std::size_t size1
        ,const void *ptr2
        ,std::size_t size2
        ,const void *ptr3
        ,std::size_t size3
        ,std::size_t num
        ,std::size_t total_bytes
        ,int *ec)
    {
        auto *fd = static_cast<file_handle *>(userdata);
        const io_vector iovector[4] = {
             {const_cast<void *>(ptr0), size0}
            ,{const_cast<void *>(ptr1), size1}
            ,{const_cast<void *>(ptr2), size2}
            ,{const_cast<void *>(ptr3), size3}
        };
        file_write(*fd, iovector, num, total_bytes, ec);
    };

    int lec{};
    std::size_t wr{};
    if ( indent ) {
        wr = details::walk_through_tokens<false, true>(
             beg.cur
            ,end.end
            ,indent
            ,&fd
            ,cb
            ,&lec
        );
    } else {
        wr = details::walk_through_tokens<false, false>(
             beg.cur
            ,end.end
            ,indent
            ,&fd
            ,cb
            ,&lec
        );
    }
    if ( lec && ec ) {
        *ec = lec;

        return 0;
    }

    return wr;
}

inline std::size_t serialize(
     std::FILE *stream
    ,const iterator &beg
    ,const iterator &end
    ,std::size_t indent = 0
    ,int *ec = nullptr)
{
    static const auto cb = []
    (    void *userdata
        ,const void *ptr0
        ,std::size_t size0
        ,const void *ptr1
        ,std::size_t size1
        ,const void *ptr2
        ,std::size_t size2
        ,const void *ptr3
        ,std::size_t size3
        ,std::size_t num
        ,std::size_t /*total_bytes*/
        ,int *ec)
    {
        auto *stream = static_cast<std::FILE*>(userdata);
        const io_vector iovector[4] = {
             {const_cast<void *>(ptr0), size0}
            ,{const_cast<void *>(ptr1), size1}
            ,{const_cast<void *>(ptr2), size2}
            ,{const_cast<void *>(ptr3), size3}
        };
        const auto *vec = static_cast<const io_vector *>(iovector);
        for ( auto *iovend = vec + num; vec != iovend; ++vec ) {
            auto wr = std::fwrite(__FJ_IO_VECTOR_PTR(vec), 1, __FJ_IO_VECTOR_SIZE(vec), stream);
            if ( wr != __FJ_IO_VECTOR_SIZE(vec) ) {
                *ec = errno;

                break;
            }
        }
    };

    int lec{};
    std::size_t wr{};
    if ( indent ) {
        wr = details::walk_through_tokens<false, true>(
             beg.cur
            ,end.end
            ,indent
            ,stream
            ,cb
            ,&lec
        );
    } else {
        wr = details::walk_through_tokens<false, false>(
             beg.cur
            ,end.end
            ,indent
            ,stream
            ,cb
            ,&lec
        );
    }
    if ( lec && ec ) {
        *ec = lec;

        return 0;
    }

    return wr;
}

inline std::size_t serialize(
     std::ostream &stream
    ,const token *beg
    ,const token *end
    ,std::size_t indent = 0
    ,int *ec = nullptr)
{
    static const auto cb = []
    (    void *userdata
        ,const void *ptr0
        ,std::size_t size0
        ,const void *ptr1
        ,std::size_t size1
        ,const void *ptr2
        ,std::size_t size2
        ,const void *ptr3
        ,std::size_t size3
        ,std::size_t num
        ,std::size_t /*total_bytes*/
        ,int *ec)
    {
        const io_vector iovector[4] = {
             {const_cast<void *>(ptr0), size0}
            ,{const_cast<void *>(ptr1), size1}
            ,{const_cast<void *>(ptr2), size2}
            ,{const_cast<void *>(ptr3), size3}
        };
        auto *stream = static_cast<std::ostream *>(userdata);
        const auto *vec = static_cast<const io_vector *>(iovector);
        for ( auto *iovend = vec + num; vec != iovend; ++vec ) {
            auto ppos = stream->tellp();
            if ( ppos == -1 ) {
                *ec = errno;

                return;
            }

            auto wr = stream->write(
                 static_cast<const char *>(__FJ_IO_VECTOR_PTR(vec))
                ,__FJ_IO_VECTOR_SIZE(vec)
            ).tellp();
            if ( wr == -1 ) {
                *ec = errno;

                return;
            }
            if ( static_cast<std::size_t>(wr - ppos) != __FJ_IO_VECTOR_SIZE(vec) ) {
                *ec = errno;

                return;
            }
        }
    };

    int lec{};
    std::size_t wr{};
    if ( indent ) {
        wr = details::walk_through_tokens<false, true>(
             beg
            ,end
            ,indent
            ,&stream
            ,cb
            ,&lec
        );
    } else {
        wr = details::walk_through_tokens<false, false>(
             beg
            ,end
            ,indent
            ,&stream
            ,cb
            ,&lec
        );
    }
    if ( lec && ec ) {
        *ec = lec;

        return 0;
    }

    return wr;
}

inline std::size_t serialize(
     std::ostream &stream
    ,const iterator &beg
    ,const iterator &end
    ,std::size_t indent = 0
    ,int *ec = nullptr)
{
    return serialize(stream, beg.cur, end.end, indent, ec);
}

inline std::size_t serialize(
     const iterator &beg
    ,const iterator &end
    ,char *buf
    ,std::size_t bufsize
    ,std::size_t indent = 0
    ,int *ec = nullptr)
{
    struct tokens_to_buf_userdata {
        char *ptr;
        const char *end;
    };
    tokens_to_buf_userdata userdata{buf, buf + bufsize};

    static const auto cb = []
    (    void *userdata
        ,const void *ptr0
        ,std::size_t size0
        ,const void *ptr1
        ,std::size_t size1
        ,const void *ptr2
        ,std::size_t size2
        ,const void *ptr3
        ,std::size_t size3
        ,std::size_t num
        ,std::size_t /*total_bytes*/
        ,int */*ec*/)
    {
        const io_vector iovector[4] = {
             {const_cast<void *>(ptr0), size0}
            ,{const_cast<void *>(ptr1), size1}
            ,{const_cast<void *>(ptr2), size2}
            ,{const_cast<void *>(ptr3), size3}
        };

        auto *puserdata = static_cast<tokens_to_buf_userdata *>(userdata);
        const auto *vec = static_cast<const io_vector *>(iovector);
        for ( auto *iovend = vec + num; vec != iovend; ++vec ) {
            std::memcpy(puserdata->ptr, __FJ_IO_VECTOR_PTR(vec), __FJ_IO_VECTOR_SIZE(vec));
            puserdata->ptr += __FJ_IO_VECTOR_SIZE(vec);
        }
    };

    int lec{};
    std::size_t wr{};
    if ( indent ) {
        wr = details::walk_through_tokens<false, true>(
             beg.cur
            ,end.end
            ,indent
            ,&userdata
            ,cb
            ,&lec
        );
    } else {
        wr = details::walk_through_tokens<false, false>(
             beg.cur
            ,end.end
            ,indent
            ,&userdata
            ,cb
            ,&lec
        );
    }
    if ( lec && ec ) {
        *ec = lec;

        return 0;
    }

    return wr;
}

inline std::size_t length_for_string(
     const iterator &beg
    ,const iterator &end
    ,std::size_t indent = 0
    ,int *ec = nullptr)
{
    int lec{};
    std::size_t wr{};
    if ( indent ) {
        wr = details::walk_through_tokens<true, true>(
             beg.cur
            ,end.end
            ,indent
            ,nullptr
            ,nullptr
            ,&lec
        );
    } else {
        wr = details::walk_through_tokens<true, false>(
             beg.cur
            ,end.end
            ,indent
            ,nullptr
            ,nullptr
            ,&lec
        );
    }
    if ( lec && ec ) {
        *ec = lec;

        return 0;
    }

    return wr;
}

inline std::string to_string(
     const iterator &beg
    ,const iterator &end
    ,std::size_t indent = 0
    ,int *ec = nullptr)
{
    std::string res;
    auto length = length_for_string(beg, end, indent);
    res.resize(length);

    serialize(beg, end, &(res[0]), res.size(), indent, ec);
    if ( ec && *ec ) {
        return std::string{};
    }

    return res;
}

/*************************************************************************************************/

namespace details {

#define fj_bytes_required_macro(v) \
    static_cast<std::uint8_t>( \
        (v < (1u<<16)) \
            ? (v < (1u<<8)) \
                ? (v < (1u<<7)) ? 1u : 2u \
                : 3u \
            : (v < (1u<<24)) ? 4u : 5u \
    )

/*************************************************************************************************/
// iterate through all tokens

// TODO: experimental!

inline bool pack_state_iterate(
     const parser *parser
    ,void *userdata
    ,bool(*callback)(
         void *userdata
        ,std::uint8_t type
        ,std::uint32_t key_off
        ,std::uint32_t key_len
        ,std::uint32_t val_off
        ,std::uint32_t val_len
        ,std::uint32_t parent_off
        ,std::uint32_t childs
        ,std::uint32_t end_off
    ))
{
    const auto *prev = parser->toks_beg;
    const char *prev_key = nullptr;
    const char *prev_val = nullptr;
    for ( const auto *it = parser->toks_beg; it != parser->toks_cur; prev = it++ ) {
        prev_key = ((prev && prev->key) ? prev->key : prev_key);
        prev_val = ((prev && prev->val) ? prev->val : prev_val);
        auto offset_key = it->key
            ? static_cast<std::uint32_t>(
                it->key - ((prev_key && it->key) ? prev_key : parser->str_beg))
            : 0u
        ;
        auto offset_val = it->val
            ? static_cast<std::uint32_t>(
                it->val - ((prev_val && it->val) ? prev_val : parser->str_beg))
            : 0u
        ;
        auto offset_parent = it->parent ? static_cast<std::uint32_t>(it - it->parent) : 0u;
        auto offset_end = it->end ? static_cast<std::uint32_t>(it->end - it) : 0u;

        bool ok = callback(
             userdata
            ,static_cast<std::uint8_t>(it->type)
            ,offset_key
            ,it->klen
            ,offset_val
            ,it->vlen
            ,offset_parent
            ,it->childs
            ,offset_end
        );
        if ( !ok ) {
            return false;
        }
    }

    return true;
}

inline std::size_t packed_state_header_size(const parser *parser) {
    std::size_t header_size =
          sizeof(std::uint32_t) // json string length field
        + (parser->str_cur - parser->str_beg) // json string
        + sizeof(std::uint32_t) // number of tokens
    ;

    return header_size;
}

struct pack_state_userdata {
    char *ptr;
    const std::size_t expected;
    std::size_t size;
};

} // ns details

/*************************************************************************************************/
// pack/unpack the internal representation for pass to another node/process

inline std::size_t packed_state_size(const parser *parser) {
    std::size_t header_size = details::packed_state_header_size(parser);

    static const auto cb = [](
         void *userdata
        ,std::uint8_t type
        ,std::uint32_t key_off
        ,std::uint32_t key_len
        ,std::uint32_t val_off
        ,std::uint32_t val_len
        ,std::uint32_t parent_off
        ,std::uint32_t childs
        ,std::uint32_t end_off)
    {
        auto &udcnt = *static_cast<std::uint32_t *>(userdata);
        auto bytes_type          = fj_bytes_required_macro(type);
        auto bytes_key_offset    = fj_bytes_required_macro(key_off);
        auto bytes_key_len       = fj_bytes_required_macro(key_len);
        auto bytes_val_offset    = fj_bytes_required_macro(val_off);
        auto bytes_val_len       = fj_bytes_required_macro(val_len);
        auto bytes_parent_offset = fj_bytes_required_macro(parent_off);
        auto bytes_childs_num    = fj_bytes_required_macro(childs);
        auto bytes_end_offset    = fj_bytes_required_macro(end_off);
        std::uint32_t per_token =
              bytes_type
            + bytes_key_offset
            + bytes_key_len
            + bytes_val_offset
            + bytes_val_len
            + bytes_parent_offset
            + bytes_childs_num
            + bytes_end_offset
        ;

        udcnt += per_token;

        return true;
    };

    std::uint32_t markup_size = 0;
    bool ok = details::pack_state_iterate(parser, static_cast<void *>(&markup_size), cb);

    return ok ? markup_size + header_size : 0;
}

/*************************************************************************************************/

inline std::size_t pack_state(char *dst, std::size_t size, const parser *parser) {
    static const auto cb = [](
         void *userdata
        ,std::uint8_t type
        ,std::uint32_t key_off
        ,std::uint32_t key_len
        ,std::uint32_t val_off
        ,std::uint32_t val_len
        ,std::uint32_t parent_off
        ,std::uint32_t childs
        ,std::uint32_t end_off)
    {
        static const auto write = [](void *dst, const void *ptr, std::size_t bytes) {
            auto *dstu = static_cast<std::uint8_t *>(dst);
            if ( bytes == 1 ) {
                *dstu = *static_cast<const std::uint8_t *>(ptr);
                *dstu |= (1u<<7);
            } else {
                *dstu = static_cast<std::uint8_t>(bytes);
                std::memcpy(dstu + 1, ptr, bytes - 1);
            }

            return static_cast<char *>(dst) + bytes;
        };

        auto *ud = static_cast<details::pack_state_userdata *>(userdata);
        auto bytes_type          = fj_bytes_required_macro(type);
        auto bytes_key_offset    = fj_bytes_required_macro(key_off);
        auto bytes_key_len       = fj_bytes_required_macro(key_len);
        auto bytes_val_offset    = fj_bytes_required_macro(val_off);
        auto bytes_val_len       = fj_bytes_required_macro(val_len);
        auto bytes_parent_offset = fj_bytes_required_macro(parent_off);
        auto bytes_childs_num    = fj_bytes_required_macro(childs);
        auto bytes_end_offset    = fj_bytes_required_macro(end_off);
        std::size_t per_token =
              bytes_type
            + bytes_key_offset
            + bytes_key_len
            + bytes_val_offset
            + bytes_val_len
            + bytes_parent_offset
            + bytes_childs_num
            + bytes_end_offset
        ;

        if ( ud->size + per_token > ud->expected ) {
            return false;
        }

        ud->ptr = write(ud->ptr, &type, bytes_type);
        ud->ptr = write(ud->ptr, &key_off, bytes_key_offset);
        ud->ptr = write(ud->ptr, &key_len, bytes_key_len);
        ud->ptr = write(ud->ptr, &val_off, bytes_val_offset);
        ud->ptr = write(ud->ptr, &val_len, bytes_val_len);
        ud->ptr = write(ud->ptr, &parent_off, bytes_parent_offset);
        ud->ptr = write(ud->ptr, &childs, bytes_childs_num);
        ud->ptr = write(ud->ptr, &end_off, bytes_end_offset);

        ud->size += per_token;

        return true;
    };

    char *ptr = dst;
    std::uint32_t json_str_len = static_cast<std::uint32_t>(parser->str_cur - parser->str_beg);
    std::memcpy(ptr, &json_str_len, sizeof(json_str_len));
    ptr += sizeof(json_str_len);
    std::memcpy(ptr, parser->str_beg, json_str_len);
    ptr += json_str_len;
    std::uint32_t toks_num = static_cast<std::uint32_t>(parser->toks_cur - parser->toks_beg);
    std::memcpy(ptr, &toks_num, sizeof(toks_num));
    ptr += sizeof(toks_num);

    std::size_t left_bytes = size - details::packed_state_header_size(parser);
    details::pack_state_userdata ud{ptr, left_bytes, 0};
    bool ok = details::pack_state_iterate(parser, static_cast<void *>(&ud), cb);

    return ok ? details::packed_state_header_size(parser) + ud.size : 0;
}

/*************************************************************************************************/

inline bool unpack_state(parser *parser, char *ptr, std::size_t size) {
    const char *end = ptr + size;
    std::uint32_t json_len{};
    std::memcpy(&json_len, ptr, sizeof(json_len));
    ptr += sizeof(json_len);

    parser->str_beg = ptr;
    parser->str_cur = parser->str_end = parser->str_beg + json_len;
    ptr += json_len;

    std::uint32_t num_toks{};
    std::memcpy(&num_toks, ptr, sizeof(num_toks));
    ptr += sizeof(num_toks);

    parser->dyn_parser = false;
    parser->dyn_tokens = true;
    parser->toks_beg = static_cast<token *>(parser->alloc_fn(num_toks * sizeof(token)));
    parser->toks_cur = parser->toks_end = parser->toks_beg + num_toks;
    token *prev = nullptr;
    const char *prev_key = nullptr;
    const char *prev_val = nullptr;
    for ( auto *it = parser->toks_beg; it != parser->toks_end; prev = it++ ) {
        static const auto reader = [](std::uint32_t *dst, char *ptr, const char *end) -> char * {
            std::uint8_t v = *reinterpret_cast<std::uint8_t *>(ptr);
            const bool onebyte = static_cast<bool>((v >> 7) & 1u);
            v &= ~(1u << 7);
            if ( onebyte ) {
                *dst = v;

                return ptr + 1;
            }

            if ( ptr + v > end ) {
                return nullptr;
            }

            std::uint32_t res{};
            std::memcpy(&res, ptr, v);

            *dst = res;

            return ptr + v;
        };

        std::uint32_t type{};
        ptr = reader(&type, ptr, end);
        if ( !ptr ) { return false; }
        std::uint32_t key_off{};
        ptr = reader(&key_off, ptr, end);
        if ( !ptr ) { return false; }
        std::uint32_t key_len{};
        ptr = reader(&key_len, ptr, end);
        if ( !ptr ) { return false; }
        std::uint32_t val_off{};
        ptr = reader(&val_off, ptr, end);
        if ( !ptr ) { return false; }
        std::uint32_t val_len{};
        ptr = reader(&val_len, ptr, end);
        if ( !ptr ) { return false; }
        std::uint32_t parent_off{};
        ptr = reader(&parent_off, ptr, end);
        if ( !ptr ) { return false; }
        std::uint32_t childs{};
        ptr = reader(&childs, ptr, end);
        if ( !ptr ) { return false; }
        std::uint32_t end_off{};
        ptr = reader(&end_off, ptr, end);
        if ( !ptr ) { return false; }

        prev_key = ((prev && prev->key) ? prev->key : prev_key);
        prev_val = ((prev && prev->val) ? prev->val : prev_val);
        it->type   = static_cast<token_type>(type);
        it->key    = (key_off ? (prev_key ? prev_key + key_off : parser->str_beg + key_off): nullptr);
        it->klen   = static_cast<decltype(it->klen)>(key_len);
        it->val    = (val_off ? (prev_val ? prev_val + val_off : parser->str_beg + val_off): nullptr);
        it->vlen   = static_cast<decltype(it->vlen)>(val_len);
        it->parent = (parent_off ? it - parent_off : nullptr);
        it->childs = static_cast<decltype(it->childs)>(childs);
        it->end    = (end_off ? it + end_off : nullptr);
    }
    parser->error = FJ_EC_OK;

    return true;
}

/*************************************************************************************************/

} // ns flatjson

#endif // __FLATJSON__IO_HPP
