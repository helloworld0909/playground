#include "rbfm.h"
#include <math.h>
#include <iostream>
#include <cstring>

RecordBasedFileManager *RecordBasedFileManager::_rbf_manager = 0;

RecordBasedFileManager *RecordBasedFileManager::instance()
{
    if (!_rbf_manager)
        _rbf_manager = new RecordBasedFileManager();

    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager() {}

RecordBasedFileManager::~RecordBasedFileManager() {}

RC RecordBasedFileManager::createFile(const string &fileName)
{
    return PagedFileManager::instance()->createFile(fileName);
}

RC RecordBasedFileManager::destroyFile(const string &fileName)
{
    return PagedFileManager::instance()->destroyFile(fileName);
}

RC RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle)
{
    if (PagedFileManager::instance()->openFile(fileName, fileHandle) == FAIL)
    {
        return FAIL;
    }
    if (fileHandle.getNumberOfPages() == 0)
    {
        byte *directory = new byte[PAGE_SIZE];
        fileHandle.appendPage(directory);
    }
    return SUCCESS;
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle)
{
    return PagedFileManager::instance()->closeFile(fileHandle);
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid)
{
    unsigned neededSpace = computeSpace(recordDescriptor, data);
    cout << "neededSpace: " << neededSpace << endl;
    if (neededSpace > PAGE_SIZE - SIZE_NUM_RECORD)
    {
        return FAIL;
    }

    PageNum freePageNum = getFreePageNum(fileHandle, neededSpace);
    cout << "freePageNum: " << freePageNum << endl;

    byte page[PAGE_SIZE];
    fileHandle.readPage(freePageNum, page);

    //Write record offset and length
    unsigned numRecord = *((unsigned *)(page + PAGE_SIZE - SIZE_NUM_RECORD));
    byte *tail = page + PAGE_SIZE - SIZE_NUM_RECORD;
    byte *pLastRecordPos = tail - numRecord * SIZE_RECORD_POS;
    unsigned offset = *((unsigned *)pLastRecordPos) + *((unsigned *)(pLastRecordPos + SIZE_RECORD_OFFSET));
    unsigned length = neededSpace - SIZE_RECORD_POS;
    *((unsigned *)pLastRecordPos - SIZE_RECORD_POS) = offset;
    *((unsigned *)(pLastRecordPos - SIZE_RECORD_POS + SIZE_RECORD_OFFSET)) = length;

    //Write record
    byte *record = (byte *)malloc(length);
    RC rc = transformRecord(recordDescriptor, data, record);
    cout << rc << endl;
    cout << string(record, length) << endl;
    memcpy(page + offset, record, length);

    //Num of records add one
    *((unsigned *)tail) += 1;

    //Update page
    fileHandle.writePage(freePageNum, page);

    return SUCCESS;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data)
{
    return -1;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data)
{
    const int FIELD_WIDTH = 10;
    unsigned bytesOfNullIndicator = computeBytesOfNullIndicator(recordDescriptor);
    const byte *pFlag = (const byte *)data;
    const byte *pField = pFlag + bytesOfNullIndicator;
    byte mask = 0x01;

    cout.setf(std::ios::left);
    for (unsigned i = 0; i < recordDescriptor.size(); i++)
    {
        Attribute attr = recordDescriptor[i];
        cout << attr.name << ": ";

        unsigned byteOffset = i / 8;
        unsigned pos = 7 - i % 8; // First flag is at pos 8;
        byte newMask = mask << pos;
        bool isNull = (*(pFlag + byteOffset) & newMask) == newMask;

        cout.width(FIELD_WIDTH);
        if (!isNull)
        {
            switch (attr.type)
            {
            case TypeInt:
                cout << *((const uint32_t *)pField);
                pField += attr.length;
                break;
            case TypeReal:
                cout << *((const float *)pField);
                pField += attr.length;
                break;
            case TypeVarChar:
                uint32_t lenVar = *((const uint32_t *)pField);
                pField += 4;
                string value(pField, lenVar);
                cout << value;
                pField += lenVar;
                break;
            }
        }
        else
        {
            cout << "NULL";
        }
    }
    cout << endl;
    return SUCCESS;
}

unsigned RecordBasedFileManager::computeBytesOfNullIndicator(const vector<Attribute> &recordDescriptor)
{
    return (unsigned)ceil((double)recordDescriptor.size() / 8);
}

unsigned RecordBasedFileManager::computeSpace(const vector<Attribute> &recordDescriptor, const void *data)
{
    unsigned space = 0;
    unsigned bytesOfNullIndicator = computeBytesOfNullIndicator(recordDescriptor);

    space += SIZE_RECORD_POS;
    space += bytesOfNullIndicator + recordDescriptor.size() * SIZE_FIELD_LENGTH;
    const byte *pFlag = (const byte *)data;
    const byte *pField = pFlag + bytesOfNullIndicator;
    byte mask = 0x01;
    for (unsigned i = 0; i < recordDescriptor.size(); i++)
    {
        unsigned byteOffset = i / 8;
        unsigned pos = 7 - i % 8; // First flag is at pos 8;
        byte newMask = mask << pos;
        bool isNull = (*(pFlag + byteOffset) & newMask) == newMask;
        cout << recordDescriptor[i].name << isNull << endl;
        if (!isNull)
        {
            Attribute attr = recordDescriptor[i];
            switch (attr.type)
            {
            case TypeInt:
            case TypeReal:
                space += attr.length;
                pField += attr.length;
                break;
            case TypeVarChar:
                uint32_t lenVar = *((const uint32_t *)pField);
                space += lenVar;
                pField += 4 + lenVar;
            }
        }
    }
    return space;
}

PageNum RecordBasedFileManager::getFreePageNum(FileHandle &fileHandle, const unsigned neededSpace)
{
    PageNum dirNum = 0;
    PageNum numEntry = 0;
    byte dir[PAGE_SIZE];
    while (true)
    {
        fileHandle.readPage(dirNum, dir);
        numEntry = *((PageNum *)(dir + PAGE_SIZE - 2 * SIZE_PAGE_NUM));
        for (unsigned i = 0; i < numEntry; i++)
        {
            PageNum pageNum = *((PageNum *)(dir + SIZE_DIR_ENTRY * i));
            unsigned freespace = *((unsigned *)(dir + SIZE_DIR_ENTRY * i + SIZE_PAGE_NUM));
            if (freespace >= neededSpace)
            {
                return pageNum;
            }
        }

        PageNum nextDirNum = *((PageNum *)(dir + PAGE_SIZE - SIZE_PAGE_NUM));
        if (nextDirNum > 0)
        {
            dirNum = nextDirNum;
        }
        else
        {
            break;
        }
    }

    //Free page not found
    byte newPage[PAGE_SIZE] = {0};
    fileHandle.appendPage(newPage);
    PageNum newPageNum = fileHandle.getNumberOfPages() - 1;

    //If current dir page is full, add a new dir page
    if (numEntry >= MAX_NUM_DIR_ENTRY)
    {
        byte newDir[PAGE_SIZE] = {0};
        *((PageNum *)newDir) = newPageNum;
        *((unsigned *)(newDir + SIZE_PAGE_NUM)) = MAX_FREESPACE;
        *((PageNum *)(newDir + PAGE_SIZE - 2 * SIZE_PAGE_NUM)) = 1;
        fileHandle.appendPage(newDir);
        PageNum newDirPageNum = fileHandle.getNumberOfPages();

        *((PageNum *)(dir + PAGE_SIZE - SIZE_PAGE_NUM)) = newDirPageNum;

        return newPageNum;
    }
    else
    {
        *((PageNum *)(dir + numEntry * SIZE_DIR_ENTRY)) = newPageNum;
        *((unsigned *)(dir + numEntry * SIZE_DIR_ENTRY + SIZE_PAGE_NUM)) = MAX_FREESPACE;

        byte *pEntryNum = dir + PAGE_SIZE - SIZE_PAGE_NUM - SIZE_NUM_ENTRY;
        *((unsigned *)(pEntryNum)) = *((unsigned *)(pEntryNum)) + 1;
        fileHandle.writePage(dirNum, dir);
        return newPageNum;
    }

    //If current dir page is full, add a new dir page
    if (numEntry >= MAX_NUM_DIR_ENTRY)
    {
        byte newDir[PAGE_SIZE] = {0};
        *((PageNum *)newDir) = newPageNum;
        *((unsigned *)(newDir + SIZE_PAGE_NUM)) = MAX_FREESPACE;
        *((PageNum *)(newDir + PAGE_SIZE - 2 * SIZE_PAGE_NUM)) = 1;
        fileHandle.appendPage(newDir);
        PageNum newDirPageNum = fileHandle.getNumberOfPages();

        *((PageNum *)(dir + PAGE_SIZE - SIZE_PAGE_NUM)) = newDirPageNum;

        return newPageNum;
    }
    else
    {
        cout << dirNum << endl;
        *((PageNum *)(dir + numEntry * SIZE_DIR_ENTRY)) = newPageNum;
        *((unsigned *)(dir + numEntry * SIZE_DIR_ENTRY + SIZE_PAGE_NUM)) = MAX_FREESPACE;

        byte *pEntryNum = dir + PAGE_SIZE - SIZE_PAGE_NUM - SIZE_NUM_ENTRY;
        *((unsigned *)(pEntryNum)) += 1;
        fileHandle.writePage(dirNum, dir);
        return newPageNum;
    }
}

RC RecordBasedFileManager::transformRecord(const vector<Attribute> &recordDescriptor, const void *data, byte *transData)
{
    unsigned bytesOfNullIndicator = computeBytesOfNullIndicator(recordDescriptor);
    const byte *pFlag = (const byte *)data;
    const byte *pField = pFlag + bytesOfNullIndicator;
    byte mask = 0x01;

    byte *pRet = transData;
    memcpy(pRet, data, bytesOfNullIndicator);
    pRet += bytesOfNullIndicator;

    for (unsigned i = 0; i < recordDescriptor.size(); i++)
    {
        Attribute attr = recordDescriptor[i];

        unsigned byteOffset = i / 8;
        unsigned pos = 7 - i % 8; // First flag is at pos 8;
        byte newMask = mask << pos;
        bool isNull = (*(pFlag + byteOffset) & newMask) == newMask;

        if (!isNull)
        {
            switch (attr.type)
            {
            case TypeInt:
                *((unsigned *)pRet) = attr.length;
                pRet += SIZE_FIELD_LENGTH;
                *((uint32_t *)pRet) = *((const uint32_t *)pField);
                cout << *((const uint32_t *)pField) << endl;
                pRet += attr.length;
                pField += attr.length;
                break;

            case TypeReal:
                *((unsigned *)pRet) = attr.length;
                pRet += SIZE_FIELD_LENGTH;
                *((float *)pRet) = *((const float *)pField);
                cout << *((const float *)pField) << endl;
                pRet += attr.length;
                pField += attr.length;
                break;

            case TypeVarChar:
                unsigned lenVar = *((const uint32_t *)pField);
                *((unsigned *)pRet) = lenVar;
                pRet += SIZE_FIELD_LENGTH;

                pField += 4;
                string value(pField, lenVar);
                cout << value << endl;
                strcpy((char *)pRet, value.c_str());
                pRet += lenVar;
                pField += lenVar;
                break;
            }
        }
        else
        {
            *((unsigned *)pRet) = 0;
            pRet += SIZE_FIELD_LENGTH;
        }
    }
    return SUCCESS;
}
