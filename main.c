#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define MAXLINE 2024
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

// Function to find a pattern in a file
void find_in_file(char filename[], char pattern[]) {
  FILE *fptr;
  char buff[MAXLINE];

  // Open the file for reading
#ifdef _WIN32
  errno_t err = fopen_s(&fptr, filename, "r");
  if (err != 0) {
#else
  fptr = fopen(filename, "r");
  if (fptr == NULL) {
#endif
    printf("%sERROR:%s Couldn't open file: %s\n", ANSI_COLOR_RED,
           ANSI_COLOR_RESET, filename);
    return;
  }

  int counter = 1;

  // Read the file line by line
  while (fgets(buff, MAXLINE, fptr)) {
    int i, j, k;
    for (i = 0; buff[i] != '\n'; i++) {
      for (j = i, k = 0; buff[j] == pattern[k] && pattern[k] != '\0'; k++, j++)
        ;
      if (pattern[k] == '\0') { // Pattern found
        printf("%s%s:%d:%s%s", ANSI_COLOR_RED, filename, counter,
               ANSI_COLOR_RESET, buff);
        break;
      }
    }

    counter++;
  }

  fclose(fptr);
}

#ifdef _WIN32
// Function to list directory contents on Windows
bool ListDirectoryContents(const char *sDir, char pattern[]) {
  WIN32_FIND_DATA fdFile;
  HANDLE hFind = NULL;

  char sPath[2048];
  char fullFilePath[2048]; // Separate buffer for the full file path

  // Specify a file mask. *.* = We want everything!
  sprintf(sPath, "%s\\*.*", sDir);

  if ((hFind = FindFirstFile(sPath, &fdFile)) == INVALID_HANDLE_VALUE) {
    printf("Path not found: [%s]\n", sDir);
    return false;
  }

  do {
    // Skip "." and ".." directories
    if (strcmp(fdFile.cFileName, ".") != 0 &&
        strcmp(fdFile.cFileName, "..") != 0) {
      // Build up the file path
      sprintf(fullFilePath, "%s\\%s", sDir, fdFile.cFileName);

      // Is the entity a File or Folder?
      if (fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        // Recursively search in subdirectories
        ListDirectoryContents(fullFilePath, pattern);
      } else {
        // Skip .exe files
        const char *extension = strrchr(fdFile.cFileName, '.');
        if (extension && _stricmp(extension, ".exe") == 0) {
          continue;
        }

        // Search the pattern in the file
        find_in_file(fullFilePath, pattern);
      }
    }
  } while (FindNextFile(hFind, &fdFile)); // Find the next file.

  FindClose(hFind); // Always clean up!

  return true;
}

#else
// Function to list directory contents on Linux
bool ListDirectoryContents(const char *sDir, char pattern[]) {
  struct dirent *entry;
  DIR *dp = opendir(sDir);

  if (dp == NULL) {
    printf("Path not found: [%s]\n", sDir);
    return false;
  }

  char fullFilePath[2048];
  struct stat path_stat;

  while ((entry = readdir(dp)) != NULL) {
    // Skip "." and ".." directories
    if (strcmp(entry->d_name, ".") != 0 &&
        strcmp(entry->d_name, "..") != 0) {
      // Build up the file path
      snprintf(fullFilePath, sizeof(fullFilePath), "%s/%s", sDir, entry->d_name);

      // Get file attributes
      stat(fullFilePath, &path_stat);

      if (S_ISDIR(path_stat.st_mode)) {
        // Recursively search in subdirectories
        ListDirectoryContents(fullFilePath, pattern);
      } else {
        // Skip .exe files
        const char *extension = strrchr(entry->d_name, '.');
        if (extension && strcmp(extension, ".exe") == 0) {
          continue;
        }

        // Search the pattern in the file
        find_in_file(fullFilePath, pattern);
      }
    }
  }

  closedir(dp); // Always clean up!

  return true;
}
#endif

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("NOT ENOUGH ARGUMENTS\ncgrep <directory> <pattern>\n");
    return EXIT_FAILURE;
  }

  ListDirectoryContents(argv[1], argv[2]);

  return EXIT_SUCCESS;
}
