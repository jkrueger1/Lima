#ifndef HWBUFFERMGR_H
#define HWBUFFERMGR_H

#include "HwFrameCallback.h"
#include "SizeUtils.h"

#include <vector>

namespace lima
{

/*******************************************************************
 * \class BufferAllocMgr
 * \brief Abstract class defining interface for buffer allocation
 *
 * Specifies the basic functionality for allocating frame buffers
 *******************************************************************/

class BufferAllocMgr
{
 public:
	virtual ~BufferAllocMgr();

	virtual int getMaxNbBuffers(const FrameDim& frame_dim) = 0;
	virtual void allocBuffers(int nb_buffers, 
				  const FrameDim& frame_dim) = 0;
	virtual const FrameDim& getFrameDim() = 0;
	virtual void getNbBuffers(int& nb_buffers) = 0;
	virtual void releaseBuffers() = 0;

	virtual void *getBufferPtr(int buffer_nb) = 0;

	virtual void clearBuffer(int buffer_nb);
	virtual void clearAllBuffers();
};


/*******************************************************************
 * \class SoftBufferAllocMgr
 * \brief Simple full software implementation of BufferAllocMgr
 *
 * This classes uses new and delete to allocate the memory buffers
 *******************************************************************/

class SoftBufferAllocMgr : public BufferAllocMgr
{
 public:
	SoftBufferAllocMgr();
	virtual ~SoftBufferAllocMgr();

	virtual int getMaxNbBuffers(const FrameDim& frame_dim);
	virtual void allocBuffers(int nb_buffers, 
				  const FrameDim& frame_dim);
	virtual const FrameDim& getFrameDim();
	virtual void getNbBuffers(int& nb_buffers);
	virtual void releaseBuffers();

	virtual void *getBufferPtr(int buffer_nb);
	
 private:
	typedef std::vector<char *> BufferList;
	typedef BufferList::const_iterator BufferListCIt;

	FrameDim m_frame_dim;
	BufferList m_buffer_list;
};


/*******************************************************************
 * \class BufferCbMgr
 * \brief Abstract class with interface for buffer alloc. and callbacks
 *
 * Specifies the basic functionality for allocating frame buffers and
 * for managing the frame callbacks
 *******************************************************************/

class BufferCbMgr : public HwFrameCallbackGen
{
 public:
	enum Cap {
		Basic=0, Concat=1, Acc=2, // bit mask
	};

	virtual ~BufferCbMgr();

	virtual Cap getCap() = 0;

	virtual int getMaxNbBuffers(const FrameDim& frame_dim, 
				    int nb_concat_frames) = 0;
	virtual void allocBuffers(int nb_buffers, int nb_concat_frames, 
				  const FrameDim& frame_dim) = 0;
	virtual const FrameDim& getFrameDim() = 0;
	virtual void getNbBuffers(int& nb_buffers) = 0;
	virtual void getNbConcatFrames(int& nb_concat_frames) = 0;
	virtual void releaseBuffers() = 0;

	virtual void *getBufferPtr(int buffer_nb, int concat_frame_nb) = 0;

	virtual void clearBuffer(int buffer_nb);
	virtual void clearAllBuffers();

	virtual void setStartTimestamp(Timestamp  start_ts);
	virtual void getStartTimestamp(Timestamp& start_ts);

	virtual void getFrameInfo(int acq_frame_nb, HwFrameInfoType& info) = 0;

	virtual void getBufferFrameDim(const FrameDim& single_frame_dim,
				       int nb_concat_frames, 
				       FrameDim& buffer_frame_dim);
	virtual void acqFrameNb2BufferNb(int acq_frame_nb,int& buffer_nb,
					 int& concat_frame_nb);

 private:
	Timestamp m_start_ts;
};

BufferCbMgr::Cap operator |(BufferCbMgr::Cap c1, BufferCbMgr::Cap c2);
BufferCbMgr::Cap operator &(BufferCbMgr::Cap c1, BufferCbMgr::Cap c2);


/*******************************************************************
 * \class StdBufferCbMgr
 * \brief Class providing standard buffer cb functionality
 *
 * This class implements the normal buffer mode using a basic 
 * BufferAllocMgr
 *******************************************************************/

class StdBufferCbMgr : public BufferCbMgr
{
 public:
	StdBufferCbMgr(BufferAllocMgr& alloc_mgr);
	virtual ~StdBufferCbMgr();

	virtual Cap getCap();

	virtual int getMaxNbBuffers(const FrameDim& frame_dim, 
				    int nb_concat_frames);
	virtual void allocBuffers(int nb_buffers, int nb_concat_frames, 
				  const FrameDim& frame_dim);
	virtual const FrameDim& getFrameDim();
	virtual void getNbBuffers(int& nb_buffers);
	virtual void getNbConcatFrames(int& nb_concat_frames);
	virtual void releaseBuffers();

	virtual void *getBufferPtr(int buffer_nb, int concat_frame_nb);

	virtual void clearBuffer(int buffer_nb);
	virtual void clearAllBuffers();

	virtual void getFrameInfo(int acq_frame_nb, HwFrameInfoType& info);

	bool newFrameReady(HwFrameInfoType& frame_info);

 protected:
	virtual void setFrameCallbackActive(bool cb_active);
	
 private:
	typedef std::vector<HwFrameInfoType> FrameInfoList;

	BufferAllocMgr& m_alloc_mgr;
	FrameDim m_frame_dim;			  
	int m_nb_concat_frames;
	FrameInfoList m_info_list;
	bool m_fcb_act;
};


/*******************************************************************
 * \class BufferCtrlMgr
 * \brief Class providing full buffer functionality for a given hardware
 *
 * This class implements all the buffer functionality required by the
 * hardware buffer interface. It can use different kinds of acq. buffer
 * managers and complement their missing functionality.
 *******************************************************************/

class BufferCtrlMgr : public HwFrameCallbackGen
{
 public:
	enum AcqMode {
		Normal, Concat, Acc,
	};

	BufferCtrlMgr(BufferCbMgr& acq_buffer_mgr);
	~BufferCtrlMgr();

	void setFrameDim(const FrameDim& frame_dim);
	void getFrameDim(      FrameDim& frame_dim);

	void setNbConcatFrames(int  nb_concat_frames);
	void getNbConcatFrames(int& nb_concat_frames);

	void setNbAccFrames(int  nb_acc_frames);
	void getNbAccFrames(int& nb_acc_frames);

	void setNbBuffers(int  nb_buffers);
	void getNbBuffers(int& nb_buffers);

	void getMaxNbBuffers(int& max_nb_buffers);

	void *getBufferPtr(int buffer_nb, int concat_frame_nb = 0);
	void *getFramePtr(int acq_frame_nb);

	void setStartTimestamp(Timestamp  start_ts);
	void getStartTimestamp(Timestamp& start_ts);

	void getFrameInfo(int acq_frame_nb, HwFrameInfoType& info);

	BufferCbMgr& getAcqBufferMgr();
	AcqMode getAcqMode();

 protected:
	virtual void setFrameCallbackActive(bool cb_active);
	
 private:
	class AcqFrameCallback : public HwFrameCallback
	{
	public:
		AcqFrameCallback(BufferCtrlMgr& buffer_mgr)
			: m_buffer_mgr(buffer_mgr) {}
	protected:
		virtual bool newFrameReady(const HwFrameInfoType& frame_info)
		{
			return m_buffer_mgr.acqFrameReady(frame_info);
		}
	private:
		BufferCtrlMgr& m_buffer_mgr;
	};
	friend class AcqFrameCallback;

	void releaseBuffers();
	bool acqFrameReady(const HwFrameInfoType& acq_frame_info);
	void accFrame(void *src_ptr, const FrameDim& src_frame_dim,
		      void *dst_ptr, const FrameDim& dst_frame_dim,
		      int& valid_pixels);

	int m_nb_concat_frames;
	int m_nb_acc_frames;
	BufferCbMgr& m_acq_buffer_mgr;
	SoftBufferAllocMgr m_aux_alloc_mgr;
	StdBufferCbMgr m_aux_buffer_mgr;
	BufferCbMgr *m_effect_buffer_mgr;
	FrameDim m_frame_dim;
	AcqFrameCallback m_frame_cb;
	bool m_frame_cb_act;
};




} // namespace lima

#endif // HWBUFFERMGR_H
