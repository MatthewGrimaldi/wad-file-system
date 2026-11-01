#include "Wad.h"

using namespace std;

// Public constructor
Wad* Wad::loadWad(const string& path) {
    return new Wad(path);
}

// Private constructor
Wad::Wad(const string& path) : wadFile(path, ios::in | ios::out | ios::binary), mapMarkerRegex("^E[0-9]M[0-9]$"), namespaceStartRegex("^.{1,2}_START$"), namespaceEndRegex("^.{1,2}_END$") {
    if (!wadFile) {
        throw runtime_error("Failed to open WAD file");
    }
    readHeader();
    buildDirectoryStructure();
}

// Helper function for parsing WAD header
void Wad::readHeader() {
    char magicBuffer[5] = {0};
    wadFile.read(magicBuffer, 4);
    magic = string(magicBuffer);

    wadFile.read(reinterpret_cast<char*>(&numDescriptors), sizeof(int));
    wadFile.read(reinterpret_cast<char*>(&descriptorOffset), sizeof(int));
}

// Helper function for building in-memory directory structure
void Wad::buildDirectoryStructure() {
    root = new Descriptor{0, 0, "/", true, {}, nullptr};
    vector<Descriptor*> directoryStack = {root};

    wadFile.seekg(descriptorOffset);
    for (int i = 0; i < numDescriptors; ++i) {
        Descriptor* desc = new Descriptor();
        wadFile.read(reinterpret_cast<char*>(&desc->offset), sizeof(int));
        wadFile.read(reinterpret_cast<char*>(&desc->length), sizeof(int));
        
        char nameBuffer[9] = {0};
        wadFile.read(nameBuffer, 8);
        desc->name = string(nameBuffer);

        bool isMapMarker = regex_match(desc->name, mapMarkerRegex);
        bool isNamespaceStart = regex_match(desc->name, namespaceStartRegex);
        bool isNamespaceEnd = regex_match(desc->name, namespaceEndRegex);

        if (isNamespaceEnd) {
            if (directoryStack.size() > 1) {
                directoryStack.pop_back();
            }
        } else {
            desc->parent = directoryStack.back();
            directoryStack.back()->children.push_back(desc);

            if (isMapMarker || isNamespaceStart) {
                desc->isDirectory = true;
                directoryStack.push_back(desc);

                if (isMapMarker) {
                    for (int j = 0; j < 10 && i + 1 < numDescriptors; ++j, ++i) {
                        Descriptor* mapElement = new Descriptor();
                        wadFile.read(reinterpret_cast<char*>(&mapElement->offset), sizeof(int));
                        wadFile.read(reinterpret_cast<char*>(&mapElement->length), sizeof(int));
                        wadFile.read(nameBuffer, 8);
                        mapElement->name = string(nameBuffer);
                        mapElement->isDirectory = false;
                        mapElement->parent = desc;
                        desc->children.push_back(mapElement);
                    }
                    directoryStack.pop_back();
                }
            } else {
                desc->isDirectory = false;
            }
        }
    }

    addToPathMap(root, "");
}

// Helper function for appending path-descriptor pairs to Path Map
void Wad::addToPathMap(Descriptor* desc, const string& parentPath) {
    string fullPath;
    string name = desc->name;
    
    // Remove "_START" namespace markers
    if (regex_match(name, namespaceStartRegex)) {
        name = name.substr(0, name.length() - 6);
    }
    
    if (parentPath.empty() || parentPath == "/") {
        fullPath = "/" + name;
    } else {
        fullPath = parentPath + "/" + name;
    }
    
    // Remove any duplicate slashes
    fullPath.erase(unique(fullPath.begin(), fullPath.end(), [](char a, char b) { return a == '/' && b == '/'; }), fullPath.end());
    
    // Don't add backslash for files
    if (!desc->isDirectory && fullPath.back() == '/') {
        fullPath.pop_back();
    }
    
    pathMap[fullPath] = desc;
    
    // For directories, recursively add children
    if (desc->isDirectory) {
        for (auto child : desc->children) {
            addToPathMap(child, fullPath);
        }
    }
}

// Testing function for printing Path Map
void Wad::printPathMap() {
    cout << "--Current PathMap: " << endl;
    for (auto& pair : pathMap) {
        cout << "   Path: " << pair.first << " | Object: " << pair.second->name << endl;
    }
}

// Testing function for printing in-memory directory structure
void Wad::printDirectory() {
    cout << "--Current Directory Structure:" << endl;
    for (auto child : root->children) {
        cout << "   " << child->name << endl;
        for (auto grandchild : child->children) {
            cout << "       " << grandchild->name << endl;
            for (auto greatgrandchild : grandchild->children) {
                cout << "           " << greatgrandchild->name << endl;
                for (auto greater : greatgrandchild->children) {
                    cout << "               " << greater->name << endl;
                    for (auto greatest : greater->children) {
                        cout << "                   " << greatest->name << endl;
                    }
                }
            }
        }
    }
}

// Helper function for traversing structure to find specific descriptor
Wad::Descriptor* Wad::findDescriptor(const string& path) {
    // Is path valid?
    if (path == "" ) {
        return nullptr;
    }

    string searchPath = path;
    
    // If the path doesn't end with a slash and it's not the root, add a slash for directory search
    if (path != "/" && path.back() != '/') {
        searchPath += '/';
    }

    auto it = pathMap.find(searchPath);
    if (it != pathMap.end()) {
        return it->second;
    }
    
    // If not found with added slash, try without (for files)
    if (searchPath.back() == '/' && searchPath != "/") {
        searchPath.pop_back();
        it = pathMap.find(searchPath);
        if (it != pathMap.end()) {
            return it->second;
        }
    }

    // If still not found, try adding "_START" to the last component
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash != string::npos) {
        string modifiedPath = path.substr(0, lastSlash + 1) + 
                                   path.substr(lastSlash + 1) + "_START";
        it = pathMap.find(modifiedPath);
        if (it != pathMap.end()) {
            return it->second;
        }
    }
    
    return nullptr;
}

// Getter for WAD magic value
string Wad::getMagic() {
    return magic;
}

// Getter for whether path leads to content (file)
bool Wad::isContent(const string& path) {
    Descriptor* desc = findDescriptor(path);

    return desc && !desc->isDirectory;
}

// Getter for whether path leads to directory
bool Wad::isDirectory(const string& path) {
    Descriptor* desc = findDescriptor(path);

    if (!desc) return false;
    
    return desc->isDirectory || regex_match(desc->name, mapMarkerRegex) || regex_match(desc->name, namespaceStartRegex);
}

// Getter for size of the object path leads to
int Wad::getSize(const string& path) {
    Descriptor* desc = findDescriptor(path);

    return (desc && !desc->isDirectory) ? desc->length : -1;
}

// Getter for the contents of a file
int Wad::getContents(const string& path, char* buffer, int length, int offset) {
    Descriptor* desc = findDescriptor(path);
    // Does path exist?
    if (!desc || desc->isDirectory) {
        return -1;
    }

    int availableBytes = max(0, desc->length - offset);
    int bytesToRead = min(length, availableBytes);

    wadFile.seekg(desc->offset + offset);
    wadFile.read(buffer, bytesToRead);

    return bytesToRead;
}

// Getter for a vector of directories on a given level
int Wad::getDirectory(const string& path, vector<string>* directory) {
    Descriptor* desc = findDescriptor(path);
    // Does path exist?
    if (!desc || !desc->isDirectory) {
        return -1;
    }

    for (auto child : desc->children) {
        string childName = child->name;
        if (regex_match(childName, namespaceStartRegex)) {
            childName = childName.substr(0, childName.length() - 6);
        }
        if (!regex_match(childName, namespaceEndRegex)) {
            directory->push_back(childName);
        }
    }

    return directory->size();
}

// Creates a new namespace directory with the given path
void Wad::createDirectory(const string& path) {
    // Is path valid?
    if (path.empty() || path == "/") {
        return;
    }

    string alt_path = path;
    if (path.back() == '/') {
        alt_path.pop_back();
    }

    size_t lastSlash = alt_path.find_last_of('/');
    string dirName = alt_path.substr(lastSlash + 1);
    string parentPath = (lastSlash == 0) ? "/" : alt_path.substr(0, lastSlash);

    // Is name valid?
    if (dirName.length() > 2) {
        return;
    }

    // Is parent valid?
    Descriptor* parent = findDescriptor(parentPath);
    if (!parent || !parent->isDirectory || regex_match(parent->name, mapMarkerRegex)) {
        return;
    }

    Descriptor* startMarker = new Descriptor{0, 0, dirName + "_START", true, {}, parent};
    Descriptor* endMarker = new Descriptor{0, 0, dirName + "_END", true, {}, parent};

    parent->children.push_back(startMarker);
    parent->children.push_back(endMarker);

    updateWadFile(parent, startMarker, endMarker);
    addToPathMap(startMarker, parentPath);
}

// Creates a new file with the given path
void Wad::createFile(const string& path) {
    // Is path valid?
    if (path.empty() || path == "/") {
        return;
    }

    size_t lastSlash = path.find_last_of('/');
    string fileName = path.substr(lastSlash + 1);
    string parentPath = (lastSlash == 0) ? "/" : path.substr(0, lastSlash);

    // Is name valid?
    if (fileName.length() > 8 || regex_match(fileName, mapMarkerRegex)) {
        return;
    }

    // Is parent valid?
    Descriptor* parent = findDescriptor(parentPath);
    if (!parent || (!parent->isDirectory && parentPath != "/") || regex_match(parent->name, mapMarkerRegex)) {
        return;
    }

    Descriptor* newFile = new Descriptor{0, 0, fileName, false, {}, parent};

    if (parentPath == "/") {
        // Adding to root directory
        root->children.push_back(newFile);
    } else {
        // Adding to a namespace directory
        auto insertPos = find_if(parent->children.rbegin(), parent->children.rend(), [](const Descriptor* d) { return d->name.find("_END") == string::npos; }); parent->children.insert(insertPos.base(), newFile);
    }

    updateWadFile(parent, newFile, nullptr);
    addToPathMap(newFile, parentPath);
}

// Writes to a file at location of the path
int Wad::writeToFile(const string& path, const char* buffer, int length, int offset) {
    // Dpes path exist?
    Descriptor* desc = findDescriptor(path);
    if (!desc || desc->isDirectory || regex_match(path, mapMarkerRegex))  {
        return -1;
    }

    // Is file empty?
    if (desc->length != 0) {
        return 0;  
    }

    wadFile.seekp(0, ios::end);
    int newDataOffset = wadFile.tellp();
    wadFile.write(buffer, length);

    desc->offset = newDataOffset;
    desc->length = length;

    wadFile.seekp(descriptorOffset + (numDescriptors - 1) * 16);
    writeDescriptor(desc);

    return length;
}

// Helper function for writing new directories and files to the WAD file
void Wad::writeDescriptor(const Descriptor* desc) {
    wadFile.write(reinterpret_cast<const char*>(&desc->offset), sizeof(int));
    wadFile.write(reinterpret_cast<const char*>(&desc->length), sizeof(int));
    wadFile.write(desc->name.c_str(), 8);
}

// Helper function for updating descriptors
void Wad::writeDescriptorList() {
    wadFile.seekp(descriptorOffset);
    function<void(Descriptor*)> writeDesc = [&](Descriptor* desc) {
        if (desc != root) {
            writeDescriptor(desc);
        }
        for (auto child : desc->children) {
            writeDesc(child);
        }
        if (desc->isDirectory && desc != root && regex_match(desc->name, namespaceStartRegex)) {
            Descriptor endMarker = {0, 0, desc->name.substr(0, desc->name.length() - 6) + "_END", true, {}, desc->parent};
            writeDescriptor(&endMarker);
        }
    };
    writeDesc(root);
}

// Helper function for updating WAD file
void Wad::updateWadFile(Descriptor* parent, Descriptor* newDesc, Descriptor* endMarker) {
    wadFile.seekp(0, ios::end);
    int newOffset = wadFile.tellp();

    writeDescriptor(newDesc);
    if (endMarker) {
        writeDescriptor(endMarker);
        numDescriptors += 2;
    } else {
        numDescriptors += 1;
    }

    wadFile.seekp(4);
    wadFile.write(reinterpret_cast<char*>(&numDescriptors), sizeof(int));

    wadFile.seekp(descriptorOffset);
    writeDescriptorList();

    string parentPath = (parent == root) ? "" : getFullPath(parent);
    addToPathMap(newDesc, parentPath);
}

// Helper function for determining the full path of a descriptor
string Wad::getFullPath(Descriptor* desc) {
    vector<string> pathComponents;
    while (desc != root) {
        pathComponents.push_back(desc->name);
        desc = desc->parent;
    }
    string fullPath = "/";
    for (auto it = pathComponents.rbegin(); it != pathComponents.rend(); ++it) {
        fullPath += *it + "/";
    }
    return fullPath;
}

// Desctructor
Wad::~Wad() {
    wadFile.close();
    function<void(Descriptor*)> cleanup = [&](Descriptor* node) {
        for (auto child : node->children) {
            cleanup(child);
        }
        delete node;
    };
    cleanup(root);
}