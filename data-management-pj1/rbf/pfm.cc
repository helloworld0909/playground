#include "pfm.h"
#include <fstream>
#include <iostream>
#include <cstdio>
#include <sys/stat.h>

using namespace std;

PagedFileManager *PagedFileManager::_pf_manager = 0;

PagedFileManager *PagedFileManager::instance()
{
    if (!_pf_manager)
        _pf_manager = new PagedFileManager();

    return _pf_manager;
}

PagedFileManager::PagedFileManager()
{
}

PagedFileManager::~PagedFileManager()
{
}

bool isFileExists(const string &fileName)
{
    struct stat stFileInfo;
    return stat(fileName.c_str(), &stFileInfo) == 0;
}

RC PagedFileManager::createFile(const string &fileName)
{
    if (isFileExists(fileName))
    {
        return FAIL;
    }

    ofstream file(DATA_PATH + fileName, ios::binary);
    byte header[PAGE_SIZE] = {IDENTIFIER};
    file.write(header, PAGE_SIZE);
    file.close();
    return SUCCESS;
}

RC PagedFileManager::destroyFile(const string &fileName)
{
    int rc = remove(fileName.c_str());
    if (rc == 0)
    {
        return SUCCESS;
    }
    else
    {
        return FAIL;
    }
}

RC PagedFileManager::openFile(const string &fileName, FileHandle &fileHandle)
{
    if (fileHandle.file.is_open() || !isFileExists(fileName))
    {
        return FAIL;
    }

    fileHandle.file.open(fileName, ios::in | ios::out | ios::binary);
    if (!fileHandle.file)
    {
        return FAIL;
    }

    byte header[PAGE_SIZE];
    fileHandle.file.read(header, PAGE_SIZE);

    if (header[0] != IDENTIFIER)
    {
        return FAIL;
    }

    fileHandle.numOfPages = header[1] | header[2] << 8;
    fileHandle.readPageCounter = header[3] | header[4] << 8;
    fileHandle.writePageCounter = header[5] | header[6] << 8;
    fileHandle.appendPageCounter = header[7] | header[8] << 8;

    return SUCCESS;
}

RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
    if (!fileHandle.file.is_open())
    {
        return FAIL;
    }
    fileHandle.file.seekp(sizeof(IDENTIFIER), ios::beg);

    fileHandle.file.write((char *)&fileHandle.numOfPages, sizeof(unsigned));
    fileHandle.file.write((char *)&fileHandle.readPageCounter, sizeof(unsigned));
    fileHandle.file.write((char *)&fileHandle.writePageCounter, sizeof(unsigned));
    fileHandle.file.write((char *)&fileHandle.appendPageCounter, sizeof(unsigned));

    fileHandle.file.close();
    return SUCCESS;
}

FileHandle::FileHandle()
{
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
}

FileHandle::~FileHandle()
{
}

RC FileHandle::readPage(PageNum pageNum, void *data)
{
    return -1;
}

RC FileHandle::writePage(PageNum pageNum, const void *data)
{
    return -1;
}

RC FileHandle::appendPage(const void *data)
{
    return -1;
}

unsigned FileHandle::getNumberOfPages()
{
    return numOfPages;
}

RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
    return -1;
}
