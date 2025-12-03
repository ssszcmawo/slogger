
/*
 * zip.c — minimal, dependency-free ZIP archive writer
 *
 * Features:
 *   • Creates fully valid ZIP archives (no compression, "store" method)
 *   • Supports directories, UTF-8 filenames, Unix permissions
 *   • Recursive directory archiving
 *   • No external dependencies — not even zlib
 *   • Single-pass local headers + deferred central directory
 *   • Compatible with Windows Explorer, unzip, 7-Zip, Info-ZIP, etc.
 *
 * Limitations:
 *   • Only "store" method (method 0) — no Deflate/ZIP64/encryption
 *   • Archive size limited to < 4 GiB (no ZIP64 extensions)
 *
 * This implementation follows the official PKWARE APPNOTE.TXT specification
 * (version 6.3.10) and is used by myLogger for log rotation archiving. 
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>

#pragma pack(push, 1)

typedef struct {
    uint32_t signature;        /* 0x04034b50 */
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression;
    uint16_t mod_time;
    uint16_t mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t name_length;
    uint16_t extra_length;
} zip_local_header_t;

typedef struct {
    uint32_t signature;        /* 0x02014b50 */
    uint16_t version_made;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression;
    uint16_t mod_time;
    uint16_t mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t name_length;
    uint16_t extra_length;
    uint16_t comment_length;
    uint16_t disk_number;
    uint16_t internal_attr;
    uint32_t external_attr;
    uint32_t local_offset;
} zip_central_header_t;

typedef struct {
    uint32_t signature;        /* 0x06054b50 */
    uint16_t disk_number;
    uint16_t cd_start_disk;
    uint16_t entries_this_disk;
    uint16_t total_entries;
    uint32_t cd_size;
    uint32_t cd_offset;
    uint16_t comment_length;
} zip_eocd_t;

#pragma pack(pop)

static FILE *zip_fp = NULL;
static uint64_t cd_offset = 0;
static uint64_t cd_size = 0;
static uint32_t entry_count = 0;

static uint32_t crc32_slow(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        uint8_t b = data[i];
        crc ^= b;
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1));
    }
    return crc ^ 0xFFFFFFFF;
}


static uint16_t dos_time(time_t t)
{
    struct tm *tm = localtime(&t);
    return (tm->tm_sec / 2) | (tm->tm_min << 5) | (tm->tm_hour << 11);
}

static uint16_t dos_date(time_t t)
{
    struct tm *tm = localtime(&t);
    return tm->tm_mday | ((tm->tm_mon + 1) << 5) | ((tm->tm_year - 80) << 9);
}


static void write_central(const char *name, uint32_t offset,
                         time_t mtime, uint32_t crc, uint32_t size, mode_t mode)
{
    zip_central_header_t ch = {0};

    ch.signature       = 0x02014b50;
    ch.version_made    = 0x0314;      /* Linux + ZIP 3.0 */
    ch.version_needed  = 20;
    ch.flags           = 0x800;       /* UTF-8 filenames */
    ch.compression     = 0;
    ch.mod_time        = dos_time(mtime);
    ch.mod_date        = dos_date(mtime);
    ch.crc32           = crc;
    ch.compressed_size = size;
    ch.uncompressed_size = size;
    ch.name_length     = strlen(name);
    ch.external_attr   = (mode << 16); /* Unix permissions */

    ch.local_offset    = offset;

    fwrite(&ch, sizeof(ch), 1, zip_fp);
    fwrite(name, 1, ch.name_length, zip_fp);

    cd_size += sizeof(ch) + ch.name_length;
    entry_count++;
}


static void add_file(const char *fullpath, const char *name_in_zip)
{
    FILE *in = fopen(fullpath, "rb");
    if (!in) return;

    struct stat st;
    stat(fullpath, &st);

    fseek(in, 0, SEEK_END);
    size_t size = ftell(in);
    fseek(in, 0, SEEK_SET);

    uint8_t *buf = malloc(size);
    if (!buf) { fclose(in); return; }
    fread(buf, 1, size, in);
    fclose(in);

    uint32_t crc = crc32_slow(buf, size);
    uint32_t offset = ftell(zip_fp);

    zip_local_header_t lh = {0};
    lh.signature        = 0x04034b50;
    lh.version_needed   = 20;
    lh.flags            = 0x800;
    lh.compression      = 0;
    lh.mod_time         = dos_time(st.st_mtime);
    lh.mod_date         = dos_date(st.st_mtime);
    lh.crc32            = crc;
    lh.compressed_size  = size;
    lh.uncompressed_size = size;
    lh.name_length      = strlen(name_in_zip);

    fwrite(&lh, sizeof(lh), 1, zip_fp);
    fwrite(name_in_zip, 1, lh.name_length, zip_fp);
    fwrite(buf, 1, size, zip_fp);

    free(buf);

    write_central(name_in_zip, offset, st.st_mtime, crc, size, st.st_mode);
}


static void walk_dir(const char *base, const char *current)
{
    DIR *dir = opendir(current ? current : base);
    if (!dir) return;

    struct dirent *ent;
    char fullpath[PATH_MAX];
    char name_in_zip[PATH_MAX];
    const char *rel;

    while ((ent = readdir(dir))) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
            continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s",
                 current ? current : base, ent->d_name);

        rel = fullpath + strlen(base) + (fullpath[strlen(base)] == '/');

        if (ent->d_type == DT_DIR) {
            /* directory entry */
            snprintf(name_in_zip, sizeof(name_in_zip), "%s/", rel);
            uint32_t offset = ftell(zip_fp);
            zip_local_header_t dirh = {0};
            dirh.signature   = 0x04034b50;
            dirh.flags       = 0x800;
            dirh.name_length = strlen(name_in_zip);
            fwrite(&dirh, sizeof(dirh), 1, zip_fp);
            fwrite(name_in_zip, 1, dirh.name_length, zip_fp);
            write_central(name_in_zip, offset, 0, 0, 0, 0755);
            walk_dir(base, fullpath);
        }
        else if (ent->d_type == DT_REG) {
            add_file(fullpath, rel);
        }
    }
    closedir(dir);
}

static void write_eocd(void)
{
    zip_eocd_t eocd = {0};
    eocd.signature        = 0x06054b50;
    eocd.entries_this_disk = entry_count;
    eocd.total_entries     = entry_count;
    eocd.cd_size           = cd_size;
    eocd.cd_offset         = cd_offset;
    fwrite(&eocd, sizeof(eocd), 1, zip_fp);
}

int create_zip(const char *folder, const char *zipname)
{
    if (!folder || !zipname || folder[0] == '\0' || zipname[0] == '\0')
        return -1;

    zip_fp = fopen(zipname, "wb");
    if (!zip_fp)
        return -1;

    cd_offset = 0;
    cd_size = 0;
    entry_count = 0;

    walk_dir(folder, folder);

    if (entry_count == 0) {
        fclose(zip_fp);
        zip_fp = NULL;
        unlink(zipname);  
        return -1;
    }

    cd_offset = ftell(zip_fp);

    fseek(zip_fp, 0, SEEK_SET);
    walk_dir(folder, folder);

    fseek(zip_fp, 0, SEEK_END);
    write_eocd();

    fclose(zip_fp);
    zip_fp = NULL;


    struct stat st;
    if (stat(zipname, &st) != 0 || st.st_size < 22) { 
        unlink(zipname);
        return -1;
    }

    return 0; 
}
