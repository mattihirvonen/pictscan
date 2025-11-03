
Picture Library Sync
====================
This project goal is to create simple command line application
to list files which are not yet "archived" properly into
photo library directory tree. Application compares only matching
- file names and
- file sizes

Why not time stamp? Reason is simple. Copying files between
Windows 98, Windows NT (XP, ...,Win11), Linux machines or
FTP transfer modify file's directory time stamp different odd ways.

Photo directory structure example

 (.)--+--- 2025_Narrowboat ---+--- subdir1
      |                       +--- subdir2
      |
      +--- 2024_Holiday ------+--- subdir1
      |                       +--- subdir2
      |
      +--- unsorted --+--- subdir1
      |               +--- subdir2
      |
      +--- backup ----+--- subdir1
      |               +--- subdir2
      |
      +--- upload ----+--- phone1
                      +--- phone2
                      +--- Canon
                      +--- Nikon
Strategy
--------
1. Photos will upload somewhere into subdirectory tree "upload"
2. Application will scan all subdirectory trees to find new
   uploaded files which are not yet copied or moved to some one
   other subdirectory trees like "2025_Narrowboat", "2024_Holiday"

Command line arguments

  - i [path]  Ignore directory for file check
  - g [path]  "gallery directory" for file scan (default is ".")
  - u [path]  original files "upload" directory tree
  - U [path]  "unsorted" directory tree
  - d         enable debug print option
  - v         enable verbose mode (multiple use increase verbosity level)

Command line example

  pictscan.exe  -i backup  -u upload  -U unsorted  [gallery_direcory_tree]


Future ToDo(s)
--------------
- compare EXIF or simple CRC for files where name and size match
- Separate upload directory outside this directory structure
  (like external USB stick or network drive)

