#pragma once
#define PNG_DEBUG 3
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <png.h>

struct Vec2i {
	int x;
	int y;
};

void WriteImage(const char* file_name, uint8_t* data, int w, int h);
void WriteImageRGBA(const char* file_name, uint8_t* data, int w, int h);
void WriteImageRows(const char* file_name, png_bytep* rows, int w, int h);
Vec2i GetBufferImageDimensions(const uint8_t* compressed_data);
Vec2i GetImageDimensions(const char* file_name);
png_byte GetBufferColorType(const uint8_t* compressed_data);
png_byte GetColorType(const char* file_name);
void ReadBufferImage(const uint8_t* compressed_data, uint8_t* data, bool rgba);
void ReadImage(const char* file_name, uint8_t* data, bool rgba);

struct ImageStream {
	const uint8_t* compressedData;
	std::size_t offset;
};

void ReadDataFromInputStream(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead) {
	png_voidp io_ptr = png_get_io_ptr(png_ptr);
	ImageStream& inputStream = *(ImageStream*)io_ptr;
	memcpy(outBytes, inputStream.compressedData + inputStream.offset, byteCountToRead);
	inputStream.offset += byteCountToRead;
}

void WriteImage(const char* file_name, uint8_t* data, int w, int h) {
	png_bytep* rows = (png_bytep*)malloc(sizeof(void*) * h);
	for (int y = 0; y < h; y++) {
		rows[y] = data + 3 * y * w;
	}

	FILE* fp = fopen(file_name, "wb");

	png_structp png_ptr;
	png_infop info_ptr;

	png_byte color_type = PNG_COLOR_TYPE_RGB;
	png_byte bit_depth = 8;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);

	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, w, h, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
		PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, rows);
	png_write_end(png_ptr, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);
	free(rows);
}
void WriteImageRGBA(const char* file_name, uint8_t* data, int w, int h) {
	png_bytep* rows = (png_bytep*)malloc(sizeof(void*) * h);
	for (int y = 0; y < h; y++) {
		rows[y] = data + 4 * y * w;
	}

	FILE* fp = fopen(file_name, "wb");

	png_structp png_ptr;
	png_infop info_ptr;

	png_byte color_type = PNG_COLOR_TYPE_RGBA;
	png_byte bit_depth = 8;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);

	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, w, h, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
		PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, rows);
	png_write_end(png_ptr, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);
	free(rows);
}
void WriteImageRows(const char* file_name, png_bytep* rows, int w, int h) {
	FILE* fp = fopen(file_name, "wb");

	png_structp png_ptr;
	png_infop info_ptr;

	png_byte color_type = PNG_COLOR_TYPE_RGBA;
	png_byte bit_depth = 8;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);

	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, w, h, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
		PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, rows);
	png_write_end(png_ptr, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);
	for (int i = 0; i < h; i++)
		free(rows[i]);
	free(rows);
}

Vec2i GetBufferImageDimensions(const uint8_t* compressed_data) {
	ImageStream stream = {compressed_data, 8};

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);

	png_set_read_fn(png_ptr, &stream, ReadDataFromInputStream);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	int w = png_get_image_width(png_ptr, info_ptr);
	int h = png_get_image_height(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	return {w, h};
}
Vec2i GetImageDimensions(const char* file_name) {
	char header[8];

	FILE* fp = fopen(file_name, "rb");
	fread(header, 1, 8, fp);

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	int w = png_get_image_width(png_ptr, info_ptr);
	int h = png_get_image_height(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	fclose(fp);

	return {w, h};
}

png_byte GetBufferColorType(const uint8_t* compressed_data) {
	ImageStream stream = {compressed_data, 8};

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	png_infop info_ptr = png_create_info_struct(png_ptr);

	png_set_read_fn(png_ptr, &stream, ReadDataFromInputStream);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	png_byte color_type = png_get_color_type(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	return color_type;
}
png_byte GetColorType(const char* file_name) {
	char header[8];

	FILE* fp = fopen(file_name, "rb");
	fread(header, 1, 8, fp);

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	png_byte color_type = png_get_color_type(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	fclose(fp);
	return color_type;
}

void ReadBufferImage(const uint8_t* compressed_data, uint8_t* data, bool rgba) {
	ImageStream stream = {compressed_data, 8};

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);

	png_set_read_fn(png_ptr, &stream, ReadDataFromInputStream);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	int w = png_get_image_width(png_ptr, info_ptr);
	int h = png_get_image_height(png_ptr, info_ptr);
	png_byte color_type = png_get_color_type(png_ptr, info_ptr);
	png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	int number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);

	png_bytepp rows = (png_bytepp)malloc(sizeof(png_bytep) * h);
	if (color_type == PNG_COLOR_TYPE_RGB) {
		if (rgba) {
			for (int y = 0; y < h; y++) {
				rows[y] = (png_bytep)malloc(3 * w);
			}
			png_read_image(png_ptr, rows);
			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					memcpy(data + 4 * (y * w + x), rows[y] + 3 * x, 3);
					data[4 * (y * w + x) + 3] = 0xff;
				}
				free(rows[y]);
			}
		} else {
			for (int y = 0; y < h; y++) {
				rows[y] = data + 3 * y * w;
			}
			png_read_image(png_ptr, rows);
		}
	} else if (color_type == PNG_COLOR_TYPE_RGBA) {
		if (rgba) {
			for (int y = 0; y < h; y++) {
				rows[y] = data + 4 * y * w;
			}
			png_read_image(png_ptr, rows);
		} else {
			for (int y = 0; y < h; y++) {
				rows[y] = (png_bytep)malloc(4 * w);
			}
			png_read_image(png_ptr, rows);
			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					if (rows[y][4 * x + 3])
						memcpy(data + 3 * (y * w + x), rows[y] + 4 * x, 3);
					else
						memset(data + 3 * (y * w + x), 0, 3);
				}
				free(rows[y]);
			}
		}
	} else {
		fprintf(stderr, "Unrecognized PNG color type.\n");
	}

	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	free(rows);
}
void ReadImage(const char* file_name, uint8_t* data, bool rgba) {
	char header[8];

	FILE* fp = fopen(file_name, "rb");
	fread(header, 1, 8, fp);

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	int w = png_get_image_width(png_ptr, info_ptr);
	int h = png_get_image_height(png_ptr, info_ptr);

	png_byte color_type = png_get_color_type(png_ptr, info_ptr);
	png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	int number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);

	png_bytepp rows = (png_bytepp)malloc(sizeof(png_bytep) * h);
	if (color_type == PNG_COLOR_TYPE_RGB) {
		if (rgba) {
			for (int y = 0; y < h; y++) {
				rows[y] = (png_bytep)malloc(3 * w);
			}
			png_read_image(png_ptr, rows);
			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					memcpy(data + 4 * (y * w + x), rows[y] + 3 * x, 3);
					data[4 * (y * w + x) + 3] = 0xff;
				}
				free(rows[y]);
			}
		} else {
			for (int y = 0; y < h; y++) {
				rows[y] = data + 3 * y * w;
			}
			png_read_image(png_ptr, rows);
		}
	} else if (color_type == PNG_COLOR_TYPE_RGBA) {
		if (rgba) {
			for (int y = 0; y < h; y++) {
				rows[y] = data + 4 * y * w;
			}
			png_read_image(png_ptr, rows);
		} else {
			for (int y = 0; y < h; y++) {
				rows[y] = (png_bytep)malloc(4 * w);
			}
			png_read_image(png_ptr, rows);
			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					if (rows[y][4 * x + 3])
						memcpy(data + 3 * (y * w + x), rows[y] + 4 * x, 3);
					else
						memset(data + 3 * (y * w + x), 0, 3);
				}
				free(rows[y]);
			}
		}
	} else {
		fprintf(stderr, "Unrecognized PNG color type.\n");
	}

	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	fclose(fp);
	free(rows);
}
