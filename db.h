#pragma once
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <sstream>

using namespace std;


const string DB_FILE = "warehouse.db";
unordered_map<int, long> g_index;
unordered_map<char, vector<long>> g_categoryIndex;

struct Record {
    int id;
    char name[50];
    int quantity;
    float price;
    char category;
    bool deleted;
    long next_free;
};


struct Header {
    int record_count;
    long free_list_head;
};


void removeFromCategoryIndex(char category, long offset) {
    auto it = g_categoryIndex.find(category);
    if (it == g_categoryIndex.end()) return;

    vector<long>& offsets = it->second;
    for (size_t i = 0; i < offsets.size(); i++) {
        if (offsets[i] == offset) {
            offsets[i] = offsets.back();
            offsets.pop_back();
            break;
        }
    }
    if (offsets.empty()) {
        g_categoryIndex.erase(it);
    }
}


bool createDatabase(const string& filename) {
    ofstream file(filename, ios::binary);
    if (!file) {
        cerr << "Не удалось создать файл\n";
        return false;
    }

    Header header;
    header.record_count = 0;
    header.free_list_head = -1;

    file.write(reinterpret_cast<char*>(&header), sizeof(Header));

    file.close();
    return true;
}


bool addRecord(const std::string& filename, Record newRecord) {
    if (g_index.find(newRecord.id) != g_index.end()) {
        cerr << "Запись с таким id уже существует\n";
        return false;
    }

    fstream file(filename, ios::binary | ios::in | ios::out);
    if (!file) return false;

    Header header;
    file.seekg(0, ios::beg);
    file.read(reinterpret_cast<char*>(&header), sizeof(Header));

    long offset;
    newRecord.deleted = false;

    if (header.free_list_head != -1) {
        offset = header.free_list_head;

        Record freeSlot;
        file.seekg(offset, ios::beg);
        file.read(reinterpret_cast<char*>(&freeSlot), sizeof(Record));

        header.free_list_head = freeSlot.next_free;
    } else {
        offset = sizeof(Header) + header.record_count * sizeof(Record);
        header.record_count++;
    }

    file.seekp(offset, ios::beg);
    file.write(reinterpret_cast<char*>(&newRecord), sizeof(Record));

    file.seekp(0, ios::beg);
    file.write(reinterpret_cast<char*>(&header), sizeof(Header));

    g_index[newRecord.id] = offset;
    g_categoryIndex[newRecord.category].push_back(offset);

    file.close();
    return true;
}


bool openDatabase(const string& filename) {
    g_index.clear();
    g_categoryIndex.clear();

    ifstream file(filename, ios::binary);
    if (!file) {
        cerr << "Не удалось открыть файл\n";
        return false;
    }

    Header header;
    file.read(reinterpret_cast<char*>(&header), sizeof(Header));

    for (int i = 0; i < header.record_count; i++) {
        long offset = sizeof(Header) + i * sizeof(Record);

        Record rec;
        file.seekg(offset, ios::beg);
        file.read(reinterpret_cast<char*>(&rec), sizeof(Record));

        if (!rec.deleted) {
            g_index[rec.id] = offset;
            g_categoryIndex[rec.category].push_back(offset);
        }
    }

    file.close();
    return true;
}


bool findByKey(const string& filename, int id, Record& result) {
    auto it = g_index.find(id);
    if (it == g_index.end()) {
        return false; 
    }

    long offset = it->second;

    ifstream file(filename, ios::binary);
    if (!file) {
        return false;
    }

    file.seekg(offset, ios::beg);
    file.read(reinterpret_cast<char*>(&result), sizeof(Record));

    file.close();
    return true;
}

vector<Record> findByCategory(const string& filename, char category) {
    vector<Record> results;

    auto catIt = g_categoryIndex.find(category);
    if (catIt == g_categoryIndex.end()) {
        return results;
    }

    ifstream file(filename, ios::binary);
    if (!file) {
        return results;
    }

    for (long offset : catIt->second) {
        Record rec;
        file.seekg(offset, ios::beg);
        file.read(reinterpret_cast<char*>(&rec), sizeof(Record));

        if (!rec.deleted) {
            results.push_back(rec);
        }
    }

    file.close();
    return results;
}


bool deleteByKey(const string& filename, int id) {
    auto it = g_index.find(id);
    if (it == g_index.end()) {
        return false;
    }
    long offset = it->second;

    fstream file(filename, ios::binary | ios::in | ios::out);
    if (!file) return false;

    Header header;
    file.seekg(0, ios::beg);
    file.read(reinterpret_cast<char*>(&header), sizeof(Header));

    Record rec;
    file.seekg(offset, ios::beg);
    file.read(reinterpret_cast<char*>(&rec), sizeof(Record));

    rec.deleted = true;
    rec.next_free = header.free_list_head;
    file.seekp(offset, ios::beg);
    file.write(reinterpret_cast<char*>(&rec), sizeof(Record));

    header.free_list_head = offset;
    file.seekp(0, ios::beg);
    file.write(reinterpret_cast<char*>(&header), sizeof(Header));

    g_index.erase(id);
    removeFromCategoryIndex(rec.category, offset);

    file.close();
    return true;
}


int deleteByCategory(const string& filename, char category) {
    auto catIt = g_categoryIndex.find(category);
    if (catIt == g_categoryIndex.end()) {
        return 0;
    }

    vector<long> offsets = catIt->second; 

    fstream file(filename, ios::binary | ios::in | ios::out);
    if (!file) return 0;

    Header header;
    file.seekg(0, ios::beg);
    file.read(reinterpret_cast<char*>(&header), sizeof(Header));

    int deletedCount = 0;

    for (long offset : offsets) {
        Record rec;
        file.seekg(offset, ios::beg);
        file.read(reinterpret_cast<char*>(&rec), sizeof(Record));

        if (rec.deleted) continue;

        rec.deleted = true;
        rec.next_free = header.free_list_head;
        file.seekp(offset, ios::beg);
        file.write(reinterpret_cast<char*>(&rec), sizeof(Record));

        header.free_list_head = offset;
        g_index.erase(rec.id);
        removeFromCategoryIndex(rec.category, offset);

        deletedCount++;
    }

    file.seekp(0, ios::beg);
    file.write(reinterpret_cast<char*>(&header), sizeof(Header));

    file.close();
    return deletedCount;
}


bool editRecord(const string& filename, int oldId, Record updated) {
    auto it = g_index.find(oldId);
    if (it == g_index.end()) {
        return false;
    }
    long offset = it->second;

    if (updated.id != oldId && g_index.find(updated.id) != g_index.end()) {
        cerr << "Новый id уже занят другой записью\n";
        return false;
    }

    fstream file(filename, ios::binary | ios::in | ios::out);
    if (!file) return false;

    Record oldRec;
    file.seekg(offset, ios::beg);
    file.read(reinterpret_cast<char*>(&oldRec), sizeof(Record));

    updated.deleted = false;

    file.seekp(offset, ios::beg);
    file.write(reinterpret_cast<char*>(&updated), sizeof(Record));

    file.close();

    if (updated.id != oldId) {
        g_index.erase(oldId);
    }
    g_index[updated.id] = offset;

    if (updated.category != oldRec.category) {
        removeFromCategoryIndex(oldRec.category, offset);
        g_categoryIndex[updated.category].push_back(offset);
    }

    return true;
}


bool backupDatabase(const string& filename, const string& backupName) {
    try {
        filesystem::copy_file(
            filename, backupName,
            filesystem::copy_options::overwrite_existing
        );
        return true;
    } catch (const exception& e) {
        cerr << "Ошибка backup: " << e.what() << "\n";
        return false;
    }
}

bool restoreDatabase(const string& filename, const string& backupName) {
    try {
        filesystem::copy_file(
            backupName, filename,
            filesystem::copy_options::overwrite_existing
        );
        openDatabase(filename);
        return true;
    } catch (const exception& e) {
        cerr << "Ошибка restore: " << e.what() << "\n";
        return false;
    }
}

bool clearDatabase(const string& filename) {
    g_index.clear();
    g_categoryIndex.clear();
    return createDatabase(filename);
}

bool deleteDatabase(const string& filename) {
    g_index.clear();
    g_categoryIndex.clear();
    return filesystem::remove(filename);
}


int importFromCSV(const string& filename, const string& csvFilename) {
    ifstream csv(csvFilename);
    if (!csv) {
        cerr << "Не удалось открыть CSV файл\n";
        return -1;
    }

    string line;
    int importedCount = 0;

    while (getline(csv, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string idStr, nameStr, qtyStr, priceStr, catStr;

        if (!getline(ss, idStr, ',')) continue;
        if (!getline(ss, nameStr, ',')) continue;
        if (!getline(ss, qtyStr, ',')) continue;
        if (!getline(ss, priceStr, ',')) continue;
        if (!getline(ss, catStr, ',')) continue;

        try {
            Record r;
            r.id = stoi(idStr);
            strncpy(r.name, nameStr.c_str(), sizeof(r.name) - 1);
            r.name[sizeof(r.name) - 1] = '\0';
            r.quantity = stoi(qtyStr);
            r.price = stof(priceStr);
            r.category = catStr.empty() ? '?' : catStr[0];
            r.deleted = false;

            if (addRecord(filename, r)) {
                importedCount++;
            }
        } catch (...) {
            continue;
        }
    }

    csv.close();
    return importedCount;
}
