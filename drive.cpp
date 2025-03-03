#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

struct FileNode {
    string name;
    int size;
    int offset;
};

class VirtualDrive {
private:
    vector<FileNode> fileDirectory;
    fstream driveFile;
    int driveSize;
    int nextFreeOffset;
    
    const vector<FileNode>& getFileDirectory() const {
    return fileDirectory;
}


    void saveMetadata() {
        ofstream metaFile("metadata.dat", ios::binary | ios::trunc);
        if (!metaFile) return;
        int count = fileDirectory.size();
        metaFile.write(reinterpret_cast<char*>(&count), sizeof(int));

        for (const auto& file : fileDirectory) {
            int nameLen = file.name.size();
            metaFile.write(reinterpret_cast<char*>(&nameLen), sizeof(int));
            metaFile.write(file.name.c_str(), nameLen);

            int fileSize = file.size;
            int fileOffset = file.offset;
            metaFile.write(reinterpret_cast<char*>(&fileSize), sizeof(int));
            metaFile.write(reinterpret_cast<char*>(&fileOffset), sizeof(int));
        }
        metaFile.close();
    }

    void loadMetadata() {
        ifstream metaFile("metadata.dat", ios::binary);
        if (!metaFile) return;
        int count;
        metaFile.read(reinterpret_cast<char*>(&count), sizeof(int));
        fileDirectory.clear();
        nextFreeOffset = sizeof(int);

        for (int i = 0; i < count; ++i) {
            int nameLen;
            metaFile.read(reinterpret_cast<char*>(&nameLen), sizeof(int));
            string name(nameLen, '\0');
            metaFile.read(&name[0], nameLen);
            int size, offset;
            metaFile.read(reinterpret_cast<char*>(&size), sizeof(int));
            metaFile.read(reinterpret_cast<char*>(&offset), sizeof(int));
            FileNode newFile = {name, size, offset};
            fileDirectory.push_back(newFile);
            nextFreeOffset = max(nextFreeOffset, offset + size);
        }
        metaFile.close();
    }

public:
    VirtualDrive(const string& filename, int size) : driveSize(size), nextFreeOffset(0) {
        driveFile.open(filename, ios::binary | ios::in | ios::out);
        if (!driveFile) {
            driveFile.open(filename, ios::binary | ios::in | ios::out | ios::trunc);
            if (!driveFile) {
                cerr << "Error: Could not create the virtual drive file!" << endl;
                exit(1);
            }
            driveFile.seekp(driveSize - 1);
            driveFile.write("\0", 1);
        }
        loadMetadata();
    }

    ~VirtualDrive() {
        saveMetadata();
        driveFile.close();
    }

    void addFile(const string& filename, const vector<char>& data) {
        if (nextFreeOffset + data.size() > driveSize) {
            cerr << "Error: Not enough space!" << endl;
            return;
        }
        FileNode newFile = {filename, static_cast<int>(data.size()), nextFreeOffset};
        fileDirectory.push_back(newFile);
        driveFile.seekp(nextFreeOffset);
        driveFile.write(data.data(), data.size());
        nextFreeOffset += data.size();
        saveMetadata();
    }

    vector<char> readFile(const string& filename) {
        for (const auto& file : fileDirectory) {
            if (file.name == filename) {
                vector<char> data(file.size);
                driveFile.seekg(file.offset);
                driveFile.read(data.data(), file.size);
                return data;
            }
        }
        cerr << "Error: File not found!" << endl;
        return vector<char>();
    }

    void deleteFile(const string& filename) {
        auto it = find_if(fileDirectory.begin(), fileDirectory.end(), [&](const FileNode& file) {
            return file.name == filename;
        });
        if (it != fileDirectory.end()) {
            fileDirectory.erase(it);
            saveMetadata();
            cout << "File " << filename << " deleted." << endl;
        } else {
            cerr << "Error: File not found!" << endl;
        }
    }

    void listFiles() {
        cout << "Files in the virtual drive:" << endl;
        for (const auto& file : fileDirectory) {
            cout << "File Name: " << file.name << ", Size: " << file.size << " bytes, Offset: " << file.offset << endl;
        }
    }
};

int main() {
    VirtualDrive myDrive("my_drive.vd", 1024 * 1024 * 10);
    
    vector<char> fileData1;
    fileData1.assign({'H', 'e', 'l', 'l', 'o'});
    myDrive.addFile("file1.txt", fileData1);

    vector<char> fileData2;
    fileData2.assign({'T', 'h', 'i', 's', ' ', 'i', 's', ' ', 'f', 'i', 'l', 'e', '2'});
    myDrive.addFile("file2.txt", fileData2);

    myDrive.listFiles();

    vector<char> data = myDrive.readFile("file1.txt");
    cout << "Read file: ";
    for (char c : data) {
        cout << c;
    }
    cout << endl;

    myDrive.deleteFile("file1.txt");
    myDrive.listFiles();

    return 0;
}