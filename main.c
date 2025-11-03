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
#include <unistd.h>     // getopt()

#define  PATHSIZE  256
#define  NAMESIZE  128
#define  EXTSIZE   8
#define  DB_SIZE   100000     // DB rows
#define  GALLERY   "."
#define  UPLOAD    "./upload"
#define  UNSORTED  "./unsorted"
#define  IGNORE    "./backup"


typedef struct
{
    int   debug;
    int   verbose;
} options_t;


typedef struct
{
    char  gallery[PATHSIZE];    // Photo gallery directory tree
    char  upload[PATHSIZE];     // Photo "import" directory (called also upload or download)
    char  unsorted[PATHSIZE];   // "unsorted" directory tree
    char  ignore[PATHSIZE];     // Directory path to ignore files in new files check
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
    int      isignore;
} db_columns_t;


typedef struct
{
    int            count;     // Count of active database entries
    int            upload;    // Count of files in "upload" directory tree
    int            unsorted;  // Count of files in "unsorted" directory tree
    int            ignore;    // Count of non checked files
    db_columns_t  *data;
    db_columns_t  *head;
} database_t;


options_t     options;
db_columns_t  db_data[DB_SIZE];
check_t       check;


db_columns_t *db_next_entry( db_columns_t *db_entry )
{
    return db_entry->next;  // Simulate use of dynamically allocated memory
}


db_columns_t *db_new_entry( database_t *db )
{
    // Simulate dynamic memory allocation for new db_entry
    db_columns_t *db_entry;

    if ( !db->head ) {
        db_entry = db->data;
    }
    else {
        db_entry       = db->head;
        db_entry->next = db_entry + 1;    // Simulate memory allocation
        db_entry       = db_entry->next;
        db_entry->prev = db->head;
    }
    db->count += 1;
    db->head   = db_entry;
    //
    return  db_entry;
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


void db_update_entry( db_columns_t *db_entry, char *fullPath, struct stat *pathStat, int count )
{
    char path[256], baseName[256], extension[256];

    extractPathComponents(fullPath, path, baseName, extension);

    strncpy(db_entry->filePath,  path,      PATHSIZE);
    strncpy(db_entry->baseName,  baseName,  NAMESIZE);
    strncpy(db_entry->extension, extension, EXTSIZE);
    //
    db_entry->fileSize = pathStat->st_size;
    db_entry->ctime    = pathStat->st_ctime;
    db_entry->mtime    = pathStat->st_mtime;
    db_entry->atime    = pathStat->st_atime;
    //
    if ( options.debug )
    {
        printf("Count: %d\n", count);
        printf("File: %s  size=%ld\n", fullPath, pathStat->st_size);

        printf("Path: %s\n",      db_entry->filePath);
        printf("Base Name: %s\n", db_entry->baseName);
        printf("Extension: %s\n", db_entry->extension);
        printf("\n");
    }
}


// Return count of file infos added into data base
int scanDirectoryTree( database_t *db, const char *dirPath)
{
    struct dirent *entry;
    DIR *dp = opendir(dirPath);

    if (dp == NULL) {
        char  errtxt[256];
        snprintf(errtxt, sizeof(errtxt), "opendir( %s )", dirPath);
        perror(errtxt);
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
                if (!options.debug && (options.verbose > 1) ) {
                    printf("Directory: %s\n", fullPath);
                }
                // Recursively scan subdirectory
                db->count = scanDirectoryTree(db, fullPath);
            } else if (S_ISREG(pathStat.st_mode))
            {
                if ( db->count < DB_SIZE )
                {
                    db_columns_t *db_entry = db_new_entry( db );

                    db_update_entry( db_entry, fullPath, &pathStat, db->count );
                }
            }
        } else {
            perror("stat");
        }
    }
    closedir(dp);
    return   db->count;
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
    char line[128];

    snprintf(line, sizeof(line), "%9ld %12s.%-6s - %s", db_entry->fileSize, db_entry->baseName, db_entry->extension, db_entry->filePath);
    printf("New upload: %s\n", line);
}

// ------------------------------------------------------------------------------------------

int mark_upload_unsorted( database_t *db, check_t *check )
{
    db_columns_t *db_entry = db->data;
    int           count    = db->count;
    int           ignore   = strlen(check->ignore) > 1 ? 1 : 0;

    db->upload   = 0;
    db->unsorted = 0;

    while ( count--  > 0 )
    {
        if (ignore && (strstr(db_entry->filePath, check->ignore) == db_entry->filePath)) {
            db_entry->isignore = 1;
            db->ignore += 1;
        }
        if (strstr(db_entry->filePath, check->upload) == db_entry->filePath) {
            db_entry->isupload = 1;
            db->upload += 1;
        }
        if (strstr(db_entry->filePath, check->unsorted) == db_entry->filePath) {
            db_entry->isunsorted = 1;
            db->unsorted += 1;
        }
        db_entry = db_next_entry( db_entry );
    }
    return db->upload + db->unsorted + db->ignore;
}


// Compare: file size and (non case sensitive) file name
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


// ToDo: Refactor to be compatible with dynamic memory allocated database
int find_new_uploads( database_t *db )
{
    int found = 0;
    int count = db->count;

    for (int i = 0; i < count; i++)
    {
        db_columns_t *entry1 = &db->data[i];
        int           exist  = 0;

        if ( !entry1->isupload ) {
            continue;
        }
        if ( entry1->isignore ) {
            continue;
        }

        for (int j = 0; j < count; j++)
        {
            db_columns_t *entry2 = &db->data[j];

            if ( i == j ) {
                continue;
            }
            if ( entry2->isignore ) {
                continue;
            }
            if ( entry2->isupload ) {
                continue;
            }
            if ( check_is_same_file(entry1, entry2) ) {
                exist += 1;
                break;
            }
        }
        if ( !exist ) {
            print_new_upload(entry1);
            found += 1;
        }
        if ( options.verbose && (exist > 1) ) {
            printf("Multi match for: %s/%s.%s\n", entry1->filePath, entry1->baseName, entry1->extension);
        }
    }
    return found;
}

// ------------------------------------------------------------------------------------------

char *normalize_path( char *newpath, char *path, int size )
{
    // Absolute path begin with '/'
    if ( path[0] == '/') {  return path;  }

    // Relative path must begin with "./" or "../"
    if ( strstr(path, "./") == path ) { return path;  }
    if ( strstr(path,"../") == path ) { return path;  }

    // Adjust relative path
    strncpy( newpath, "./", size - 2);
    strncat( newpath, path, size - 2);
    printf("- standardize: <%s>\n", newpath);
    return   newpath;
}


int parse_options(int argc, char *argv[])
{
    char line[256];
    int  opt;

    // Define the options: "a" and "b:" (b requires an argument)
    while ((opt = getopt(argc, argv, "vg:u:U:i:d")) != -1)
    {
        switch (opt) {
            case 'd':
                options.debug +=1;   // Increase debug level
                break;
            case 'v':
                options.verbose +=1; // Increase verbose level
                break;
            case 'u':
                strncpy(check.upload,   normalize_path(line,optarg,sizeof(line)), sizeof(check.upload));
                break;
            case 'U':
                strncpy(check.unsorted, normalize_path(line,optarg,sizeof(line)), sizeof(check.unsorted));
                break;
            case 'g':
                strncpy(check.gallery,  normalize_path(line,optarg,sizeof(line)), sizeof(check.gallery));
                break;
            case 'i':
                strncpy(check.ignore,   normalize_path(line,optarg,sizeof(line)), sizeof(check.ignore));
                break;
            case '?':
                // Handle unknown options
                fprintf(stderr, "Unknown option: -%c\n", optopt);
                return -1;
                break;
        }
    }
    // Remaining arguments (non-option arguments)
    if (optind < argc) {
        printf("Non-option arguments:\n");
        for (int i = optind; i < argc; i++) {
            printf("  %s\n", argv[i]);
        }
    }
    return 0;
}


int main(int argc, char *argv[])
{
    options.debug = 0;

    database_t  database;

    // Set defaults:
    strncpy(check.upload,   UPLOAD,   sizeof(check.upload));
    strncpy(check.gallery,  GALLERY,  sizeof(check.gallery));
    strncpy(check.unsorted, UNSORTED, sizeof(check.unsorted));
    strncpy(check.ignore,   IGNORE,   sizeof(check.ignore));

    if ( options.verbose ) {
        printf("Scanning directory: %s\n", check.gallery);
    }

    if ( parse_options(argc, argv) ) {
        return -1;  // Exit: Bad command line argument
    }

    memset( &database, 0, sizeof(database_t));
    database.data = db_data;

    int count = scanDirectoryTree( &database, check.gallery );

    mark_upload_unsorted( &database, &check );

    int uploads = find_new_uploads( &database );

    if ( options.debug ) {
         print_db(db_data, count);
    }
    if ( options.verbose )
    {
         printf("\n");
         printf("Files (all):       %d\n", count);
         printf("Files (upload):    %d\n", database.upload);
         printf("Files (ignored):   %d\n", database.ignore);
         printf("Files (unsorted):  %d\n", database.unsorted);
         printf("Files (new):       %d\n", uploads);
    }
    return 0;
}

//================================================================================
#else

#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int opt;
    // Define the options: "a" and "b:" (b requires an argument)
    while ((opt = getopt(argc, argv, "ab:c")) != -1) {
        switch (opt) {
            case 'a':
                printf("Option -a was provided\n");
                break;
            case 'b':
                printf("Option -b was provided with argument: %s\n", optarg);
                break;
            case 'c':
                printf("Option -c was provided\n");
                break;
            case '?':
                // Handle unknown options
                fprintf(stderr, "Unknown option: -%c\n", optopt);
                break;
        }
    }

    // Remaining arguments (non-option arguments)
    if (optind < argc) {
        printf("Non-option arguments:\n");
        for (int i = optind; i < argc; i++) {
            printf("  %s\n", argv[i]);
        }
    }

    return 0;
}


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
