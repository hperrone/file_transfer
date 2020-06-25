//////////////////////////////////////////////////////////////////////////////
//
// Released under MIT License
// Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
//////////////////////////////////////////////////////////////////////////////

#ifndef FT_FILE_FILEMETA_H
#define FT_FILE_FILEMETA_H

#include <filesystem>
#include <vector>

#include "ft_utils.hpp"
#include "protocol/ft_msg.hpp"

namespace ft { namespace file {

FT_DECLARE_CLASS(FileMetadata)

/// @brief File metadata used on server side
///
/// Provides abstraction for a metadata file used in the server side for storing
/// information on the files being transferred.
///
/// This class is core at providing the ability to resume file transfers after
/// a connection is interrupted or any of the process is terminated.
///
/// The layout of the file is
///   - file_length: 8 bytes
///   - chunk_size:  8 bytes
///   - file_hash:   64 bytes
///   - chunk_bitmap: variable length (1 bit per chunk)
///
/// Each bit in the chunk_bitmap represents a chunk and is set to 1 if the chunk
/// have been already saved into the target file.
///
/// The file name of the metadata file is: ".file_name.meta" where file_name is
/// the name of the file being transferred. i.e.: for a file named 'image.jpg',
/// the metafile file name will be '.image.jpg.meta'.
///
/// File is never fully load into memory, all the changes are done directly to
/// the file, so if the process is terminated while receiving a file, the
/// information on received chunks is up to date.
class FileMetadata {
private:
 	const std::filesystem::path file_effective_path;
	const size_t                file_size;
	const size_t                file_chunk_size;
	const std::vector<uint8_t>  file_hash;
	const size_t                file_n_chunks;
	const size_t                header_size;
	const size_t                bitmap_size;
	std::filesystem::path       metadata_file;

public:
	/// Constructor
	FileMetadata(const std::filesystem::path& file_effective_path,
			size_t file_size, size_t file_chunk_size,
			std::vector<uint8_t> file_hash);

	virtual ~FileMetadata() {}

	/// @brief If the metadata file not exists, creates it
	///
	/// The metadata file is initialized with the target file length, chunk size
	/// and all the bits in the chunk_bitmap set to 0.
	void createIfNotExist();

	/// @brief Sets on/off the bit in the chunk_bitmap for the chunk index
	///
	/// Sets the bit to 1 if valid is true, 0 otherwise.
	void markChunk(size_t idx, bool valid);

	/// @brief Get the first chunk index that is not marked as saved
	///
	/// Search in the chunk_bitmap for the first bit set to 0 after the chunk
	/// index specified by from_chunk_idx.
	size_t nextMissingChunk(size_t from_chunk_idx) const;

	/// @brief Read the header of the metadata file.
	static void readHeader(const std::filesystem::path& file_effective_path,
			size_t& file_size, size_t& file_chunk_size,
			std::vector<uint8_t>& file_hash);
};

} // file
} // ft
#endif //FT_FILE_FILEMETA_H