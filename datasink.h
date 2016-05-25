#ifndef MEDIA_PROCESSING_DATA_SINK_H__
#define MEDIA_PROCESSING_DATA_SINK_H__

#include <QObject>
#include <stdint.h>
#include <string>

typedef struct {
    unsigned char* imgBuffer;
    uint32_t fourcc;
    uint32_t width;
    uint32_t height;
}VideoImage;


typedef struct DataFrame
{
	unsigned char* mBuffer;
	unsigned long  mSize;
	unsigned long  mSizeInBuff;
	int64_t        mTimestamp;
	unsigned long  mReserved1;
	unsigned long  mReserved2;

	bool WriteTo(const unsigned char* data, unsigned long size,
		int64_t timestamp, unsigned long reserved1, unsigned long reserved2);

	bool ReadOut(unsigned char* data, unsigned long &size, int64_t& timestamp,
		unsigned long &reserved1, unsigned long &reserved2);

}DataFrame;

class DataSink : public QObject
{
    Q_OBJECT
protected:
	typedef enum {
		CIRCULAR_QUEUE_MID,
		CIRCULAR_QUEUE_EMPTY,
		CIRCULAR_QUEUE_FULL
	} cq_level_t;

	cq_level_t mState;

    uint32_t mRear;
    uint32_t mFront;
    uint32_t mMaxFrames;

	DataFrame* mBuffer;
	bool mIsLost;
	std::string mName;
public:

    explicit DataSink(const char* name, unsigned int maxFrames, unsigned int frameSize, QObject *parent = 0);
    virtual ~DataSink();

	bool PushBackData(const unsigned char* data, unsigned long size,
		int64_t timestamp, unsigned long reserved1, unsigned long reserved2);

	bool PopUpData(unsigned char* data, unsigned long &size, int64_t& timestamp,
		unsigned long &reserved1, unsigned long &reserved2);

	void PopUpNoCopy();

//	void SetLost(bool flag) { mIsLost = flag; }
//	const char* GetSinkName() { return mName.c_str(); }
};

#endif//MEDIA_PROCESSING_DATA_SINK_H__
