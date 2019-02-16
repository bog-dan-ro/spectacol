/* compat.cpp: various Qt compatibility functions

    Copyright (c) 2015, BogDan Vatra <bogdan@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QDirIterator>
#include <QStandardPaths>

#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <memory>
#include <thread>

extern "C"  {
# include <compat.h>
# include <utils.h>
# include <ui/ui.h>
}

extern "C" const compat_fd COMPAT_FILE_OPEN_FAILED = nullptr;

extern "C" compat_fd compat_file_open(const char *path, int write)
{
    std::unique_ptr<QFile> file(new QFile(path));
    if (file->open(write ? QIODevice::WriteOnly : QIODevice::ReadOnly))
        return (compat_fd)file.release();
    return nullptr;
}

extern "C" off_t compat_file_get_length(compat_fd fd)
{
    return reinterpret_cast<QFile*>(fd)->size();
}

extern "C" int compat_file_read(compat_fd fd, utils_file *file)
{
    return size_t(reinterpret_cast<QFile*>(fd)->read((char*)file->buffer, file->length)) == file->length ? 0 : 1;
}


extern "C" int compat_file_write(compat_fd fd, const unsigned char *buffer, size_t length)
{
    return size_t(reinterpret_cast<QFile*>(fd)->write((const char*)buffer, length)) == length ? 0 : 1;
}

extern "C" int compat_file_close(compat_fd fd)
{
    delete reinterpret_cast<QFile*>(fd);
    return 0;
}

extern "C" int compat_file_exists(const char *path)
{
    return QFile(path).exists() ? 1 : 0;
}

extern "C" const char *compat_get_temp_path()
{
    static const QByteArray path = QDir::tempPath().toUtf8();
    return path.constData();
}

extern "C" const char *compat_get_home_path()
{
    static const QByteArray path = QDir::homePath().toUtf8();
    return path.constData();
}

extern "C" const char *compat_get_config_path()
{
    static const QByteArray path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation).toUtf8();
    return path.constData();
}

extern "C" int compat_is_absolute_path(const char *path)
{
    return QFileInfo(path).isAbsolute() ? 1 : 0;
}

static QByteArray contextType(int type) {
    switch (type) {
    case UTILS_AUXILIARY_LIB:
        return "lib";
    case UTILS_AUXILIARY_ROM:
        return "rom";
    case UTILS_AUXILIARY_WIDGET:
        return "ui";
    }
    return QByteArray();
}

extern "C" int compat_get_next_path(path_context *ctx)
{
    switch (ctx->state++) {
    case 0:
        strncpy( ctx->path, ".", PATH_MAX );
        return 1;

    case 1: {
        QByteArray type(QStandardPaths::writableLocation(QStandardPaths::DataLocation).toUtf8() + QDir::separator().toLatin1() + contextType(ctx->type));
        strncpy( ctx->path, type.constData(), PATH_MAX );
        return 1;
    }

    case 2: {
        QByteArray type(":/" + contextType(ctx->type));
        strncpy( ctx->path, type.constData(), PATH_MAX );
        return 1;
    }
    }
    return 0;
}


extern "C" compat_dir compat_opendir(const char *path)
{
    QDirIterator *it = new QDirIterator(path, QDir::AllEntries);
    return compat_dir(it);
}


extern "C" compat_dir_result_t compat_readdir(compat_dir directory, char *name, size_t length)
{
    auto dir = reinterpret_cast<QDirIterator*>(directory);
    if (!dir->hasNext())
        return COMPAT_DIR_RESULT_END;
    dir->next();

    strncpy(name, dir->fileName().toUtf8().constData(), length);
    return COMPAT_DIR_RESULT_OK;
}


extern "C" int compat_closedir(compat_dir directory)
{
    delete reinterpret_cast<QDirIterator*>(directory);
    return 0;
}


extern "C" double compat_timer_get_time( void )
{
    timeval tv;
    int error = gettimeofday( &tv, NULL );
    if( error ) {
        ui_error( UI_ERROR_ERROR, "%s: error getting time: %s", __func__, strerror( errno ) );
        return -1;
    }

    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

extern "C" void compat_timer_sleep( int ms )
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
