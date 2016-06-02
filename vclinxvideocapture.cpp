#include "vclinxvideocapture.h"
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <QDebug>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif

#define CLEAR(x) memset(&(x), 0, sizeof(x))

void VCLinxVideoCapture::mainThread(void *opaque)
{
    VCLinxVideoCapture* pThis = (VCLinxVideoCapture*)opaque;

    uint32_t fr = pThis->mListDevices.at(pThis->mOpenedDeviceIdx)->framerateNum / pThis->mListDevices.at(pThis->mOpenedDeviceIdx)->framerateDen;
    uint32_t width = pThis->mListDevices.at(pThis->mOpenedDeviceIdx)->bestWidth;
    uint32_t height = pThis->mListDevices.at(pThis->mOpenedDeviceIdx)->bestHeight;
    uint32_t fourcc = pThis->mListDevices.at(pThis->mOpenedDeviceIdx)->fourcc;
    VideoImage img;


    unsigned char* srcBuf[4];
    int srcPitches[4];
//    unsigned char* dstBuf[4];
//    int dstPitches[4];

    int size = avpicture_get_size(AV_PIX_FMT_YUV420P, width, height);
    unsigned char* rgb = new unsigned char[size];
    // Setting up AVFrame
    AVFrame* picturePtr = av_frame_alloc();//avcodec_alloc_frame();
    picturePtr->quality = 0;

    // Filling AVFrame with YUV420P data
    avpicture_fill((AVPicture *)picturePtr, rgb,
                       AV_PIX_FMT_YUV420P, width, height);

    AVPixelFormat srcFmt;

    switch (fourcc) {
    case V4L2_PIX_FMT_YUV420:
        srcFmt = AV_PIX_FMT_YUV420P;
        srcPitches[0] = width;
        srcPitches[1] = srcPitches[2] = width/2;
        srcPitches[3] = 0;
        break;
    case V4L2_PIX_FMT_YUYV:
        srcFmt = AV_PIX_FMT_YUYV422;
        srcPitches[0] = width*2;
        srcPitches[1] = srcPitches[2] = 0;
        srcPitches[3] = 0;
        break;
    case V4L2_PIX_FMT_RGB32:
        srcFmt = AV_PIX_FMT_RGB32;
        srcPitches[0] = width*4;
        srcPitches[1] = srcPitches[2] = 0;
        srcPitches[3] = 0;
        break;
    case V4L2_PIX_FMT_RGB24:
        srcFmt = AV_PIX_FMT_RGB24;
        srcPitches[0] = width*3;
        srcPitches[1] = srcPitches[2] = 0;
        srcPitches[3] = 0;
        break;
    default:
        srcFmt = AV_PIX_FMT_YUV420P;
        srcPitches[0] = width;
        srcPitches[1] = srcPitches[2] = width/2;
        srcPitches[3] = 0;
        break;
    }

    SwsContext* context = sws_getContext(width, height, srcFmt, width, height, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);


//    dstBuf[0] = rgb;
//    dstBuf[1] = dstBuf[2] = dstBuf[3] = 0;


    while (pThis->mRunningFlag) {
        // do something here
        struct v4l2_buffer bufferImg;
        bufferImg.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        bufferImg.memory = V4L2_MEMORY_MMAP;

        fd_set fds;
        struct timespec ts;
        int r;

        FD_ZERO(&fds);
        FD_SET(pThis->mOpenedDevice, &fds);

        /* Timeout. */
        ts.tv_sec = 0;
        ts.tv_nsec = (pThis->mListDevices.at(pThis->mOpenedDeviceIdx)->framerateDen*1000000) / pThis->mListDevices.at(pThis->mOpenedDeviceIdx)->framerateNum;

        r = pselect(pThis->mOpenedDevice + 1, &fds, NULL, NULL, &ts, NULL);

        if (-1 == r) {
            if (EINTR == errno)
                continue;
            break;
        }

        if (0 == r) {
            continue;
        }

        if (!FD_ISSET(pThis->mOpenedDevice, &fds))
            continue;

        /* Wait for next frame */
        if (ioctl (pThis->mOpenedDevice, VIDIOC_DQBUF, &bufferImg) < 0)
        {
            switch (errno)
            {
                case EAGAIN:
                    continue;
                case EIO:
                    /* Could ignore EIO, see spec. */
                    /* fall through */
                default:
                    qDebug() << "dequeue error: \n";
                    break;
            }
        }

        //qDebug() << "Got video captured frame with index is: " << bufferImg.index;

        img.imgBuffer = (unsigned char*)pThis->mBufferRequested[bufferImg.index];

        switch (fourcc) {
        case V4L2_PIX_FMT_YUV420:
            srcBuf[0] = img.imgBuffer;
            srcBuf[1] = img.imgBuffer + width*height;
            srcBuf[2] = srcBuf[1] + width*height / 4;
            srcBuf[3] = 0;
            break;
        case V4L2_PIX_FMT_YUYV:
            srcBuf[0] = img.imgBuffer;
            srcBuf[1] = 0;//img.imgBuffer + width*height;
            srcBuf[2] = 0;//srcBuf[1] + width*height / 4;
            srcBuf[3] = 0;
            break;
        case V4L2_PIX_FMT_RGB32:
            srcBuf[0] = img.imgBuffer;
            srcBuf[1] = 0;//img.imgBuffer + width*height;
            srcBuf[2] = 0;//srcBuf[1] + width*height / 4;
            srcBuf[3] = 0;
            break;
        case V4L2_PIX_FMT_RGB24:
            srcBuf[0] = img.imgBuffer;
            srcBuf[1] = 0;//img.imgBuffer + width*height;
            srcBuf[2] = 0;//srcBuf[1] + width*height / 4;
            srcBuf[3] = 0;
            break;
        default:
            srcBuf[0] = img.imgBuffer;
            srcBuf[1] = img.imgBuffer + width*height;
            srcBuf[2] = srcBuf[1] + width*height / 4;
            srcBuf[3] = 0;
            break;
        }

        sws_scale(context, srcBuf, srcPitches, 0, height, picturePtr->data, picturePtr->linesize);

        img.imgBuffer = rgb;
        img.fourcc = V4L2_PIX_FMT_YUV420;
        img.width = width;
        img.height = height;

        emit pThis->sendCapturedImg(&img);

        if (ioctl(pThis->mOpenedDevice, VIDIOC_QBUF, &bufferImg) < 0) {
            qDebug() << "Get image error\n";
            break;
        }
    }

    sws_freeContext(context);
    av_frame_free(&picturePtr);
    delete rgb;
}

VCLinxVideoCapture::VCLinxVideoCapture(QObject *parent)
    : QObject(parent)
    , mOpenedDevice(-1)
    , mMainThreadHandle(NULL)
    , mOpenedDeviceIdx(0xffffffff)
{
    int fd;
    unsigned idx = 0;
    QString devname = "/dev/video";

    do
    {
        QString dev = devname + QString::number(idx);

        fd = open (dev.toStdString().c_str(), O_RDWR);
        if (fd != -1) {

            uint32_t caps;
            /* Get device capabilites */
            struct v4l2_capability cap;
            if (ioctl (fd, VIDIOC_QUERYCAP, &cap) < 0)
            {
                qDebug() << "Cannot get device capabilities: " << dev;
                close (fd);
                idx++;
                continue;
            }

            if (cap.capabilities & V4L2_CAP_DEVICE_CAPS)
            {
                caps = cap.device_caps;
            }
            else
            {
                caps = cap.capabilities;
            }

            if ((caps & V4L2_CAP_VIDEO_CAPTURE) || (caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
                DeviceCaps* devCap = new DeviceCaps;
                devCap->devName = dev;
                if (caps & V4L2_CAP_READWRITE)
                    devCap->readMethod = IO_METHOD_READ;

                if (caps & V4L2_CAP_STREAMING)
                    devCap->readMethod = IO_METHOD_MMAP;

                // determine best resolution and framerate and color-space
                uint32_t bestWidth = 0, bestHeight = 0;
                uint32_t bestFRNum = 1, bestFRDen = 1, fourcc = 0xffffffff;

                enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                struct v4l2_fmtdesc fmt;
                struct v4l2_frmsizeenum frmsize;
                struct v4l2_frmivalenum frmival;

                fmt.index = 0;
                fmt.type = type;

                // priority fourcc is YUV420P YUY2 RGB
                while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) >= 0) {
                    if ((0xffffffff == fourcc) &&
                            ((fmt.pixelformat == V4L2_PIX_FMT_YUV420) ||
                             (fmt.pixelformat == V4L2_PIX_FMT_YUYV) ||
                             (fmt.pixelformat == V4L2_PIX_FMT_RGB32) ||
                             (fmt.pixelformat == V4L2_PIX_FMT_RGB24)))
                    {
                        fourcc = fmt.pixelformat; // first time
                    }
                    else if (V4L2_PIX_FMT_YUV420 == fmt.pixelformat) {
                        if (fourcc != V4L2_PIX_FMT_YUV420) {
                            fourcc = V4L2_PIX_FMT_YUV420;
                            bestWidth = 0;
                            bestHeight = 0;
                        }
                        else {
                            fmt.index ++;
                            continue;
                        }
                    }
                    else if (V4L2_PIX_FMT_YUYV == fmt.pixelformat) {
                        if ((fourcc != V4L2_PIX_FMT_YUV420) && (fourcc != V4L2_PIX_FMT_YUYV)) {
                            fourcc = V4L2_PIX_FMT_YUYV;
                            bestWidth = 0;
                            bestHeight = 0;
                        }
                        else {
                            fmt.index ++;
                            continue;
                        }
                    }
                    else if (V4L2_PIX_FMT_RGB32 == fmt.pixelformat) {
                        if ((fourcc != V4L2_PIX_FMT_YUV420) && (fourcc != V4L2_PIX_FMT_YUYV)
                                && (fourcc != V4L2_PIX_FMT_RGB32)) {
                            fourcc = V4L2_PIX_FMT_RGB32;
                            bestWidth = 0;
                            bestHeight = 0;
                        }
                        else {
                            fmt.index ++;
                            continue;
                        }
                    }
                    else if (V4L2_PIX_FMT_RGB24 == fmt.pixelformat) {
                        if ((fourcc != V4L2_PIX_FMT_YUV420) && (fourcc != V4L2_PIX_FMT_YUYV)
                                && (fourcc != V4L2_PIX_FMT_RGB32) && (fourcc != V4L2_PIX_FMT_RGB24)) {
                            fourcc = V4L2_PIX_FMT_RGB24;
                            bestWidth = 0;
                            bestHeight = 0;
                        }
                        else {
                            fmt.index ++;
                            continue;
                        }
                    }
                    else {
                        fmt.index++;
                        continue;
                    }

                    frmsize.pixel_format = fmt.pixelformat;
                    frmsize.index = 0;
                    while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0) {

                        memset(&frmival, 0, sizeof(frmival));
                        //frmival.index = 0;
                        frmival.pixel_format = fmt.pixelformat;
                        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                            frmival.width = frmsize.discrete.width;
                            frmival.height = frmsize.discrete.height;
                        } else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
                            frmival.width = frmsize.stepwise.max_width;
                            frmival.height = frmsize.stepwise.max_height;
                        }

                        if (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0) {
                            if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                                while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0) {
                                    if ((float)frmival.discrete.denominator / (float)frmival.discrete.numerator >= 25.0) {
                                        if (bestWidth* bestHeight < frmival.width*frmival.height) {
                                            bestWidth = frmival.width;
                                            bestHeight = frmival.height;
                                            bestFRNum = frmival.discrete.denominator;
                                            bestFRDen = frmival.discrete.numerator;
                                        }
                                    }
                                    frmival.index++;
                                }
                            }
                        } else {
                            if ((float)frmival.stepwise.max.denominator / (float)frmival.stepwise.max.numerator >= 25.0) {
                                if (bestWidth* bestHeight < frmival.width*frmival.height) {
                                    bestWidth = frmival.width;
                                    bestHeight = frmival.height;
                                    bestFRNum = frmival.stepwise.max.denominator;
                                    bestFRDen = frmival.stepwise.max.numerator;
                                }
                            }
                        }

                        frmsize.index++;
                    }
                    fmt.index++;
                }

                devCap->fourcc = fourcc;
                devCap->bestWidth = bestWidth;
                devCap->bestHeight = bestHeight;
                devCap->framerateNum = bestFRNum;
                devCap->framerateDen = bestFRDen;
                mListDevices.push_back(devCap);
            }

            close (fd);
        }

        idx++;
    }while( fd != -1);
}

VCLinxVideoCapture::~VCLinxVideoCapture()
{
    closeDevice();
}

unsigned VCLinxVideoCapture::numberDevices()
{
    return mListDevices.size();
}

const QListDevices &VCLinxVideoCapture::getDevices() const
{
    return mListDevices;
}

bool VCLinxVideoCapture::openDevice(unsigned idx)
{
    if (idx >= mListDevices.size())
        return false;

    if (mOpenedDevice != -1)
        closeDevice();

    mOpenedDevice = open(mListDevices.at(idx)->devName.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);
    if (mOpenedDevice == -1)
        return false;

    struct v4l2_format fmt;
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = mListDevices.at(idx)->fourcc;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    fmt.fmt.pix.width = mListDevices.at(idx)->bestWidth;
    fmt.fmt.pix.height = mListDevices.at(idx)->bestHeight;

    if (ioctl(mOpenedDevice, VIDIOC_S_FMT, &fmt) < 0) {
        qDebug() << "Set capture format error\n";
        closeDevice();
        return false;
    }

    struct v4l2_streamparm streamparm;
    CLEAR(streamparm);
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(mOpenedDevice, VIDIOC_G_PARM, &streamparm) < 0) {
        printf("Query stream param error\n");
        return -1;
    }

    mListDevices.at(idx)->framerateNum = streamparm.parm.capture.timeperframe.denominator;
    mListDevices.at(idx)->framerateDen = streamparm.parm.capture.timeperframe.numerator;

    qDebug() << "Max framerate supported is: " << (mListDevices.at(idx)->framerateNum / mListDevices.at(idx)->framerateDen);

    streamparm.parm.capture.capturemode = 0;
    streamparm.parm.capture.extendedmode = 0;

    if (ioctl(mOpenedDevice, VIDIOC_S_PARM, &streamparm) < 0) {
        printf("Setting stream param error\n");
        closeDevice();
        return false;
    }

    struct v4l2_requestbuffers videoBuf;
    CLEAR(videoBuf);
    videoBuf.count = 4;
    videoBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    videoBuf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(mOpenedDevice, VIDIOC_REQBUFS, &videoBuf) < 0) {
        qDebug() << "Request buffer error\n";
        closeDevice();
        return false;
    }

    uint32_t numBuf = videoBuf.count;

    for(uint32_t idx = 0; idx < numBuf; idx++) {

        struct v4l2_buffer bufferImg;
        bufferImg.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        bufferImg.memory = V4L2_MEMORY_MMAP;
        bufferImg.index = idx;

        if (ioctl(mOpenedDevice, VIDIOC_QUERYBUF, &bufferImg) < 0) {
            qDebug() << "Query buffer error\n";
            closeDevice();
            return false;
        }

        char* bufs = (char*)mmap(NULL, bufferImg.length, PROT_READ | PROT_WRITE, MAP_SHARED, mOpenedDevice, bufferImg.m.offset);
        mBufferLength = bufferImg.length;
        mBufferRequested.push_back(bufs);
        if (ioctl (mOpenedDevice, VIDIOC_QBUF, &bufferImg) < 0)
        {
            qDebug() << "cannot queue buffer " << idx;
            closeDevice();
            return false;
        }
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(mOpenedDevice, VIDIOC_STREAMON, &type) < 0) {
        qDebug() << "Start Capture error";
        closeDevice();
        return false;
    }

    mOpenedDeviceIdx = idx;

    mRunningFlag = true;
    mMainThreadHandle = new std::thread(mainThread, this);

    return true;
}

void VCLinxVideoCapture::closeDevice()
{
    if (mOpenedDevice != -1)
    {
        if (mMainThreadHandle) {
            mRunningFlag = false;
            mMainThreadHandle->join();
            delete mMainThreadHandle;
            mMainThreadHandle = NULL;
        }

        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(mOpenedDevice, VIDIOC_STREAMOFF, &type) < 0) {
            qDebug() << "Start Capture error";
        }

        while(!mBufferRequested.empty()) {
            munmap(mBufferRequested.front(), mBufferLength);
            mBufferRequested.erase(mBufferRequested.begin());
        }

        close(mOpenedDevice);
        mOpenedDevice = -1;
    }
}
