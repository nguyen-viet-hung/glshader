#include "datasink.h"
#include <QDebug>

bool DataFrame::WriteTo(const unsigned char* data, unsigned long size,
	int64_t timestamp, unsigned long reserved1, unsigned long reserved2)
{
	if (size > mSize)
	{
        qDebug() << "DataFrame got some error. Size to push back is some great = " << size << " bytes";
		return false;
	}

	if(!mBuffer)
		return false;

	memcpy(mBuffer, data, size);

	mTimestamp = timestamp;
	mReserved1 = reserved1;
	mReserved2 = reserved2;

	mSizeInBuff = size;

	return true;
}

bool DataFrame::ReadOut(unsigned char* data, unsigned long &size, int64_t& timestamp,
	unsigned long &reserved1, unsigned long &reserved2)
{
	if (!mBuffer || !mSizeInBuff)
		return false;

	unsigned long size_copied = (size >= mSizeInBuff) ? mSizeInBuff : size;
	memcpy(data, mBuffer, size_copied);

	size = size_copied;
	timestamp = mTimestamp;
	reserved1 = mReserved1;
	reserved2 = mReserved2;

	return true;
}

DataSink::DataSink(const char* name, unsigned int maxFrames, unsigned int frameSize, QObject *parent)
    : QObject(parent)
    , mState(CIRCULAR_QUEUE_EMPTY)
    , mRear(0)
    , mFront(0)
    , mMaxFrames(maxFrames)
    , mName(name)
{
	mBuffer = new DataFrame[maxFrames];
	if (mBuffer)
	{
		for (unsigned int i = 0; i < maxFrames; i++)
		{
			mBuffer[i].mBuffer = new unsigned char[frameSize];
			mBuffer[i].mSize = frameSize;
		}
	}
}


DataSink::~DataSink()
{
    for (uint32_t i = 0; i < mMaxFrames; i++)
	{
		delete []mBuffer[i].mBuffer;
	}

	delete []mBuffer;
}


bool DataSink::PushBackData(const unsigned char* data, unsigned long size,
	int64_t timestamp, unsigned long reserved1, unsigned long reserved2)
{
	if (mState == CIRCULAR_QUEUE_FULL)
		return false;
	
	DataFrame* writeObj = mBuffer + mRear;
	if (!writeObj->WriteTo(data, size, timestamp, reserved1, reserved2))
		return false;

	mRear = (mRear + 1) % mMaxFrames;
	if (mRear == mFront)
		mState = CIRCULAR_QUEUE_FULL;
	else
		mState = CIRCULAR_QUEUE_MID;

	return true;
}

bool DataSink::PopUpData(unsigned char* data, unsigned long &size, int64_t& timestamp,
	unsigned long &reserved1, unsigned long &reserved2)
{
	if (mState == CIRCULAR_QUEUE_EMPTY)
		return false;

	DataFrame* readObj = mBuffer + mFront;
	if (!readObj->ReadOut(data, size, timestamp, reserved1, reserved2))
		return false;

	mFront = (mFront + 1) % mMaxFrames;
	if (mFront == mRear)
		mState = CIRCULAR_QUEUE_EMPTY;
	else
		mState = CIRCULAR_QUEUE_MID;

	return true;
}

void DataSink::PopUpNoCopy()
{
	if (mState == CIRCULAR_QUEUE_EMPTY)
		return;

	mFront = (mFront + 1) % mMaxFrames;
	if (mFront == mRear)
		mState = CIRCULAR_QUEUE_EMPTY;
	else
		mState = CIRCULAR_QUEUE_MID;
}
