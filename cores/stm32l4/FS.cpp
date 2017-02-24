/*
 FS.cpp - file system wrapper
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

#include "FS.h"
#include "dosfs_config.h"

File::File(const char* path, const char* mode) {
    _file = f_open(path, mode);
}

File::File() {
    _file = NULL;
}

size_t File::write(uint8_t c) {
    if (!_file)
        return 0;
    
    if (f_putc(c, _file) == -1) {
	return 0;
    }

    return 1;
}

size_t File::write(const uint8_t *buf, size_t size) {
    if (!_file)
        return 0;

    return f_write(buf, 1, size, _file);
}

int File::available() {
    if (!_file)
        return 0;

    return f_length(_file) - f_tell(_file);
}

int File::read() {
    if (!_file)
        return -1;

    return f_getc(_file);
}

size_t File::read(uint8_t* buf, size_t size) {
    if (!_file)
        return -1;

    return f_read(buf, 1, size, _file);
}

int File::peek() {
    long position;
    int c;

    if (!_file)
        return -1;

    position = f_tell(_file);
    c = f_getc(_file);
    f_seek(_file, position, F_SEEK_SET);
    return c;
}

void File::flush() {
    if (!_file)
        return;

    f_flush(_file);
}

bool File::seek(uint32_t pos, SeekMode mode) {
    if (!_file)
        return false;

    return (f_seek(_file, pos, mode) == F_NO_ERROR);
}

size_t File::position() const {
    if (!_file)
        return 0;

    return f_tell(_file);
}

size_t File::size() const {
    if (!_file)
        return 0;

    return f_length(_file);
}

void File::close() {
    if (!_file)
	return;

    f_close(_file);

    _file = NULL;
}

File::operator bool() const {
    return !!_file;
}


Dir::Dir(const char* path) {
    char filename[F_MAXPATH];

    strcpy(_path, path);

    if (path[strlen(path)-1] != '/')  
	strcat(_path, "/");

    strcpy(filename, _path);
    strcat(filename, "*.*");

    if (f_findfirst(filename, &_find) != F_NO_ERROR) {
	_find.find_clsno = 0x0fffffff;
    }
}

Dir::Dir() {
    _path[0] = '\0';
    _find.find_clsno = 0x0fffffff;
};

File Dir::openFile(const char* mode) {
    char filename[F_MAXPATH];

    if (_find.find_clsno == 0x0fffffff)
        return File();

    strcpy(filename, _path);
    strcat(filename, _find.filename);

    return File(filename, mode);
}

String Dir::fileName() {
    if (_find.find_clsno == 0x0fffffff)
        return String();

    return String(&_find.filename[0]);
}

size_t Dir::fileSize() {
    if (_find.find_clsno == 0x0fffffff)
        return 0;

    return _find.filesize;
}

bool Dir::next() {
    if (_find.find_clsno == 0x0fffffff)
        return false;

    return (f_findnext(&_find) == F_NO_ERROR);
}

FS::FS() {
}

bool FS::begin()
{
    return (f_initvolume() == F_NO_ERROR);
}

void FS::end()
{
    f_delvolume();
}

bool FS::check()
{
    return (f_checkvolume() == F_NO_ERROR);
}

bool FS::format()
{
    return (f_hardformat(0) == F_NO_ERROR);
}

bool FS::info(FSInfo& info)
{
    F_SPACE space;
    
    if (f_getfreespace(&space) != F_NO_ERROR) 
	return false;
    
    info.totalBytes    = (uint64_t)space.total | ((uint64_t)space.total_high << 32);
    info.usedBytes     = (uint64_t)space.used  | ((uint64_t)space.used_high << 32);
    info.blockSize     = 512;
    info.pageSize      = 0;
    info.maxOpenFiles  = DOSFS_CONFIG_MAX_FILES;
    info.maxPathLength = F_MAXPATH;
    
    return true;
}

File FS::open(const char* path, const char* mode) {
    return File(path, mode);
}

File FS::open(const String& path, const char* mode) {
    return open(path.c_str(), mode);
}

bool FS::exists(const char* path) {
    unsigned char attr;

    return (f_getattr(path, &attr) == F_NO_ERROR);
}

bool FS::exists(const String& path) {
    return exists(path.c_str());
}

Dir FS::openDir(const char* path) {
    return Dir(path);
}

Dir FS::openDir(const String& path) {
    return openDir(path.c_str());
}

bool FS::mkdir(const char* path) {
    return (f_mkdir(path) == F_NO_ERROR);
}

bool FS::mkdir(const String& path) {
    return mkdir(path.c_str());
}

bool FS::rmdir(const char* path) {
    return (f_rmdir(path) == F_NO_ERROR);
}

bool FS::rmdir(const String& path) {
    return rmdir(path.c_str());
}

bool FS::chdir(const char* path) {
    return (f_chdir(path) == F_NO_ERROR);
}

bool FS::chdir(const String& path) {
    return chdir(path.c_str());
}

bool FS::rename(const char* pathFrom, const char* pathTo) {
    return (f_move(pathFrom, pathTo) == F_NO_ERROR);
}

bool FS::rename(const String& pathFrom, const String& pathTo) {
    return rename(pathFrom.c_str(), pathTo.c_str());
}

bool FS::remove(const char* path) {
    return (f_delete(path) == F_NO_ERROR);
}

bool FS::remove(const String& path) {
    return remove(path.c_str());
}

FS DOSFS;
