//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2017
// European Synchrotron Radiation Facility
// CS40220 38043 Grenoble Cedex 9 
// FRANCE
//
// Contact: lima@esrf.fr
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################

#ifndef CTSAVING_COMPRESSION_H
#define CTSAVING_COMPRESSION_H

#include "lima/CtSaving.h"
#include "processlib/SinkTask.h"

namespace lima {

struct _BufferHelper
{
DEB_CLASS_NAMESPC(DebModControl,"_BufferHelper","Control");

void _init(int buffer_size);
public:
static const int BUFFER_HELPER_SIZE;
_BufferHelper();
_BufferHelper(int buffer_size);
~_BufferHelper();
int used_size;
void *buffer;
};
typedef std::vector<_BufferHelper*> ZBufferType;
typedef std::map<int,ZBufferType*> dataId2ZBufferType;

#ifdef WITH_Z_COMPRESSION 
#include <zlib.h>

#define TEST_AVAIL_OUT	if(!m_compression_struct.avail_out) \
    {									\
     _BufferHelper *newBuffer = new _BufferHelper(); \
      m_compression_struct.next_out = (Bytef*)newBuffer->buffer;	\
      m_compression_struct.avail_out = newBuffer->BUFFER_HELPER_SIZE;	\
      return_buffers->push_back(newBuffer);				\
    }

   class ZCompression: public SinkTaskBase
  {
    DEB_CLASS_NAMESPC(DebModControl,"Z Compression Task","Control");

    CtSaving::SaveContainer& 	m_container;
    int 			m_frame_per_file;
    CtSaving::HeaderMap 	m_header;
    
    z_stream_s		m_compression_struct;
    bool                          m_no_header;
    int                              m_compression_level;
  public:
    ZCompression(CtSaving::SaveContainer &save_cnt,
		 int framesPerFile,const CtSaving::HeaderMap &header);
    ZCompression(CtSaving::SaveContainer &save_cnt,
		 int level);
    
    ~ZCompression();
    virtual void process(Data &aData);
    void _compression(const char *buffer,int size,ZBufferType* return_buffers);
    void _end_compression(ZBufferType* return_buffers);
  };
#endif // WITH_Z_COMPRESSION

#ifdef WITH_LZ4_COMPRESSION
#include <lz4frame.h>

static const int LZ4_HEADER_SIZE = 19;
static const int LZ4_FOOTER_SIZE = 4;

static const LZ4F_preferences_t lz4_preferences = {
  { LZ4F_max256KB, LZ4F_blockLinked, LZ4F_noContentChecksum, LZ4F_frame, 0, { 0, 0 } },
  0,   /* compression level */
  1,   /* autoflush */
  { 0, 0, 0, 0 },  /* reserved, must be set to 0 */
};

 class Lz4Compression: public SinkTaskBase
 {
   DEB_CLASS_NAMESPC(DebModControl,"Lz4 Compression Task","Control");
   
   CtSaving::SaveContainer&		m_container;
   int				m_frame_per_file;
   CtSaving::HeaderMap		m_header;
   LZ4F_compressionContext_t	m_ctx;
 public:
   Lz4Compression(CtSaving::SaveContainer &save_cnt,
		  int framesPerFile,const CtSaving::HeaderMap &header);
   ~Lz4Compression();
   virtual void process(Data &aData);
   void _compression(const char *src,int size,ZBufferType* return_buffers);   
 };
#endif // WITH_LZ4_COMPRESSION

#ifdef WITH_BS_COMPRESSION
#include "bshuf_h5filter.h"

class BsCompression: public SinkTaskBase
{
}
#endif // WITH_BS_COMPRESSION 
};
#endif // CTSAVING_COMPRESSION_H
