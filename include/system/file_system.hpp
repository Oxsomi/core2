#pragma once
#include "types/types.hpp"

namespace oic {

    //!The access flags of a file
    //READ: The file's data can be read
    //WRITE: The file's data can be written
    enum class FileAccess : u8 {
        READ = 0b01,
        WRITE = 0b10,
        READ_WRITE = 0b11
    };

    //!The queried info about a file
    //The layout of a recursive file structure:
    //folders, files, recursive
    struct FileInfo {

        //!The size type of the file system
        using SizeType = u32;

        //!The full path in oic representation
        String path{};

        //!The total size of the file
        //When the file is a folder, this is set to 0
        usz fileSize{};

        //!The last modification time of the file
        //When the file is virtual, this is set to 0
        time_t modificationTime{};

        //!Implementation specific data
        void *dataExt{};

        //!Implementation specific magic number (for identifying subtypes)
        SizeType magicNumber{};

        //!The parent's file id
        SizeType parent{};

        //!The start location of the folders
        SizeType folderHint{};

        //!The start location of the files (fileHint - folderHint = folderCount)
        SizeType fileHint{};

        //!The end location of the files (fileEnd - fileHint = fileCount)
        SizeType fileEnd{};

        //!The access flags of this file
        FileAccess access{};

        //!Whether this file has children
		//Doesn't indicate if there is data attached to this file
		//A folder can have data (for example an archive)
        bool isFolder{};

        //Helper functions

        usz getFolders() const;
        usz getFiles() const;
		usz getFileObjects() const;

		//!Determines if this file can be accessed with these flags
        bool hasAccess(FileAccess flags) const;

		//!Determines if this file is located in the virtual file system or the physical one
		bool isVirtual() const;

		//!Determines if this file or folder has data attached to it
		bool hasData() const;

    };

    //!A callback for handling file changes and loops
    using FileCallback = void (*)(class FileSystem*, const FileInfo&);

    //!A callback for handling file changes and loops
    using FileChangeCallback = void (*)(class FileSystem*, const FileInfo&, bool remove);

    //!The class responsible for handling file I/O
    //A file system can also be implemented for an archive as well as a native file system
    //Every file system supports virtual files, though local and global files aren't always guaranteed
    //~/ is the virtual file system (READ)
    //./ is the local or virtual file system (READ_WRITE)
    //../ and ./ get resolved to form the final path
    //\ is disallowed
    //This is called oic file notation
	//Each file system has to handle keeping the file system up to date
    class FileSystem {

    public:

        //Constructors

        FileSystem(bool allowLocalFiles);
        virtual ~FileSystem() = default;

        FileSystem(const FileSystem&) = delete;
        FileSystem &operator=(const FileSystem&) = delete;
        FileSystem(FileSystem&&) = delete;
        FileSystem &operator=(FileSystem&&) = delete;

        //!Called to add a file modification callback
        void addFileChangeCallback(FileChangeCallback);

        //!Called to remove a file modification callback
        void removeFileChangeCallback(FileChangeCallback);

        //!Get the properties of a file
        //@param[in] path The target file object with oic file notation
        //@return FileInfo &fileProperties
        //@warning Throws if the file doesn't exist
        FileInfo &get(const String &path);

        //!Get the properties of a file
        //@param[in] path The target file object with oic file notation
        //@return FileInfo &fileProperties
        //@warning Throws if the file doesn't exist
        const FileInfo &get(const String &path) const;

        //!Resolve the path to a normal file directory (without ../ and ./)
        //@param[in] path The target file object with oic file notation
        //@param[out] outPath
        //@return bool exists Whether the path leads to a valid path
        bool resolvePath(const String &path, String &outPath) const;

        //!Allows looping through the children of a folder
        bool foreachFile(const FileInfo &path, FileCallback callback, bool recurse);

		//!Detect if the path exists
		//@param[in] path The target file object with oic file notation
		//@return bool exists Whether the path leads to a valid path
		bool exists(const String &path) const;

        //!Called when a file has changed
        void notifyFileChange(const String &path, bool isRemoved);

        //!Read a (part of a) file into a buffer
        //@param[in] file The target file object
        //@param[out] buffer The output
        //@param[in] size The number of bytes to read (0 = all by default)
        //@param[in] offset The byte offset in the file
        //@return bool success
        virtual bool read(const FileInfo &file, Buffer &buffer, usz size = 0, usz offset = 0) const = 0;

		//!Read a (part of a) file into a buffer
		//@param[in] path The path in oic file notation
		//@param[out] buffer The output
		//@param[in] size The number of bytes to read (0 = all by default)
		//@param[in] offset The byte offset in the file
		//@return bool success
		inline bool read(const String &str, Buffer &buffer, usz size = 0, usz offset = 0) const {
			return read(get(str), buffer, size, offset);
		}

        //!Write to a (part of a) file
        //@param[in] file The target file object
        //@param[out] buffer The input
        //@param[in] size The number of bytes to write (0 = all by default)
        //@param[in] bufferOffset The byte offset in the buffer
        //@param[in] fileOffset The byte offset in the file (0 = clear, usz_MAX = append, otherwise it tries to write into the offset)
        //@return bool success
        virtual bool write(FileInfo &file, const Buffer &buffer, usz size = 0, usz bufferOffset = 0, usz fileOffset = 0) = 0;

        //!Write to a (part of a) file
		//@param[in] path The path in oic file notation
        //@param[out] buffer The input
        //@param[in] size The number of bytes to write (0 = all by default)
        //@param[in] bufferOffset The byte offset in the buffer
        //@param[in] fileOffset The byte offset in the file (0 = clear, usz_MAX = append, otherwise it tries to write into the offset)
        //@return bool success
		inline bool write(const String &file, const Buffer &buffer, usz size = 0, usz bufferOffset = 0, usz fileOffset = 0) {
			return write(get(file), buffer, size, bufferOffset, fileOffset);
		}

        //!Add a directory or file
        //@param[in] file The target file object
		//@param[in] isFolder If the file can have children
        //@return bool success
        bool add(const String &file, bool isFolder);

		//!Remove a directory or file
		//@param[in] file The target file object
        //@return bool success
		bool remove(FileInfo &file);

    protected:

		//!Creates a local folder
		virtual void mkdir(FileInfo &) {}

		//!Creates the look up tables by file path
		void resetLut();
    
        //!Called to initialize the file system cache
        virtual void initFiles() = 0;

		//!Used to handle file changes and update the metadata for the file
		virtual void onFileChange(FileInfo &, bool) {}

    private:

		//!Whether or not the local file system is utilized
		bool allowLocalFiles;

        //!File cache
        List<FileInfo> virtualFiles, localFiles;

		//!File id by path look up tables
		HashMap<String, FileInfo::SizeType> virtualFileLut, localFileLut;

        //!List of all file change callbacks
        List<FileChangeCallback> callbacks;

    };

}