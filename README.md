.TH P3_Report

.SH Name
P1 \- File Systems

.SH Synopsis
The goal of this project was to implement a library that can read and write WAD files, and mount it to a terminal using a FUSE daemon, effectively creating a rudimentary file storage system.

.SH Description
This project can be broken down into two halves: the Wad class library and the wadfs daemon program. All of the source code for these programs can be found in their respective directories, "libWad/" and "wadfs/". The files used to create the library include "/libWad/Wad.h", "/libWad/Wad.cpp", and a Makefile to compile them. The files used to create the daemon include just "/wadfs/wadfs.cpp" and a Makefile to compile it and link it to the library.

The way that WAD files and their contents are stored within memory is through an n-ary tree of Descriptor objects. A Descriptor struct is declared within the Wad class definition to act as a container for all of the information stored within WAD descriptors, such as offset, length, name, whether it is a directory or file, as well as a vector of descriptors pointing to its children and a pointer to its parent descriptor. This allows for easy tracking of the directory structure, since it mimicks how directory hierarchies are organized within the WAD files. To aid in traversal there is also an unordered map containing strings as keys and pointers to descriptors as values. Each key-value pair represents a path that leads to a given element. Now to provide a brief overview of the library's functionality, here are the definitions for all of the library methods required by the project guidelines:

\fBloadWad()\fR \- This is a public function that calls the private constructor to generate a new Wad object and returns a pointer to it when given a path to read from. The private constructor will then read the header of the WAD file the path leads to and construct an n-ary tree representing the directory structure, as well as a map representing all of the path-element pairs in the structure.

\fBgetMagic()\fR \- This function simply returns the magic, or WAD file type, of the currently loaded WAD file.

\fBisContent()\fR \- Returns if the element a given path leads to is a file.

\fBisDirectory()\fR \- Returns if the element a given path leads to is a directory.

\fBgetSize()\fR \- If the element a given path leads to is a content file, returns its size, otherwise returns -1.

\fBgetContents()\fR \- If the element a given path leads to is a content file, fills a buffer that is passed by reference with its data, effectively reading it, and returns the number of bytes read, otherwise returns -1.

\fBgetDirectory()\fR \- If the element a given path leads to is a directory, fills a vector that is passed by reference with names of the elements immediately contained within that directory, and returns the number of elements, otherwise returns -1.

\fBcreateDirectory()\fR \- Creates a namespace directory at the location specified by a path and writes it to the WAD file.

\fBcreateFile()\fR \- Creates a content file at the location specified by a path and writes it to the WAD file.

\fBwriteToFile()\fR \- Takes a buffer passed by reference, and writes the data it contains to an empty file specified by a given path. Returns the number of bytes written if successful, otherwise returns -1.

There are also a multitude of helper functions, but here are the 3 most important ones:

\fBfindDescriptor()\fR \- Takes in a path and uses the path map to return a pointer to the resulting element.

\fBaddToPathMap()\fR \- Takes in a path and a pointer to a descriptor element to be added to the path map.

\fBupdateWadFile()\fR \- Writes any new directories or files to the WAD file.

The wadfs daemon, which is the other half of this project, utilizes the FUSE library to allow this file system to be mounted to a directory. The main() call reads in 3 command line arguments: a flag, a WAD file to be read/written to, and a directory to be mounted. It then loads a Wad object for the specified file path and calls the fuse_main library call, which is what initiates the mounting process. In calling this, it also passes in a list of callback operations the daemon can perform and a void pointer to the loaded Wad object. Here is a brief overview of the callback operations and their respective functions:

\getattr = wadfs_getattr()\fR \- Determines if an object is a file or directory, and assigns their respective permissions and sizes. (isContent(), isDirectory(), and getSize())

\mknod = wadfs_mknod()\fR \- Creates a file at a given path location. (createFile())

\mkdir = wadfs_mkdir()\fR \- Creates a directory at a given path location. (createDirectory())

\open = wadfs_open()\fR \- Simply allows only file objects to be opened. (isContent())

\read = wadfs_read()\fR \- Takes a buffer passed by reference and retreives the data of a file. (getContents())

\write = wadfs_write()\fR \- Takes a buffer passed by reference and writes the data to a file. (writeToFile())

\readdir = wadfs_readdir()\fR \- Fills the display directory with the objects on the current directory level. (getDirectory())

As one may notice, these callback operations all closely match what the library functions already do, so a lot of them are only a couple lines of code that simply call the library methods through the Wad object that was passed into fuse_main through the void pointer earlier.

.SH Testing
The Wad library was tested mainly using the wad_dump.cpp and libtest.cpp programs. For the first portion of this half of the project, I was only working on the reading of WAD files, so I was just comparing my outputs to the given dump text files. Once the readability was implemented, I began testing with the libtest.cpp program, which was much more extensive. 

For testing the wadfs daemon, I simply followed along with Ernesto's demonstration of the daemon and ensured mine could accomplish all of the same tasks. The daemon was a lot simpler than the actual Wad library, since the library did most of the heavy-lifting, so there was significantly less testing that needed to be done since the library already worked.

.SH Bugs
The only bug that I am aware of was revealed to me in test case 35 of the libtest.cpp program: The Big Test. Whenever multiple files of the same name are written to different directories it will occasionally cause the file to be duplicated.

.SH Link
    Video Recording \- https://youtu.be/jav_rssuvsY

.SH References/Citations

    Reading DOOM WAD Files \- https://www.cyotek.com/blog/reading-doom-wad-files

    Fuse Tutorial \- https://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/html/

    Write a filysystem with FUSE \- https://engineering.facile.it/blog/eng/write-filesystem-fuse/

    Writing Less Simple, Yet Stupid Filesystem Using FUSE in C \- https://maastaar.net/fuse/linux/filesystem/c/2019/09/28/writing-less-simple-yet-stupid-filesystem-using-FUSE-in-C/

    CS137 FUSE Documentation \- https://www.cs.hmc.edu/~geoff/classes/hmc.cs137.201601/homework/fuse/fuse_doc.html

    Ernesto Mujica's Discussion Video

    COP4600 Official Discord Server

.SH Author
Matthew Grimaldi