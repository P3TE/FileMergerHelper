#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <utility>

class InputArguments {
public:
    std::string programExecutablePath;
    std::string inputPath;
    bool makeChanges = false;

    InputArguments(int argc, char* argv[]) {
        if (argc != 2 && argc != 3) {
            throw std::invalid_argument("Expected exactly one argument: <input_path> [--apply]");
        }

        programExecutablePath = argv[0];
        inputPath = argv[1];

        if (argc == 3)
        {
            const std::string ExpectedArgument = "--apply";
            if (ExpectedArgument == argv[2])
            {
                makeChanges = true;
            } else
            {
                std::stringstream ss;
                ss << "The second parameter MUST be '--apply', but was: '" << argv[2] << "'.";
                throw std::invalid_argument(ss.str());
            }
        }

        bool inputPathIsDirectory = std::filesystem::is_directory(inputPath);
        if (!inputPathIsDirectory) {
            std::stringstream ss;
            ss << "Input path '" << inputPath << "' is not a directory.";
            throw std::invalid_argument(ss.str());
        }
    }
};

class FoundFile
{
public:
    uintmax_t file_size;
    std::string file_path;

    explicit FoundFile(const uintmax_t file_size, std::string file_path)
        : file_size(file_size),
          file_path(std::move(file_path))
    {
    }
};

class FoundCommonFiles
{
public:
    uintmax_t file_size;
    std::vector<FoundFile> known_files;

    void add_file(FoundFile found_file)
    {
        known_files.push_back(std::move(found_file));
    }

    explicit FoundCommonFiles(FoundFile found_file)
    {
        file_size = found_file.file_size;
        add_file(std::move(found_file));
    }
};

bool two_files_have_same_bytes(const std::string& pathA, const std::string& pathB)
{
    std::ifstream streamA(pathA, std::ios::binary);
    std::ifstream streamB(pathB, std::ios::binary);

    constexpr std::streamsize BufferSize = 1024;
    uint8_t bufferA[BufferSize];
    uint8_t bufferB[BufferSize];

    while (streamA && streamB)
    {
        streamA.read(reinterpret_cast<char*>(&bufferA), BufferSize);
        streamB.read(reinterpret_cast<char*>(&bufferB), BufferSize);

        std::streamsize bytesReadA = streamA.gcount();
        std::streamsize bytesReadB = streamB.gcount();

        if (bytesReadA != bytesReadB)
        {
            // Shouldn't happen because we are meant to check the file size before running this...
            return false;
        }

        const int memoryCompareReturnCode = std::memcmp(bufferA, bufferB, bytesReadA);

        if (memoryCompareReturnCode != 0)
        {
            // Contents are not equal...
            return false;
        }
    }

    return true;
}

struct file_size_comparator
{
    inline bool operator() (const std::shared_ptr<FoundCommonFiles>& a, const std::shared_ptr<FoundCommonFiles>& b) const
    {
        return (a->file_size < b->file_size);
    }
};

void display_duplicates(const std::map<uintmax_t, std::vector<std::shared_ptr<FoundCommonFiles>>>& file_size_to_path_map)
{
    std::vector<std::shared_ptr<FoundCommonFiles>> duplicate_found_files;

    for (const auto& map_pair : file_size_to_path_map)
    {
        for (const auto& found_common_files : map_pair.second)
        {
            if (found_common_files->known_files.size() > 1)
            {
                duplicate_found_files.emplace_back(found_common_files);
            }
        }
    }

    std::ranges::sort(duplicate_found_files.begin(), duplicate_found_files.end(), file_size_comparator());
    // reverse(duplicateFileSizes.begin(), duplicateFileSizes.end());

    // TODO - Handle the case where files have the same size in bytes, but the contents differ.
    //          An example of this is the warcraft III screenshots.

    for (auto files_with_duplicates : duplicate_found_files)
    {
        // const std::vector<std::string>& filePaths = file_size_to_path_map.at(duplicateFileSize);

        std::cout << "Duplicate file size detected of size " << files_with_duplicates->file_size << " bytes! Paths include:" << std::endl;
        for (const FoundFile& filePath : files_with_duplicates->known_files)
        {
            std::filesystem::path path = filePath.file_path;
            std::filesystem::path parentPath = path.parent_path();
            std::cout << parentPath.generic_string() << std::endl;
            std::cout << " - (" << path.filename().generic_string() << ")" << std::endl;
        }

        std::cout << std::endl;
    }
}

void scan_for_duplicates(std::shared_ptr<InputArguments> args) {
    std::map<std::string, std::string> fileNameToPathMap;
    std::map<uintmax_t, std::vector<std::shared_ptr<FoundCommonFiles>>> fileSizeToPathMap;

    std::queue<std::filesystem::directory_entry> directoryQueue;
    std::filesystem::path inputPath = args->inputPath;
    std::filesystem::directory_entry firstEntry = std::filesystem::directory_entry(inputPath);
    directoryQueue.emplace(firstEntry);

    while (!directoryQueue.empty()) {
        const std::filesystem::directory_entry currentDirectory = std::move(directoryQueue.front());
        directoryQueue.pop();

        // std::cout << "Searching directory: " << currentDirectory.path().generic_string() << std::endl;

        for (const std::filesystem::directory_entry& entry: std::filesystem::directory_iterator(currentDirectory)) {
            if (entry.is_directory()) {
                directoryQueue.emplace(entry);
            } else {
                std::string entryPath = entry.path().generic_string();

                /*
                std::cout << "Found file with name: " << entry.path().filename().generic_string()
                    << " and size: " << entry.file_size() << " bytes."
                    << " with path: " << entryPath
                    << std::endl;
                    */

                uintmax_t file_size = entry.file_size();

                FoundFile found_file(file_size, entryPath);

                if (fileSizeToPathMap.contains(file_size)) {
                    std::vector<std::shared_ptr<FoundCommonFiles>>& existing_common_files = fileSizeToPathMap.at(file_size);

                    bool same_file_found = false;

                    for (std::shared_ptr<FoundCommonFiles>& existing_common_file : existing_common_files)
                    {
                        const FoundFile& potential_same_file = existing_common_file->known_files[0];

                        if (two_files_have_same_bytes(found_file.file_path, potential_same_file.file_path))
                        {
                            existing_common_file->add_file(std::move(found_file));
                            same_file_found = true;
                            break;
                        }
                    }

                    if (!same_file_found)
                    {
                        auto new_shared_ptr = std::make_shared<FoundCommonFiles>(std::move(found_file));
                        existing_common_files.push_back(std::move(new_shared_ptr));
                    }
                } else {
                    std::vector<std::shared_ptr<FoundCommonFiles>> new_entry;
                    auto new_shared_ptr = std::make_shared<FoundCommonFiles>(std::move(found_file));
                    new_entry.push_back(std::move(new_shared_ptr));
                    fileSizeToPathMap[file_size] = std::move(new_entry);
                }
            }
        }
    }

    display_duplicates(fileSizeToPathMap);
}



int main(int argc, char* argv[]) {

    const std::string ProgramName = "File Duplicate Scanner";
    const std::string ProgramVersion = "0.0.1";

    std::cout << "Starting " << ProgramName << " version " << ProgramVersion << std::endl;

    // Process arguments.
    std::shared_ptr<InputArguments> inputArguments;
    try {
        inputArguments = std::make_shared<InputArguments>(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error processing input arguments: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Input path: " << inputArguments->inputPath << std::endl;

    if (inputArguments->makeChanges)
    {
        std::cout << "Make changes is set to TRUE, The file system WILL be modified!" << std::endl;

        std::cout << "Do you wish to continue? [n]: ";
        std::string executionConfirmation = "n";
        std::getline(std::cin, executionConfirmation);

        if (executionConfirmation == "y" || executionConfirmation == "yes") {
            std::cout << "'" << executionConfirmation << "' received, continuing..." << std::endl;
        } else {
            std::cout << "Aborting! (" << executionConfirmation << ") received." << std::endl;
            return 0;
        }
    } else
    {
        std::cout << "Make changes is set to false, no file system changes will be made." << std::endl;
    }

    scan_for_duplicates(inputArguments);

    return 0;
}