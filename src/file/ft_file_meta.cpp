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

#include "file/ft_file.hpp"
#include "file/ft_file_meta.hpp"

namespace ft { namespace file {

FileMetadata::FileMetadata(const std::filesystem::path& file_effective_path,
		size_t file_size, size_t file_chunk_size, std::vector<uint8_t> file_hash)
: file_effective_path(file_effective_path)
, file_size(file_size)
, file_hash(file_hash)
, file_chunk_size(file_chunk_size)
, file_n_chunks(file_size / file_chunk_size + ((file_size % file_chunk_size) > 0 ? 1 : 0))
, header_size(sizeof(this->file_size) + sizeof(this->file_chunk_size) +
		proto::HASH_SIZE)
, bitmap_size((file_n_chunks / 8) + ((file_n_chunks % 8) > 0 ? 1 : 0))
{
	this->metadata_file = file_effective_path.parent_path();
	this->metadata_file /= std::string(".") +
			file_effective_path.filename().generic_string() + ".meta";
}

void FileMetadata::createIfNotExist()
{
	// Create directories if they not exists
	std::filesystem::create_directories(this->file_effective_path.parent_path());

	if (!std::filesystem::exists(this->metadata_file)) {
		// -- Write the header -- //
		std::ofstream ms(this->metadata_file , std::ios::out | std::ios::binary);
		ms.unsetf(std::ios::skipws);

		ms.write((char*)&this->file_size,       sizeof(this->file_size));
		ms.write((char*)&this->file_chunk_size, sizeof(this->file_chunk_size));
		ms.write((char*)this->file_hash.data(), proto::HASH_SIZE);

		// Then write the bitmap (1 bit per chunk in the file)
		for (int i = 0; i < this->bitmap_size; i++) {
			uint8_t b = 0;
			ms.write((char*)&b, 1);
		}

		ms.close();
	}
}

void FileMetadata::markChunk(size_t chunk_idx, bool valid)
{
	std::fstream ms(this->metadata_file , std::ios::in | std::ios::out |
			std::ios::binary);
	ms.unsetf(std::ios::skipws);

	// Read the byte containing the bit
	ms.seekg(this->header_size + chunk_idx / 8, std::ifstream::beg);
	uint8_t bitmap = 0;
	ms.read((char*)&bitmap, 1);

	// Update the read byte setting the bit
	uint8_t bit = (uint8_t)(1 << (7 - (chunk_idx % 8)));
	if (valid) {
		bitmap |= bit;
	} else {
		bitmap &= ~bit;
	}

	// write the byte to the file (in the same position is was read)
	ms.seekp(this->header_size + chunk_idx / 8, std::ifstream::beg);
	ms.write((char*)&bitmap, 1);
	ms.close();
}

size_t FileMetadata::nextMissingChunk(size_t from_chunk_idx) const
{
	std::ifstream ms(this->metadata_file , std::ios::in | std::ios::binary);
	ms.unsetf(std::ios::skipws);
	ms.seekg(this->header_size + from_chunk_idx / 8, std::ifstream::beg);

	// Skip all bytes set to 0xFF (all bits set)
	uint8_t bitmap = 0xFF;
	while(ms && bitmap == 0xFF) {
		ms.read((char*)&bitmap, 1);
	}

	size_t ret = UINT64_MAX;
	if (bitmap != 0xFF) {
		// Calculate the chunk index based on the position within the file
		ret = ((int)ms.tellg() - this->header_size - 1) * 8;
		// plus the bit number within the byte
		for (int i = 7; i >= 0 && ((bitmap >> i) & 0x01) == 0x01; i--, ret++);
		// This is to handle case when the last chunk is set
		ret = (ret < this->file_n_chunks) ? ret : UINT64_MAX;
	}

	ms.close();
	return ret;
}

void FileMetadata::readHeader(const std::filesystem::path& file_effective_path,
		size_t& file_size, size_t& file_chunk_size,
		std::vector<uint8_t>& file_hash)
{
	auto metadata_file = file_effective_path.parent_path();
	metadata_file /= std::string(".") +
			file_effective_path.filename().generic_string() + ".meta";

	file_size = 0;
	file_chunk_size = 0;
	file_hash.resize(proto::HASH_SIZE);
	if (std::filesystem::exists(metadata_file)) {
		std::ifstream ms(metadata_file , std::ios::in | std::ios::binary);
		ms.unsetf(std::ios::skipws);

		ms.read((char*)&file_size, sizeof(file_size));
		ms.read((char*)&file_chunk_size, sizeof(file_chunk_size));
		ms.read((char*)file_hash.data(), proto::HASH_SIZE);
		ms.close();
	}
}

} // file
} // ft