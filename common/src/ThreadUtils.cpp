#include "ThreadUtils.h"
#include <errno.h>

using namespace lima;

MutexAttr::MutexAttr(Type type)
{
	if (pthread_mutexattr_init(&m_mutex_attr) != 0)
		throw LIMA_COM_EXC(Error, "Error initializing mutex attr");

	try {
		setType(type);
	} catch (...) {
		destroy();
		throw;
	}
}

MutexAttr::MutexAttr(const MutexAttr& mutex_attr)
	: m_mutex_attr(mutex_attr.m_mutex_attr)
{
}

MutexAttr::~MutexAttr()
{
	destroy();
}

void MutexAttr::setType(Type type)
{
	int kind;
	switch (type) {
	case Normal: 
		kind = PTHREAD_MUTEX_NORMAL;
		break;
	case Recursive:
		kind = PTHREAD_MUTEX_RECURSIVE;
		break;
	case ErrorCheck:
		kind = PTHREAD_MUTEX_ERRORCHECK;
		break;
	default:
		throw LIMA_COM_EXC(InvalidValue, "Invalid MutexAttr type");
	}

	if (pthread_mutexattr_settype(&m_mutex_attr, kind) != 0)
		throw LIMA_COM_EXC(Error, "Error setting mutex attr");
}

MutexAttr::Type MutexAttr::getType() const
{
	int kind;
	if (pthread_mutexattr_gettype(&m_mutex_attr, &kind) != 0)
		throw LIMA_COM_EXC(Error, "Error getting mutex attr");

	switch (kind) {
	case PTHREAD_MUTEX_NORMAL:
		return Normal;
	case PTHREAD_MUTEX_RECURSIVE:
		return Recursive;
	case PTHREAD_MUTEX_ERRORCHECK:
		return ErrorCheck;
	default:
		throw LIMA_COM_EXC(Error, "Invalid mutex attr kind");
	}
}

MutexAttr& MutexAttr::operator =(Type type)
{
	setType(type);
	return *this;
}

MutexAttr& MutexAttr::operator =(const MutexAttr& mutex_attr)
{
	m_mutex_attr = mutex_attr.m_mutex_attr;
	return *this;
}

void MutexAttr::destroy()
{
	pthread_mutexattr_destroy(&m_mutex_attr);
}


Mutex::Mutex(MutexAttr mutex_attr)
	: m_mutex_attr(mutex_attr)
{
	pthread_mutexattr_t& attr = m_mutex_attr.m_mutex_attr;
	if (pthread_mutex_init(&m_mutex, &attr) != 0)
		throw LIMA_COM_EXC(Error, "Error initializing mutex");
}

Mutex::~Mutex()
{
	pthread_mutex_destroy(&m_mutex);
}

void Mutex::lock()
{
	if (pthread_mutex_lock(&m_mutex) != 0)
		throw LIMA_COM_EXC(Error, "Error locking mutex");
}

void Mutex::unlock()
{
	if (pthread_mutex_unlock(&m_mutex) != 0)
		throw LIMA_COM_EXC(Error, "Error unlocking mutex");
}

bool Mutex::tryLock()
{
	
	switch (pthread_mutex_trylock(&m_mutex)) {
	case EBUSY:
		return false;
	case 0:
		return true;
	default:
		throw LIMA_COM_EXC(Error, "Error trying to lock mutex");
	}
}

MutexAttr Mutex::getAttr()
{
	return m_mutex_attr;
}


Cond::Cond()
{
	if (pthread_cond_init(&m_cond, NULL) != 0)
		throw LIMA_COM_EXC(Error, "Error initializing condition");
}

Cond::~Cond()
{
	pthread_cond_destroy(&m_cond);
}

void Cond::wait(Mutex& mutex)
{
	if (pthread_cond_wait(&m_cond, &mutex.m_mutex) != 0)
		throw LIMA_COM_EXC(Error, "Error waiting for condition");
}

void Cond::signal()
{
	if (pthread_cond_signal(&m_cond) != 0)
		throw LIMA_COM_EXC(Error, "Error signaling condition");
}


Thread::Thread()
{
	m_started = m_finished = false;
}

Thread::~Thread()
{
	if (m_started) {
		abort();
		pthread_join(m_thread, NULL);
	}
}

void Thread::start()
{
	if (m_started)
		throw LIMA_COM_EXC(Error, "Thread already started");

	if (pthread_create(&m_thread, NULL, staticThreadFunction, this) != 0)
		throw LIMA_HW_EXC(Error, "Error creating thread");

	m_started = true;
}

void Thread::abort()
{
	if (!m_started)
		throw LIMA_COM_EXC(Error, "Thread has not been started");

	if (!m_finished)
		pthread_cancel(m_thread);
}

bool Thread::hasStarted()
{
	return m_started;
}

bool Thread::hasFinished()
{
	return m_finished;
}

void *Thread::staticThreadFunction(void *data)
{
	Thread *thread = (Thread *) data;

	try {
		thread->threadFunction();
	} catch (...) {

	}
	
	thread->m_finished = true;
	return NULL;
}


CmdThread::AuxThread::AuxThread(CmdThread& master)
	: m_master(master)
{
}

CmdThread::AuxThread::~AuxThread()
{
}

void CmdThread::AuxThread::threadFunction()
{
	m_master.cmdLoop();
}

CmdThread::CmdThread()
	: m_thread(*this)
{
	m_status = InInit;
	m_cmd = None;
}

CmdThread::~CmdThread()
{
	abort();
	waitStatus(Finished);
}

AutoMutex CmdThread::lock()
{
	return AutoMutex(m_mutex, AutoMutex::Locked);
}

AutoMutex CmdThread::tryLock()
{
	return AutoMutex(m_mutex, AutoMutex::TryLocked);
}

int CmdThread::getStatus()
{
	AutoMutex l = lock();
	return m_status;
}

void CmdThread::setStatus(int status)
{
	AutoMutex l = lock();
	m_status = status;
	m_cond.signal();
}

void CmdThread::waitStatus(int status)
{
	AutoMutex l = lock();
	while (m_status != status)
		m_cond.wait(l.mutex());
}

int CmdThread::waitNotStatus(int status)
{
	AutoMutex l = lock();
	while (m_status == status)
		m_cond.wait(l.mutex());
	return m_status;
}

void CmdThread::sendCmd(int cmd)
{
	AutoMutex l = lock();
	m_cmd = cmd;
	m_cond.signal();
}

void CmdThread::start()
{
	if (m_thread.hasStarted())
		throw LIMA_COM_EXC(InvalidValue, "Thread already started");

	m_cmd = Init;
	m_thread.start();
}

void CmdThread::abort()
{
	if (getStatus() != Finished)
		sendCmd(Abort);
}

int CmdThread::waitNextCmd()
{
	AutoMutex l = lock();

	while (m_cmd == None)
		m_cond.wait(l.mutex());

	int cmd = m_cmd;
	m_cmd = None;
	return cmd;
}

void CmdThread::cmdLoop()
{
	while (getStatus() != Finished) {
		int cmd = waitNextCmd();
		switch (cmd) {
		case None:
			throw LIMA_COM_EXC(InvalidValue, "Invalid None cmd");
		case Init:
			init();
			break;
		case Stop:
			setStatus(Stopped);
			break;
		case Abort:
			setStatus(Finished);
			break;
		default:
			execCmd(cmd);
		}
	}
}
