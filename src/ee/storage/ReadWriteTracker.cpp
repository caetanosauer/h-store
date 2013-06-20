/* Copyright (C) 2013 by H-Store Project
 * Brown University
 * Massachusetts Institute of Technology
 * Yale University
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "common/debuglog.h"
#include "common/FatalException.hpp"
#include "common/ValueFactory.hpp"
#include "storage/ReadWriteTracker.h"
#include "storage/tablefactory.h"

using namespace std;

namespace voltdb {

ReadWriteTracker::ReadWriteTracker(int64_t txnId) :
        txnId(txnId) {
    
    // Let's get it on!
}

ReadWriteTracker::~ReadWriteTracker() {
    // TODO: Do we need to free up the entries in our maps?
}
    
void ReadWriteTracker::markTupleRead(const std::string tableName, TableTuple *tuple) {
    uint32_t tupleId = tuple->getTupleID();
    this->reads.insert(std::make_pair(tableName, tupleId));
}

void ReadWriteTracker::markTupleWritten(const std::string tableName, TableTuple *tuple) {
    uint32_t tupleId = tuple->getTupleID();
    this->writes.insert(std::make_pair(tableName, tupleId));
    
}

std::vector<std::string> ReadWriteTracker::getTableNames(boost::unordered_map<std::string, rowOffsets> *map) const {
    std::vector<std::string> tableNames;
    tableNames.reserve(map->size());
    boost::unordered_map<std::string, rowOffsets>::const_iterator iter = map->begin();
    while (iter != map->end()) {
        tableNames.push_back(iter->first);
        iter++;
    } // FOR
    return (tableNames);
}

std::vector<std::string> ReadWriteTracker::getTablesRead() {
    return this->getTableNames(&this->reads);
}    
std::vector<std::string> ReadWriteTracker::getTablesWritten() {
    return this->getTableNames(&this->writes);
}

void ReadWriteTracker::clear() {
    this->reads.clear();
    this->writes.clear();
}

// -------------------------------------------------------------------------

ReadWriteTrackerManager::ReadWriteTrackerManager(ExecutorContext *ctx) : executorContext(ctx) {
    CatalogId databaseId = 1;
    this->resultSchema = TupleSchema::createTrackerTupleSchema();
    
    string *resultColumnNames = new string[this->resultSchema->columnCount()];
    resultColumnNames[0] = std::string("TABLE_NAME");
    resultColumnNames[1] = std::string("TUPLE_ID"); 
    
    this->resultTable = reinterpret_cast<Table*>(voltdb::TableFactory::getTempTable(
                databaseId,
                std::string("ReadWriteTrackerManager"),
                this->resultSchema,
                resultColumnNames,
                NULL));
}

ReadWriteTrackerManager::~ReadWriteTrackerManager() {
    TupleSchema::freeTupleSchema(this->resultSchema);
    delete this->resultTable;
    
    boost::unordered_map<int64_t, ReadWriteTracker*>::const_iterator iter = this->trackers.begin();
    while (iter != this->trackers.end()) {
        delete iter->second;
        iter++;
    } // FOR
}

ReadWriteTracker* ReadWriteTrackerManager::enableTracking(int64_t txnId) {
    ReadWriteTracker *tracker = new ReadWriteTracker(txnId);
    trackers[txnId] = tracker;
    return (tracker);
}

ReadWriteTracker* ReadWriteTrackerManager::getTracker(int64_t txnId) {
    boost::unordered_map<int64_t, ReadWriteTracker*>::const_iterator iter;
    iter = trackers.find(txnId);
    if (iter != trackers.end()) {
        return iter->second;
    }
    return (NULL);
}

void ReadWriteTrackerManager::removeTracker(int64_t txnId) {
    ReadWriteTracker *tracker = this->getTracker(txnId);
    if (tracker != NULL) {
        trackers.erase(txnId);
        delete tracker;
    }
}

void ReadWriteTrackerManager::getTuples(boost::unordered_map<std::string, rowOffsets> *map) const {
    this->resultTable->deleteAllTuples(false);
    TableTuple tuple = this->resultTable->tempTuple();
    boost::unordered_map<std::string, rowOffsets>::const_iterator iter = map->begin();
    while (iter != map->end()) {
        rowOffsets::const_iterator tupleIter = iter->second.begin();
        while (tupleIter != iter->second.end()) {
            int idx = 0;
            tuple.setNValue(idx++, ValueFactory::getStringValue(iter->first)); // TABLE_NAME
            tuple.setNValue(idx++, ValueFactory::getIntegerValue(*tupleIter)); // TUPLE_ID
            this->resultTable->insertTuple(tuple);
            tupleIter++;
        } // WHILE
        iter++;
    } // WHILE
    return;
}

Table* ReadWriteTrackerManager::getTuplesRead(ReadWriteTracker *tracker) {
    this->getTuples(&tracker->reads);
    return (this->resultTable);
}

Table* ReadWriteTrackerManager::getTuplesWritten(ReadWriteTracker *tracker) {
    this->getTuples(&tracker->writes);
    return (this->resultTable);
}

}