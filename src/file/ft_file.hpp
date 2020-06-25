//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#ifndef FT_FILE_FILE_H
#define FT_FILE_FILE_H

#include <filesystem>
#include <vector>

#include "ft_utils.hpp"
#include "protocol/ft_msg.hpp"

namespace ft { namespace file {

FT_DECLARE_CLASS(File)
FT_DECLARE_CLASS(FileChunk)

static const size_t CHUNK_SIZE = ft::proto::MAX_MSG_PAYLOAD_SIZE;

/// @brief Represents a file being transferred
///
/// Provides an abstraction of actual file in the filesystem as well as it holds
/// additional information such as the hash and its size.
///
/// This is an abstract class. Instances must be obtained by using the static
/// methods make*File().
///
/// Files on their origins (usually the client doing the upload) are local files
/// and they must be instantiated using makeLocalFile().
///
/// Files on their destinations (usually the server doing the download) are
/// RemoteFiles, and they must be instantiated using makeRemoteFile(). Two
/// overloaded versions of this method. The shortest one is used to load
/// information from the metadata file. The second, on reception of an FILE
/// OFFER message, when the file is still not created in the filesystem.
///
/// On the upload directory, a folder is created for each client_uuid and
/// uploaded files are placed on such directories. This is to avoid file name
/// collisions among different clients uploading different files with the same
/// name.
///
class File : public virtual std::enable_shared_from_this<File> {
private:
	static std::filesystem::path sm_path_prefix;

public:
	const std::filesystem::path path;
	const std::vector<uint8_t>  hash;
	const size_t                size;

public:
	static void setLocalPathPrefix(const std::filesystem::path& path_prefix);

	static FilePtr makeLocalFile(const std::filesystem::path& path);

	static FilePtr makeRemoteFile(const std::filesystem::path& path,
			const std::vector<uint8_t>& hash, const size_t& size);

	static FilePtr makeRemoteFile(const std::filesystem::path& path);

protected:
	File(const std::filesystem::path& path, const std::vector<uint8_t>& hash,
			const size_t size)
	: path(path), hash(hash), size(size) {}

public:
	virtual ~File() {}

	virtual bool isComplete() const = 0;

	virtual FileChunkPtr getChunk(size_t chunk_idx) const = 0;

	virtual void saveChunk(const FileChunkPtr chunk) = 0;

	virtual size_t getNextMissingChunk(size_t from_chunk_idx = 0) const = 0;

	size_t getNumOfChunks() const {
		return size / CHUNK_SIZE + ( size % CHUNK_SIZE > 0 ? 1 : 0);
	}
};

/// @brief A segment of data within a File
///
/// Holds the data of a segment of the file, including its index, data and hash.
///
/// This is mostly a data object.
class FileChunk {
public:
	const FileWPtr             file;
	const size_t               idx;
	const std::vector<uint8_t> data;
	const std::vector<uint8_t> hash;

public:
	FileChunk(const FilePtr file, const size_t idx,
		const std::vector<uint8_t>& data, const std::vector<uint8_t>& hash)
	: file(file), idx(idx), data(data), hash(hash) {}

	virtual ~FileChunk() {}
};

} // file
} // ft
#endif //FT_FILE_FILE_H