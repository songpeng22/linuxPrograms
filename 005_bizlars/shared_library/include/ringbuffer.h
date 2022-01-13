#pragma once
#include <mutex>
using namespace std;

template <typename T>
class RingBuffer
{
public:
	RingBuffer(unsigned long len)
	{
		size = len;
		dataArray = new T[size];

		Clear();
	}

	~RingBuffer()
	{
		if (dataArray != NULL)
		{
			delete[] dataArray;
			dataArray = NULL;
		}
	}

	void Clear()
	{
		m_mutex.lock();

		for (unsigned long idx = 0; idx < size; idx++)
			dataArray[idx] = 0;

		inIndex = 0;
		outIndex = 0;
		level = 0;

		m_mutex.unlock();
	}

	unsigned long GetLevel()
	{
		unsigned long tmpLevel;

		m_mutex.lock();

		tmpLevel = level;

		m_mutex.unlock();

		return tmpLevel;
	}


	bool SetData(T *pData, unsigned long len)
	{
		m_mutex.lock();

		// check size
		if (len > size - level)
			return false;

		// input data
		for (unsigned long idx = 0; idx < len; idx++)
		{
			dataArray[inIndex++] = pData[idx];
			if (inIndex == size) inIndex = 0;
			level++;
		}

		m_mutex.unlock();

		return true;
	}


	unsigned long GetData(T *pData, unsigned long len)
	{
		unsigned long idx = 0;

		m_mutex.lock();

		while (level && len)
		{
			pData[idx++] = dataArray[outIndex++];
			if (outIndex == size) outIndex = 0;
			level--;
			len--;
		}

		m_mutex.unlock();

		return idx;
	}

private:

	T				*dataArray;
	unsigned long	inIndex;
	unsigned long	outIndex;
	unsigned long	level;
	unsigned long	size;

	mutex           m_mutex;
};

