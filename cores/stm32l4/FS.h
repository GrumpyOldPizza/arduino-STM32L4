/*
 FS.h - file system wrapper
 Copyright (c) 2015 Ivan Grokhotkov. All rights reserved.
 Copyright (c) 2016 Thomas Roell. All rights reserved.
 This file is part of the STM32L4 core for Arduino environment.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef FS_H
#define FS_H

#include <Arduino.h>
#include <dosfs_api.h>

class File;
class Dir;

enum SeekMode {
    SeekSet = 0,
    SeekCur = 1,
    SeekEnd = 2
};

class File : public Stream
{
public:
    File(const char *path, const char *mode);
    File();

    // Print methods:
    size_t write(uint8_t) override;
    size_t write(const uint8_t *buf, size_t size) override;

    // Stream methods:
    int available() override;
    int read() override;
    int peek() override;
    void flush() override;
    size_t readBytes(char *buffer, size_t length)  override {
        return read((uint8_t*)buffer, length);
    }
    size_t read(uint8_t* buf, size_t size);
    bool seek(uint32_t pos, SeekMode mode = SeekSet);
    size_t position() const;
    size_t size() const;
    void close();
    operator bool() const;

protected:
    F_FILE *_file;
};

class Dir {
public:
    Dir(const char *path);
    Dir();

    File openFile(const char* mode);
    String fileName();
    size_t fileSize();
    bool next();

protected:
    char   _path[F_MAXPATH];
    F_FIND _find;
};

struct FSInfo {
    size_t totalBytes;
    size_t usedBytes;
    size_t blockSize;
    size_t pageSize;
    size_t maxOpenFiles;
    size_t maxPathLength;
};

class FS
{
public:
    FS();

    bool begin();
    void end();

    bool check();
    bool format();
    bool info(FSInfo& info);

    File open(const char* path, const char* mode);
    File open(const String& path, const char* mode);

    bool exists(const char* path);
    bool exists(const String& path);

    Dir openDir(const char* path);
    Dir openDir(const String& path);

    bool mkdir(const char* path);
    bool mkdir(const String& path);

    bool rmdir(const char* path);
    bool rmdir(const String& path);

    bool chdir(const char* path);
    bool chdir(const String& path);

    bool remove(const char* path);
    bool remove(const String& path);

    bool rename(const char* pathFrom, const char* pathTo);
    bool rename(const String& pathFrom, const String& pathTo);

};

extern FS DOSFS;

#endif //FS_H
