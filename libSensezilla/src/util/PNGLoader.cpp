
#include "all.h"
#include "png.h"




PNGData PNGLoader::loadFromPNG(string fname) {
	PNGData retval;
	retval.A = retval.B = retval.G = retval.R = retval.GRAY = NULL;
	FILE * fin = fopen(fname.c_str(), "rb");
	if ( !fin ) {
		log_e("Error: Cannot open file %s",fname.c_str());
		return retval;
	}
	png_byte header[8];
	fread(header, 1, 8, fin);
	if ( png_sig_cmp(header,0,8) ) {
		fclose(fin);
		log_e("Error: %s is not a PNG file",fname.c_str());
		return retval;
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if ( !png_ptr ) {
		fclose(fin);
		log_e("Error: Cannot allocate png_ptr"); return retval;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if ( !info_ptr ) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(fin);
		log_e("Error: Cannot allocate info_ptr"); return retval;
	}

	png_infop end_info = png_create_info_struct(png_ptr);
	if ( !end_info ) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(fin);
		log_e("Error: Cannot allocate end_info"); return retval;
	}

	if ( setjmp(png_jmpbuf(png_ptr)) ) {
		log_e("Error reading PNG file");
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		fclose(fin);
		return retval;
	}

	png_init_io(png_ptr, fin);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr,info_ptr);

	int height = png_get_image_height(png_ptr,info_ptr);
	int width = png_get_image_width(png_ptr,info_ptr);
	int channels = png_get_channels(png_ptr,info_ptr);

	png_bytepp row_pointers = (png_bytepp)png_malloc(png_ptr,height*sizeof(png_bytep));
	for ( int i = 0; i < height; i++ )
		row_pointers[i] = NULL;
	for ( int i = 0; i < height; i++ ) 
		row_pointers[i] = (png_bytep)png_malloc(png_ptr,width*channels);

	if ( png_get_bit_depth(png_ptr,info_ptr) == 16)
		png_set_strip_16(png_ptr);
	if ( png_get_bit_depth(png_ptr,info_ptr) < 8 )
		png_set_packing(png_ptr);

	png_read_image(png_ptr, row_pointers);

	png_read_end(png_ptr, end_info);

	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	fclose(fin);
	
	png_byte color_type = png_get_color_type(png_ptr,info_ptr);

	
	log_i("Loaded %dx%d PNG image",width,height);
	retval.width = width;
	retval.height = height;
	if ( color_type & PNG_COLOR_MASK_ALPHA ) {
		retval.A = allocate2D(width,height);
		for ( int r = 0; r < height; r++ ) {
			for ( int c = 0; c < width; c++ ) {
				retval.A[r][c] = row_pointers[r][c*channels + (channels - 1)] / 255.0;
			}
		}
	}
	if ( color_type & PNG_COLOR_MASK_COLOR ) {
		retval.R = allocate2D(width,height);
		retval.G = allocate2D(width,height);
		retval.B = allocate2D(width,height);

		for ( int r = 0; r < height; r++ ) {
			for ( int c = 0; c < width; c++ ) {
				retval.R[r][c] = row_pointers[r][c*channels] / 255.0;
				retval.G[r][c] = row_pointers[r][c*channels + 1] / 255.0;
				retval.B[r][c] = row_pointers[r][c*channels + 2] / 255.0;
			}
		}
	}
	else {
		retval.GRAY = allocate2D(width,height);

		for ( int r = 0; r < height; r++ ) {
			for ( int c = 0; c < width; c++ ) {
				retval.GRAY[r][c] = row_pointers[r][c*channels] / 255.0;
			}
		}
	}

	for ( int i = 0; i < height; i++ ) {
		png_free(png_ptr, row_pointers[i]);
	}
	png_free(png_ptr,row_pointers);

	return retval;
}


void PNGLoader::writeToPNG(string fname, PNGData data){
	int width,height;
	png_byte color_type = 0;
	int channels = 0;
	if ( data.A ) {
		channels++;
		color_type |= PNG_COLOR_MASK_ALPHA;
	}
	width = data.width;
	height = data.height;

	if ( data.R && data.G && data.B ) {
		color_type |= PNG_COLOR_MASK_COLOR;
		channels += 3;
	} else if ( data.GRAY ) {
		channels ++;
	} else {
		log_e("No color or grayscale data available for writing?");
		return;
	}

	png_bytepp row_pointers = new png_bytep[height];
	for ( int r = 0; r < height; r++ ) {
		row_pointers[r] = new png_byte[width*channels];
		for ( int c = 0; c < width; c++ ) {
			if ( color_type & PNG_COLOR_MASK_COLOR ) {
				row_pointers[r][channels*c]		= (png_byte)(255 * data.R[r][c]);
				row_pointers[r][channels*c + 1] = (png_byte)(255 * data.G[r][c]);
				row_pointers[r][channels*c + 2] = (png_byte)(255 * data.B[r][c]);
			} else {
				row_pointers[r][channels*c]		= (png_byte)(255 * data.GRAY[r][c]);
			}

			if ( color_type & PNG_COLOR_MASK_ALPHA ) {
				row_pointers[r][channels*c + channels - 1] = (png_byte)(255 * data.GRAY[r][c]);
			}
		}
	}



	FILE * fp = fopen(fname.c_str(), "wb");
	if ( !fp) {
		log_e("Cannot open file %s",fname.c_str());
		goto error1;
	}

	png_structp png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if ( !png_ptr ) {
		log_e("Cannot allocate png_ptr");
		goto error2;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if ( !info_ptr) {
		log_e("Cannot allocate info_ptr");
		goto error3;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		log_e("Error during png write");
		goto error4;
	}

	png_init_io(png_ptr, fp);


	png_set_IHDR(png_ptr,info_ptr,width,height,8,color_type,PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);

	png_set_rows(png_ptr,info_ptr,row_pointers);

	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

error4:
	png_destroy_write_struct(&png_ptr,&info_ptr);
	goto error2;
error3:
	png_destroy_write_struct(&png_ptr,NULL);
error2:
	fclose(fp);
error1:
	for ( int r = 0; r < height; r++ ) {
		delete [] row_pointers[r];
	}
	delete [] row_pointers;
}

void PNGLoader::freePNGData(PNGData * data) { 
	free2D(data->A,data->height);
	free2D(data->R,data->height);
	free2D(data->G,data->height);
	free2D(data->B,data->height);
	free2D(data->GRAY,data->height);
	data->height = data->width = 0;
	data->A = data->R = data->G = data->B = data->GRAY = NULL;
}

void PNGLoader::free2D(double ** dat, int height) {
	if ( dat ) {
		for ( int c = 0 ; c < height; c++ ) {
			delete dat[c];
		}
		delete dat;
	}
}

double ** PNGLoader::allocate2D(int width, int height) {
	double ** retval;
	retval = new double*[height];
	for ( int c = 0; c < height; c++ ) {
		retval[c] = new double[width];
	}
	return retval;
}