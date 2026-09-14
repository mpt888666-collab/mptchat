//
// Created by mpt on 2026/8/24.
//

#include "FileSystem.h"

#include "FileWorker.h"
struct FileTask;

FileSystem::~FileSystem()
{

}

void FileSystem::PostMsgToQue(std::shared_ptr<FileTask> msg, int index)
{
    _file_workers[index]->PostTask(msg);
}

FileSystem::FileSystem()
{
    for (int i = 0; i < FILE_WORKER_COUNT; i++) {
        _file_workers.push_back(std::make_shared<FileWorker>());
    }
}