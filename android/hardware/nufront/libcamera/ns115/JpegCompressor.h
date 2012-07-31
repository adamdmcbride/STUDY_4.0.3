/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HW_EMULATOR_CAMERA_JPEG_COMPRESSOR_H
#define HW_EMULATOR_CAMERA_JPEG_COMPRESSOR_H

/*
 * Contains implementation of a class NV21JpegCompressor that encapsulates a
 * converter between NV21, and JPEG formats.
 * Contains implementation of a class YuYvJpegCompressor that encapsulates a
 * converter between YUV422i, and JPEG formats.
 */

namespace android {

enum {
        JPG_INPUT_FMT_NV21 = 0,
        JPG_INPUT_FMT_NV12,
        JPG_INPUT_FMT_YUYV,
        JPG_INPUT_FMT_YVYU,
};

///////////////////////////////////////////////////////////////////////////
//
// Abstract Image compressor, must be implemented
//
///////////////////////////////////////////////////////////////////////////
class JpegCompressor{
     public:
	   
          int compressRawImage(const void* image,
                              int width,
                              int height,
                              int quality,
                              int useEffect);

        JpegCompressor(){}
	 virtual ~JpegCompressor(){
               if (NULL != mJpegBuffer)
              {
                  delete [] mJpegBuffer;
                  mJpegBuffer = NULL;
              }
         
	 } 

   /* Get size of the compressed JPEG buffer.
     * This method must be called only after a successful completion of
     * compressRawImage call.
     * Return:
     *  Size of the compressed JPEG buffer.
     */
    size_t getCompressedSize() const
    {
        return mJpegSize;
    }

    /* Copies out compressed JPEG buffer.
     * This method must be called only after a successful completion of
     * compressRawImage call.
     * Param:
     *  buff - Buffer where to copy the JPEG. Must be large enough to contain the
     *      entire image.
     */
    void getCompressedImage(void* buff) const
    {
        memcpy(buff, mJpegBuffer, mJpegSize);
    }
   
     
   virtual void getYuvOffsets(int width, int height, uint8_t * pimg, uint8_t ** pu, uint8_t ** pv) = 0;
    /****************************************************************************
     * Class data
     ***************************************************************************/

protected:
    
    int      mInputFmt;
    int      mStrides[3];
    int      mPlaneNum;
    bool    mIs420;
    uint8_t *mJpegBuffer;
    size_t mJpegSize;
};


///////////////////////////////////////////////////////////////////////////
//
// NV21/NV12 Image compressor
//
///////////////////////////////////////////////////////////////////////////
class YUV420spJpegCompressor : public JpegCompressor{
	public:
		
		/* Constructs JpegCompressor instance. */
		YUV420spJpegCompressor(int inputFmt);
		/* Destructs JpegCompressor instance. */
	virtual	~YUV420spJpegCompressor(){}

             /*
               * get offsets of Y, U, V 
	        */
        virtual	 void getYuvOffsets(int width, int height, uint8_t * pimg, uint8_t ** pu, uint8_t ** pv);
 };


///////////////////////////////////////////////////////////////////////////
//
// YUYV/YVYU Image compressor
//
///////////////////////////////////////////////////////////////////////////
class YUV422IJpegCompressor : public JpegCompressor{
	public:
		
		/* Constructs JpegCompressor instance. */
		YUV422IJpegCompressor(int inputFmt);
		/* Destructs JpegCompressor instance. */
	virtual	~YUV422IJpegCompressor(){}

		/*
               * get offsets of Y, U, V 
	        */
	virtual	void getYuvOffsets(int width, int height, uint8_t * pimg, uint8_t ** pu, uint8_t ** pv);
 };


}; /* namespace android */

#endif  /* HW_EMULATOR_CAMERA_JPEG_COMPRESSOR_H */
