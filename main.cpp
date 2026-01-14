#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <queue>

class InputArguments {
public:
    std::string programExecutablePath;
    std::string inputPath;

    InputArguments(int argc, char* argv[]) {
        if (argc != 2) {
            throw std::invalid_argument("Expected exactly one argument: <input_path>");
        }

        programExecutablePath = argv[0];
        inputPath = argv[1];

        bool inputPathIsDirectory = std::filesystem::is_directory(inputPath);
        if (!inputPathIsDirectory) {
            std::stringstream ss;
            ss << "Input path '" << inputPath << "' is not a directory.";
            throw std::invalid_argument(ss.str());
        }
    }
};

void scan_for_duplicates(std::shared_ptr<InputArguments> args) {
    std::map<std::string, std::string> fileNameToPathMap;
    std::map<uintmax_t, std::string> fileSizeToPathMap;

    std::queue<std::filesystem::directory_entry> directoryQueue;
    std::filesystem::path inputPath = args->inputPath;
    std::filesystem::directory_entry firstEntry = std::filesystem::directory_entry(inputPath);
    directoryQueue.emplace(firstEntry);

    while (!directoryQueue.empty()) {
        const std::filesystem::directory_entry currentDirectory = std::move(directoryQueue.front());
        directoryQueue.pop();

        std::cout << "Searching directory: " << currentDirectory.path().generic_string() << std::endl;

        for (const std::filesystem::directory_entry& entry: std::filesystem::directory_iterator(currentDirectory)) {
            if (entry.is_directory()) {
                directoryQueue.emplace(entry);
            } else {
                std::cout << "Found file with name: " << entry.path().filename().generic_string()
                    << " and size: " << entry.file_size() << " bytes."
                    << std::endl;

                uintmax_t fileSize = entry.file_size();
                if (fileSizeToPathMap.contains(fileSize)) {
                    std::cout << "Duplicate file size detected!" << std::endl;
                    std::cout << " - Original file: " << fileSizeToPathMap[fileSize] << std::endl;
                    std::cout << " - Duplicate file: " << entry.path().generic_string() << std::endl;
                } else {
                    fileSizeToPathMap[fileSize] = entry.path().generic_string();
                }
            }
        }
    }

    /*
    for (const std::filesystem::directory_entry& entry: std::filesystem::directory_iterator(args->inputPath)) {
        std::cout << entry.path() << std::endl;
        if (entry.is_directory()) {

        } else {

        }
    }*/
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

    std::cout << "Do you wish to continue? [n]" << std::endl;
    std::string executionConfirmation = "y";
    std::cout << "WARNING: Auto confirmation enabled!";
    // std::getline(std::cin, executionConfirmation);

    std::cout << executionConfirmation << std::endl;

    if (executionConfirmation == "y" || executionConfirmation == "yes") {
        std::cout << "'" << executionConfirmation << "' received, continuing..." << std::endl;
    } else {
        std::cout << "Aborting! (" << executionConfirmation << ") received." << std::endl;
        return 0;
    }

    scan_for_duplicates(inputArguments);

    return 0;
}