//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>

// Using BLAKE2b from Crypto++ for calculating hashes
#include <cryptlib.h>
#include <blake2.h>

#include "file/ft_file_meta.hpp"
#include "file/ft_file.hpp"

namespace ft { namespace file {

/// @brief Local file in the client filesystem
///
/// The class FileLocal represents a local file in the client file system.
///
/// The key method within this class is getChunk(), used to retrieve the file
/// segments being transferred.
class FileLocal : virtual public File {

public:
	const std::filesystem::path effective_path;
public:
	FileLocal(const std::filesystem::path& path,
			const std::vector<uint8_t>& hash, const size_t size,
			const std::filesystem::path& effective_path);
public:
	virtual ~FileLocal() {}

	virtual bool isComplete() const { return true; };

	virtual FileChunkPtr getChunk(size_t chunk_idx) const;

	virtual void saveChunk(FileChunkPtr chunk) {
		throw std::domain_error("File chunks cannot be saved in local files");
	};

	virtual size_t getNextMissingChunk(size_t from_chunk_idx = 0) const {
		// Local file have all the chunks
		return UINT64_MAX;
	};

	// Only File::makeLocalFile should construct instances of FileLocal
	friend FilePtr File::makeLocalFile(const std::filesystem::path& in_file);
};



///////////////////////////////////////////////////////////////////////////////



/// @brief Remote file in the server filesystem
///
/// The class FileRemote represents a file in the server's file system.
class FileRemote : virtual public File {
	FileMetadata file_metadata;

public:
	const std::filesystem::path effective_path;
public:
	FileRemote(const std::filesystem::path& path,
			const std::vector<uint8_t>& hash,
			const size_t size,
			const std::filesystem::path& effective_path);

	virtual ~FileRemote() {}

	virtual bool isComplete() const;

	virtual FileChunkPtr getChunk(size_t chunk_idx) const {
		throw std::domain_error("File chunks cannot be retrieved from remote "
				"files");
	}

	virtual void saveChunk(const FileChunkPtr chunk);

	virtual size_t getNextMissingChunk(size_t from_chunk_idx = 0) const {
		return this->file_metadata.nextMissingChunk(from_chunk_idx);
	};

	FilePtr metadataFile_makeRemoteFile( const std::filesystem::path& path,
		const std::filesystem::path& effective_path);

};


/////////////////////////////////////////////////////////////////////////////
// Module static functions

static void calc_hash(const std::filesystem::path& file,
		std::vector<uint8_t>& digest_out);

static void calc_hash(std::ifstream& is, std::vector<uint8_t>& digest_out);

static void calc_hash(std::vector<uint8_t>& buf,
		std::vector<uint8_t>& digest_out);

////////////////////////////////////////////////////////////////////////////
// File class' members

std::filesystem::path File::sm_path_prefix = "";

void File::setLocalPathPrefix(const std::filesystem::path& path_prefix)
{
	File::sm_path_prefix = path_prefix;
}

FilePtr File::makeLocalFile(const std::filesystem::path& path)
{
	auto effective_path = File::sm_path_prefix;
	effective_path /= path;

	if (!std::filesystem::is_regular_file(effective_path) ||
			!std::filesystem::exists(effective_path)) {
		throw std::runtime_error(std::string("File does not exists or is not a "
				"regular file: ") + (std::string)effective_path);
	}

	std::vector<uint8_t> hash;
	calc_hash(effective_path, hash);
	return std::make_shared<FileLocal>(path, hash,
			std::filesystem::file_size(effective_path), effective_path);
}

FilePtr File::makeRemoteFile(const std::filesystem::path& path,
		const std::vector<uint8_t>& hash, const size_t& size)
{
	auto effective_path = File::sm_path_prefix;
	effective_path /= path;

	return std::make_shared<FileRemote>(path, hash, size, effective_path);
}

FilePtr File::makeRemoteFile(const std::filesystem::path& path)
{
	auto effective_path = File::sm_path_prefix;
	effective_path /= path;

	size_t file_size;
	size_t file_chunk_size;
	std::vector<uint8_t> file_hash;

	FilePtr ret;
	FileMetadata::readHeader(effective_path, file_size, file_chunk_size,
			file_hash);
	if (file_chunk_size > 0) {
		ret = std::make_shared<FileRemote>(path, file_hash, file_size,
				effective_path);
	}

	return ret;
}

////////////////////////////////////////////////////////////////////////////
// FileLocal class' members

FileLocal::FileLocal(const std::filesystem::path& path,
		const std::vector<uint8_t>& hash, const size_t size,
		const std::filesystem::path& effective_path)
: File(path, hash, size)
, effective_path(effective_path)
{
}

FileChunkPtr FileLocal::getChunk(size_t idx) const
{
	size_t offset = idx * CHUNK_SIZE;

	if (offset > this->size) {
		throw std::range_error("Chunk index outside file length range");
	}

	size_t chunk_size = (this->size - offset) < CHUNK_SIZE ?
			(this->size - offset) : CHUNK_SIZE;

	// Read the data chunk to a memory buffer
	std::vector<uint8_t> chunk_data(chunk_size);
	std::ifstream is(this->effective_path, std::ios::in | std::ios::binary);
	is.unsetf(std::ios::skipws);
	is.seekg(offset, std::ifstream::beg);
	is.read((char*)chunk_data.data(), chunk_data.size());
	is.close();

	// Calculate the hash
	std::vector<uint8_t> chunk_hash;
	calc_hash(chunk_data, chunk_hash);

	return std::make_shared<FileChunk>(
			std::const_pointer_cast<File>(shared_from_this()), idx, chunk_data,
			chunk_hash);
}

////////////////////////////////////////////////////////////////////////////
// FileRemote class' members

FileRemote::FileRemote(const std::filesystem::path& path,
			const std::vector<uint8_t>& hash, const size_t size,
			const std::filesystem::path& effective_path)
: File(path, hash, size)
, effective_path(effective_path)
, file_metadata(effective_path, size, CHUNK_SIZE, hash)
{
	std::filesystem::create_directories(this->effective_path.parent_path());
	file_metadata.createIfNotExist();

	if (!std::filesystem::exists(this->effective_path)) {
		// If file does not exists, create an empty file
		std::ofstream os(this->effective_path,
				std::ios::out | std::ios::binary);
		os.seekp(size - 1, std::ios::beg);
		uint8_t b = 0;
		os.write((char*)&b, 1);
		os.close();
	}
}

bool FileRemote::isComplete() const
{
	bool ret = false;
	if (std::filesystem::exists(this->effective_path)) {
		// Hash is computing intensive, so only calculate it if all the chunks
		// have been received.
		if (this->file_metadata.nextMissingChunk(0) == UINT64_MAX) {
			std::vector<uint8_t> local_hash;
			calc_hash(this->effective_path, local_hash);
			ret = memcmp(this->hash.data(), local_hash.data(),
					this->hash.size()) == 0;
		}
	}

	return ret;
}

void FileRemote::saveChunk(const FileChunkPtr chunk)
{
	//Check if the chunk index is valid according to the file being received
	size_t offset = chunk->idx * CHUNK_SIZE;
	if (offset > this->size) {
		std::cout << "Try to save chunk index outside file length range" <<
				std::endl;
		return;
	}

	//Check if the chunk size is valid according to the file being received
	size_t chunk_size = (this->size - offset) < CHUNK_SIZE ?
			(this->size - offset) : CHUNK_SIZE;

	if (chunk_size != chunk->data.size()) {
		std::cout << "Try to save chunk with invalid size" << std::endl;
		return;
	}

	// Open the file, seek to the beginning of the chunk and write it.
	std::fstream os(this->effective_path,
			std::ios::in | std::ios::out | std::ios::binary);
	os.unsetf(std::ios::skipws);
	os.seekp(offset, std::ifstream::beg);
	os.write((char*)(chunk->data.data()), chunk->data.size());
	os.close();

	file_metadata.markChunk(chunk->idx, true);
}

////////////////////////////////////////////////////////////////////////////
// Implementation of module's static functions

static void calc_hash(const std::filesystem::path& file,
		std::vector<uint8_t>& digest_out)
{
	std::ifstream is(file, std::ifstream::in | std::ifstream::binary);
	is.unsetf(std::ios::skipws);
	try {
		calc_hash(is, digest_out);
	} catch (std::exception any) {
		is.close();
		throw any;
	}
	is.close();
}

static void calc_hash(std::ifstream& is, std::vector<uint8_t>& digest_out)
{
	CryptoPP::BLAKE2b blake_hash((unsigned int)64);
	std::vector<uint8_t> buf(blake_hash.BlockSize());

	digest_out.clear();
	digest_out.resize(blake_hash.DigestSize());

	is.seekg(0, std::ifstream::beg);
	do {
		(void)memset(buf.data(), 0, buf.size());
		is.read((char*)buf.data(), buf.size());
		size_t n_read = is.gcount();
		blake_hash.Update(buf.data(), n_read);
	} while(is);

	blake_hash.TruncatedFinal(digest_out.data(), digest_out.size());
}

static void calc_hash(std::vector<uint8_t>& buf,
		std::vector<uint8_t>& digest_out)
{
	CryptoPP::BLAKE2b blake_hash((unsigned int)64);

	digest_out.clear();
	digest_out.resize(blake_hash.DigestSize());
	blake_hash.Update(buf.data(), buf.size());
	blake_hash.TruncatedFinal(digest_out.data(), digest_out.size());
}

} // file
} // ft