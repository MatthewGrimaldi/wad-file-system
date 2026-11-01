#ifndef WAD_H
#define WAD_H

#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <iostream>

using namespace std;

class Wad {
private:
    // WAD descriptor struct
    struct Descriptor {
        int offset;
        int length;
        string name;
        bool isDirectory;
        vector<Descriptor*> children;
        Descriptor* parent;
    };

    // Class attributes
    string magic;
    int numDescriptors;
    int descriptorOffset;
    fstream wadFile;
    Descriptor* root;
    unordered_map<string, Descriptor*> pathMap;
    regex mapMarkerRegex;
    regex namespaceStartRegex;
    regex namespaceEndRegex;

    // Constructor
    Wad(const string& path);

    // Private methods
    void readHeader();
    void buildDirectoryStructure();
    Descriptor* findDescriptor(const string& path);
    void addToPathMap(Descriptor* desc, const string& path);
    void writeDescriptor(const Descriptor* desc);
    void writeDescriptorList();
    void updateWadFile(Descriptor* parent, Descriptor* startMarker, Descriptor* endMarker);
    string getFullPath(Descriptor* desc);

public:
    // Public methods
    static Wad* loadWad(const string& path);
    string getMagic();
    bool isContent(const string& path);
    bool isDirectory(const string& path);
    int getSize(const string& path);
    int getContents(const string& path, char* buffer, int length, int offset = 0);
    int getDirectory(const string& path, vector<string>* directory);
    void createDirectory(const string& path);
    void createFile(const string& path);
    int writeToFile(const string& path, const char* buffer, int length, int offset = 0);

    // Testing methods
    void printPathMap();
    void printDirectory();

    // Destructor
    ~Wad();
};

#endif