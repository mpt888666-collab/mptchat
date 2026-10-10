//
// Created by mpt on 2026/8/24.
//

#ifndef RESOURCESERVER_FILESYSTEM_H
#define RESOURCESERVER_FILESYSTEM_H

#include "const.h"
#include "FileWorker.h"
#include "Singleton.h"
#include <memory>
#include <vector>


class FileSystem :public Singleton<FileSystem>
{
    friend class Singleton<FileSystem>;
public:
    ~FileSystem();
    void PostMsgToQue(std::shared_ptr<FileTask> msg, int index);
private:
    FileSystem();
    std::vector<std::shared_ptr<FileWorker>>  _file_workers;
};

#endif //RESOURCESERVER_FILESYSTEM_H