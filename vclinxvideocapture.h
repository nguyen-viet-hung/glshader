#ifndef VCLINXVIDEOCAPTURE_H
#define VCLINXVIDEOCAPTURE_H

#include <QObject>
#include <vector>
#include <QString>
#include <stdint.h>
#include <thread>
#include <linux/videodev2.h>
#include "datasink.h"

enum IOMethod {
    IO_METHOD_READ,
    IO_METHOD_MMAP
};

typedef struct {
    QString devName;
    enum IOMethod readMethod;
    uint32_t fourcc;
    uint32_t bestWidth;
    uint32_t bestHeight;
    uint32_t framerateNum;
    uint32_t framerateDen;
}DeviceCaps;

typedef std::vector<DeviceCaps*> QListDevices;

class VCLinxVideoCapture : public QObject
{
    Q_OBJECT
protected:
    QListDevices mListDevices;
    int mOpenedDevice;
    std::thread* mMainThreadHandle;
    volatile bool mRunningFlag;
    std::vector<void*> mBufferRequested;
    uint32_t mBufferLength;
    unsigned mOpenedDeviceIdx;

    static void mainThread(void* opaque);
public:
    VCLinxVideoCapture(QObject *parent = 0);
    virtual ~VCLinxVideoCapture();

    unsigned numberDevices();
    const QListDevices& getDevices() const;

    bool openDevice(unsigned idx);
    void closeDevice();

signals:

    void sendCapturedImg(VideoImage* img);

public slots:
};

#endif // VCLINXVIDEOCAPTURE_H
