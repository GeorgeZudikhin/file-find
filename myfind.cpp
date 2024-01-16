/*
 * This program searches for a list of filenames within a specified search path.
 * For each filename:
 * 1. The program creates a child process using fork().
 * 2. The child process performs a search in the given path for its assigned filename.
 * 3. If the file is found, the child process writes the output in the specified format to a pipe.
 * 4. The parent process reads from the pipe and prints the result to stdout.
 * 
 * The use of fork() enables concurrent file searches, with each child process looking for one filename.
 * Inter-process communication (IPC) via pipes ensures that the results can be collected and displayed by the parent process.
*/

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>
#include <cstring>
#include <vector>


#define READ_END 0
#define WRITE_END 1


// Compares two strings, with optional case insensitivity.
bool isEqualString(const std::string& s1, const std::string& s2, bool ignoreCase = false) {
    if (ignoreCase) {
        return strcasecmp(s1.c_str(), s2.c_str()) == 0;
    } 
    return s1 == s2;
}

// Handle the error encountered when opening a directory and print relevant error message.
void handleErrorOpeningDir(const std::string& path) {
    switch (errno) {
        case EACCES: 
            perror(("Access denied to directory: " + path).c_str()); 
            break;
        default:
            perror(("Failed to open directory: " + path).c_str());
    }
}

// Search for a given filename in the specified path.
bool searchForFile(const std::string& path, const std::string& filename, int pipefd[2], bool isRecursive = false, bool ignoreCase = false) 
{
    DIR* dir;
    struct dirent* ent;
    bool isFound = false; // Flag to track if the file is found

    // Try opening the directory.
    if ((dir = opendir(path.c_str())) != NULL) {
        // Loop through each entry in the directory.
        while ((ent = readdir(dir)) != NULL) {
            std::string fileOrDirName = ent->d_name;
            std::string fullpath = path + "/" + fileOrDirName;

            // Skip "." and ".." directories.
            if (fileOrDirName == "." || fileOrDirName == "..") {
                continue;
            }

            // If the entry is a directory and recursive search is enabled, search inside it.
            if (ent->d_type == DT_DIR && isRecursive) {
                isFound = isFound || searchForFile(fullpath, filename, pipefd, isRecursive, ignoreCase); // Propagate found flag
            } 
            // If the entry is a file and its name matches the search term, handle the match.
            else if (ent->d_type == DT_REG && isEqualString(fileOrDirName, filename, ignoreCase)) {
                // A buffer to store the absolute path of the matched file
                char absPath[PATH_MAX];
                // Attempt to resolve the absolute path of the matched file
                if(realpath(fullpath.c_str(), absPath) != NULL) {
                    std::string result = std::to_string(getpid()) + ": " + filename + ": " + absPath + "\n";
                    // Write the result string to the pipe, which communicates with the parent process.
                    // The number of bytes successfully written is returned and checked.
                    ssize_t bytesWritten = write(pipefd[WRITE_END], result.c_str(), result.length());
                    // If there's an error writing to the pipe (e.g., if bytesWritten is negative), handle it.
                    if(bytesWritten < 0) {
                        perror("Error writing to pipe");
                        exit(EXIT_FAILURE);
                    }
                    isFound = true; // Mark the file as found
                }
            }
        }
        closedir(dir);
    } 
    else {
        handleErrorOpeningDir(path);
    }
    return isFound; // Return the found flag
}

int main(int argc, char* argv[]) {
    // Argument parsing section
    bool isRecursive = false;
    bool ignoreCase = false;
    int opt;

    // Parse the provided flags.
    while ((opt = getopt(argc, argv, "Ri")) != -1) {
        switch (opt) {
            case 'R':
                isRecursive = true;
                break;
            case 'i':
                ignoreCase = true;
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " [-R] [-i] searchpath filename1 [filename2] …[filenameN]" << std::endl;
                exit(EXIT_FAILURE);
        }
    }

    // Check for sufficient arguments after the flags.
    if (optind >= argc - 1) {
        std::cerr << "Expected arguments after options\n";
        exit(EXIT_FAILURE);
    }

    // Create a pipe for inter-process communication.   
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("Pipe error!");
        exit(EXIT_FAILURE);
    }

    std::string searchpath = argv[optind];
    optind++;

    std::vector<pid_t> childProcesses;

    // Create child processes to search for each filename in parallel.
    for(int i = optind; i < argc; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Child process error!");
            exit(EXIT_FAILURE);
        }

        // Parent process: store child PID.
        if (pid > 0) {
            childProcesses.push_back(pid);
        } 
        // Child process: perform search and communicate results to parent via pipe.
        else {
            // Close the unused read end
            close(pipefd[READ_END]); 

            bool fileFound = searchForFile(searchpath, argv[i], pipefd, isRecursive, ignoreCase);

            // In case file was not found
            if (!fileFound) {
                // Construct the error message
                std::string errorMsg = std::to_string(getpid()) + ": " + argv[i] + ": File not found in " + searchpath + "\n";
                // Write this error message to the write-end of the pipe so the parent process can read it.
                // The parent will then display this error message to the user.
                ssize_t bytesWritten = write(pipefd[WRITE_END], errorMsg.c_str(), errorMsg.length());
                
                if(bytesWritten < 0) {
                    perror("Error writing to pipe");
                    exit(EXIT_FAILURE);
                }
            }
            
            close(pipefd[WRITE_END]);
            exit(EXIT_SUCCESS);
        }
    }

    close(pipefd[WRITE_END]);

    // executed by the parent process only: read results from all child processes and print.
    for (const pid_t& child: childProcesses) {
        // A buffer to read data from the pipe that communicates with child processes.
        char buffer[512];
        // A variable to hold the count of bytes read from the pipe in each iteration.
        ssize_t count;
        // Continue reading from the pipe as long as there's data (i.e., count is positive).
        // Read up to (sizeof(buffer) - 1) bytes to leave space for a null terminator.
        while ((count = read(pipefd[READ_END], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[count] = '\0';
            std::cout << buffer;
        }

        // Wait for the child process to finish.
        waitpid(child, nullptr, 0);
    }

    close(pipefd[READ_END]);

    return 0;
}