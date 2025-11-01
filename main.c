/*
Here is a simple example of C code to recursively scan a directory tree and list all files and subdirectories.
This code uses the POSIX opendir, readdir, and closedir functions, which are commonly available on Linux and UNIX-like systems.
*/

#if 1

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#define  PATHSIZE  256
#define  NAMESIZE  64
#define  EXTSIZE   8
#define  DB_SIZE   100000     // DB rows
#define  UPLOAD    "upload"
#define  UNSORTED  "unsorted"

typedef struct
{
    char upload[NAMESIZE];
    char unsorted[NAMESIZE];
} check_t;


typedef struct
{
    void     *next;    // Reserved for future use (dynamic database allocation)
    void     *prev;
    //
    _off_t   fileSize;
    char     filePath[PATHSIZE];
    char     baseName[NAMESIZE];
    char     extension[EXTSIZE];
    //
    time_t   ctime;    // Creation time
    time_t   mtime;    // Modification time
    time_t   atime;    // Access time
    //
    int      isupload;
    int      isunsorted;
} db_columns_t;


typedef struct
{
    int           count;   // Count of active database entries
    db_columns_t *data;
} database_t;


db_columns_t  db_data[DB_SIZE];
check_t       check;


db_columns_t *db_next_entry( db_columns_t *entry )
{
    #if 1
    return entry->next;  // Simulate usage of dynamically allocated memory
    #else
    return ++entry;
    #endif
}

db_columns_t *db_new_entry( db_columns_t *entry )
{
    #if 1
    // Simulate dynamic memory allocation for new db_entry
    void  *prev = entry;
    entry->next = entry + 1;
    entry = entry->next;
    entry->prev = prev;
    return  entry;
    #else
    return ++entry;
    #endif
}

// ------------------------------------------------------------------------------------------

void extractPathComponents(const char *filePath, char *path, char *baseName, char *extension)
{
    const char *lastSlash = strrchr(filePath, '/');   // Find the last '/'
    const char *lastDot   = strrchr(filePath, '.');   // Find the last '.'

    if (lastSlash) {
        // Extract path
        strncpy(path, filePath, lastSlash - filePath);
        path[lastSlash - filePath] = '\0';
    } else {
        strcpy(path, ""); // No path found
    }

    if (lastDot && lastDot > lastSlash) {
        // Extract base name
        strncpy(baseName, lastSlash ? lastSlash + 1 : filePath, lastDot - (lastSlash ? lastSlash + 1 : filePath));
        baseName[lastDot - (lastSlash ? lastSlash + 1 : filePath)] = '\0';

        // Extract extension
        strcpy(extension, lastDot + 1);
    } else {
        // No extension found
        strcpy(baseName, lastSlash ? lastSlash + 1 : filePath);
        strcpy(extension, "");
    }
}


// Return count of file infos added into data base
int scanDirectoryTree( database_t *db, const char *dirPath)
{
    int           count    =  db->count;
    db_columns_t *db_entry = &db->data[ db->count ];

    char path[256], baseName[256], extension[256];

    struct dirent *entry;
    DIR *dp = opendir(dirPath);

    if (dp == NULL) {
        perror("opendir");
        return -1;
    }

    while ((entry = readdir(dp)) != NULL)
    {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, entry->d_name);

        struct stat pathStat;
        if (stat(fullPath, &pathStat) == 0)
        {
            if (S_ISDIR(pathStat.st_mode))
            {
                printf("Directory: %s\n\n", fullPath);
                // Recursively scan subdirectory
                count = scanDirectoryTree(db, fullPath);
            } else if (S_ISREG(pathStat.st_mode))
            {
                extractPathComponents(fullPath, path, baseName, extension);

                if ( count < DB_SIZE )
                {
                    strncpy(db_entry->filePath,  path,      PATHSIZE);
                    strncpy(db_entry->baseName,  baseName,  NAMESIZE);
                    strncpy(db_entry->extension, extension, EXTSIZE);
                    //
                    db_entry->fileSize = pathStat.st_size;
                    db_entry->ctime    = pathStat.st_ctime;
                    db_entry->mtime    = pathStat.st_mtime;
                    db_entry->atime    = pathStat.st_atime;
                    count             += 1;
                    //
                    #if 1
                    printf("Count: %d\n", count);
                    printf("File: %s  size=%ld\n", fullPath, pathStat.st_size);

                    printf("Path: %s\n",      db_entry->filePath);
                    printf("Base Name: %s\n", db_entry->baseName);
                    printf("Extension: %s\n", db_entry->extension);
                    printf("\n");
                    #endif
                    //
                    db_entry = db_new_entry( db_entry );
                }
            }
        } else {
            perror("stat");
        }
    }
    closedir(dp);
    db->count = count;
    return  db->count;
}

// ------------------------------------------------------------------------------------------

void print_db( db_columns_t *db_entry, int count )
{

    for( int i = 0; i < count; i++ )
    {
        printf( "%2d: %s - %s.%s\n", i, db_entry->filePath, db_entry->baseName, db_entry->extension);
        db_entry = db_next_entry( db_entry );
    }
}


void print_new_upload( db_columns_t *db_entry )
{
    printf("New upload: %s.%s  size=%ld\n", db_entry->baseName, db_entry->extension, db_entry->fileSize);
}

// ------------------------------------------------------------------------------------------

int mark_upload_unsorted( database_t *db, check_t *check )
{
    int found = 0;
    int count = db->count;

    db_columns_t *db_entry = db->data;

    while ( count--  > 0 )
    {
        if (strstr(db_entry->filePath, check->upload)) {
            db_entry->isupload = 1;
            found += 1;
        }
        if (strstr(db_entry->filePath, check->unsorted)) {
            db_entry->isunsorted = 1;
            found += 1;
        }
        db_entry = db_next_entry( db_entry );
    }
    return found;
}


int check_is_same_file( db_columns_t *entry1, db_columns_t *entry2 )
{
//  printf("%s - %s.%s - %s - %s.%s\n", entry1->filePath, entry1->baseName, entry1->extension, entry2->filePath, entry2->baseName,  entry2->extension );

//  int filePath  =  strcmp(entry1->filePath,  entry2->filePath)  ? 0 : 1;
//  int baseName  =  strcmp(entry1->baseName,  entry2->baseName)  ? 0 : 1;
//  int extension =  strcmp(entry1->extension, entry2->extension) ? 0 : 1;
//  int fileSize  = (entry1->fileSize == entry2->fileSize)        ? 1 : 0;

    if ( entry1->fileSize != entry2->fileSize ) {
        return 0;
    }
    if ( strcmp(entry1->baseName, entry2->baseName) ) {
        return 0;
    }
    if ( strcmp(entry1->extension, entry2->extension) ) {
        return 0;
    }
    return 1; // Files are same
}


int find_new_uploads( database_t *db )
{
    int found = 0;
    int count = db->count;

    db_columns_t *db_entry = db->data;

    for (int i = 0; i < count; i++)
    {
        db_columns_t *entry1 = &db_entry[i];
        int           exist  = 0;

        if ( !entry1->isupload ) {
            continue;
        }
        for (int j = 0; j < count; j++)
        {
            db_columns_t *entry2 = &db_entry[j];

            if ( i == j ) {
                continue;
            }
            if ( entry2->isupload ) {
                continue;
            }
            if ( check_is_same_file(entry1, entry2) ) {
                exist = 1;
                break;
            }
        }
        if ( !exist ) {
            print_new_upload(entry1);
            found += 1;
        }
    }
    return found;
}

// ------------------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    database_t  database;

    memset( &database, 0, sizeof(database_t));
    database.data = db_data;

    strncpy(check.upload,   UPLOAD,   NAMESIZE);
    strncpy(check.unsorted, UNSORTED, NAMESIZE);

    const char *startDir = argc > 1 ? argv[1] : "."; // Default to current directory
    printf("Scanning directory: %s\n", startDir);

    int count = scanDirectoryTree( &database, startDir );
    printf("Files (all):      %d\n", count);

    int found = mark_upload_unsorted( &database, &check );
    printf("Files (up&un):    %d\n", found);

    int uploads = find_new_uploads( &database );
    printf("Files (uploads):  %d\n", uploads);

    print_db(db_data, count);

    return 0;
}

#else

#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>
#include <sys/stat.h>
#include <time.h>

/*
// Callback function for nftw()
int print_file_info(const char *path, const struct stat *statbuf, int typeflag, struct FTW *ftwbuf) {
    if (typeflag == FTW_F) { // Check if it's a regular file
        printf("File: %s, Size: %ld bytes\n", path, statbuf->st_size);
    }
    return 0; // Continue traversal
}
*/

// Callback function for nftw
int display_info(const char *path, const struct stat *statbuf, int typeflag, struct FTW *ftwbuf)
{
    if (typeflag == FTW_F) // Only process regular files
    {
        printf("File: %s\n", path);
        printf("Size: %ld bytes\n", statbuf->st_size);

        // Convert and print the last modification time
        char timebuf[64];
        struct tm *timeinfo = localtime(&statbuf->st_mtime);
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", timeinfo);
        printf("Last Modified: %s\n\n", timebuf);
    }
    return 0; // Continue traversal
}



int main(int argc, char *argv[]) {
    const char *start_dir = (argc > 1) ? argv[1] : "."; // Start directory (default: current directory)

    // Traverse the directory tree
    if (nftw(start_dir, display_info, 20, FTW_PHYS) == -1) {
        perror("nftw");
        exit(EXIT_FAILURE);
    }

    return 0;
}

#endif
