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
    return fileHandle.openFile(fileName);
}

RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
    return fileHandle.closeFile();
}

FileHandle::FileHandle()
{
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
}

FileHandle::~FileHandle()
{
    closeFile();
}

RC FileHandle::openFile(const string &fileName)
{
    if (file.is_open() || !isFileExists(fileName))
    {
        return FAIL;
    }

    file.open(fileName, ios::in | ios::out | ios::binary);
    if (!file.is_open())
    {
        return FAIL;
    }

    byte header[PAGE_SIZE];
    file.read(header, PAGE_SIZE);

    if (header[0] != IDENTIFIER)
    {
        return FAIL;
    }
    numOfPages = (unsigned)*(header + sizeof(IDENTIFIER));
    readPageCounter = (unsigned)*(header + sizeof(IDENTIFIER) + 1 * sizeof(unsigned));
    writePageCounter = (unsigned)*(header + sizeof(IDENTIFIER) + 2 * sizeof(unsigned));
    appendPageCounter = (unsigned)*(header + sizeof(IDENTIFIER) + 3 * sizeof(unsigned));

    return SUCCESS;
}

RC FileHandle::closeFile()
{
    if (!file.is_open())
    {
        return FAIL;
    }
    file.seekp(sizeof(IDENTIFIER), ios::beg);

    file.write((char *)&numOfPages, sizeof(unsigned));
    file.write((char *)&readPageCounter, sizeof(unsigned));
    file.write((char *)&writePageCounter, sizeof(unsigned));
    file.write((char *)&appendPageCounter, sizeof(unsigned));

    file.close();
    return SUCCESS;
}

RC FileHandle::readPage(PageNum pageNum, void *data)
{
    if (!file.is_open() || pageNum >= getNumberOfPages())
    {
        return FAIL;
    }
    file.seekg((pageNum + 1) * PAGE_SIZE, ios::beg);
    file.read((char *)data, PAGE_SIZE);
    readPageCounter++;
    return SUCCESS;
}

RC FileHandle::writePage(PageNum pageNum, const void *data)
{
    if (!file.is_open() || pageNum >= getNumberOfPages())
    {
        return FAIL;
    }
    file.seekp((pageNum + 1) * PAGE_SIZE, ios::beg);
    file.write((const char *)data, PAGE_SIZE);
    writePageCounter++;
    return SUCCESS;
}

RC FileHandle::appendPage(const void *data)
{
    if (!file.is_open())
    {
        return FAIL;
    }
    file.seekp(0, ios::end);
    file.write((const char *)data, PAGE_SIZE);
    numOfPages++;
    appendPageCounter++;
    return SUCCESS;
}

unsigned FileHandle::getNumberOfPages()
{
    return numOfPages;
}

RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
    readPageCount = readPageCounter;
    writePageCount = writePageCounter;
    appendPageCount = appendPageCounter;
    return SUCCESS;
}
