#include "wav.h"
#include "memory.h"

RawSound* wav_load(const char* filename) {
	assert(filename);

	FileHandle wav_file = file_open(filename);

	uint chunk_id = file_read_uint32(wav_file);
	if(chunk_id != ('R'+('I'<<8)+('F'<<16)+('F'<<24)))
		LOG_ERROR("Bad file type");
	file_read_uint32(wav_file);
	uint wave_id = file_read_uint32(wav_file);
	if(wave_id != ('W'+('A'<<8)+('V'<<16)+('E'<<24)))
		LOG_ERROR("Bad wave id");

	// Format chunk
	uint format_id = file_read_uint32(wav_file);
	if(format_id != ('f'+('m'<<8)+('t'<<16)+(' '<<24)))
		LOG_ERROR("Not format chunk");
	uint format_length __attribute__ ((unused)) = file_read_uint32(wav_file);
	assert(format_length == 0x10);

	uint16 format_code __attribute__ ((unused)) = file_read_uint16(wav_file);
	assert(format_code == 0x01);

	uint16 n_channels = file_read_uint16(wav_file);
	assert(n_channels == 1 || n_channels == 2);
	uint32 sample_rate = file_read_uint32(wav_file);
	file_read_uint32(wav_file);
	file_read_uint16(wav_file);
	uint16 bits_per_sample = file_read_uint16(wav_file);
	assert(bits_per_sample == 8 || bits_per_sample == 16);

	// Data chunk
	// https://projects.developer.nokia.com/svn/matchempoker/tags/v1.1.0/qt_build/src/GEAudioBuffer.cpp
	uint32 data_id = 1635017060;
    uint32 current_field = file_read_uint32(wav_file);
    while(current_field != data_id) // Skip till "data" is found
        current_field = file_read_uint32(wav_file);

    uint data_size = file_read_uint32(wav_file);

	if(data_size < 1)
        LOG_ERROR("Data id is not \'data\'");
	void* data = MEM_ALLOC(data_size);
	file_read(wav_file, data, data_size);

	// Fill sound struct
	RawSound* result = (RawSound*)MEM_ALLOC(sizeof(RawSound));
	result->frequency = sample_rate;
	result->bits = bits_per_sample;
	result->channels = n_channels;
	result->size = data_size;
	result->data = data;

	// If 16 bits per sample and we're on big-endian cpu, swap bytes
	uint16 endian_test = 0x0011;
	if(bits_per_sample == 16 && endian_test != endian_swap2(endian_test)) {
		for(size_t i = 0; i < data_size; i += 2) {
			uint16* sample = (uint16*)(result->data + i);
			*sample = endian_swap2(*sample);
		}
	}

	return result;
}

void wav_free(RawSound* sound) {
	assert(sound);
	assert(sound->data);

	MEM_FREE(sound->data);
	sound->data = NULL;
	MEM_FREE(sound);
}

