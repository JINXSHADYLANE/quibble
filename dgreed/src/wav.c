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
	uint format_length = file_read_uint32(wav_file);
	assert(format_length == 0x10);
	format_length = 0; // To prevent unused warning
	uint16 format_code = file_read_uint16(wav_file);
	assert(format_code == 0x01);
	format_code = 0; // To prevent unused warning
	uint16 n_channels = file_read_uint16(wav_file);
	assert(n_channels == 1 || n_channels == 2);
	uint32 sample_rate = file_read_uint32(wav_file);
	file_read_uint32(wav_file);
	file_read_uint16(wav_file);
	uint16 bits_per_sample = file_read_uint16(wav_file);
	assert(bits_per_sample == 8 || bits_per_sample == 16);

	// Data chunk
	uint data_id = file_read_uint32(wav_file);
	if(data_id != ('d'+('a'<<8)+('t'<<16)+('a'<<24)))
		LOG_ERROR("Bad data id");
	uint data_size = file_read_uint32(wav_file);	
	void* data = MEM_ALLOC(data_size);
	file_read(wav_file, data, data_size);

	// Fill sound struct
	RawSound* result = (RawSound*)MEM_ALLOC(sizeof(RawSound));
	result->frequency = sample_rate;
	result->bits = bits_per_sample;
	result->channels = n_channels;
	result->size = data_size;
	result->data = data;

	return result;
}	

void wav_free(RawSound* sound) {
	assert(sound);
	assert(sound->data);

	MEM_FREE(sound->data);
	sound->data = NULL;
	MEM_FREE(sound);
}	

