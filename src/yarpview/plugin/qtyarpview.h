/*
 * Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef QTYARPVIEW_H
#define QTYARPVIEW_H

#include <QQuickItem>
#include "videoproducer.h"

//#include <cstdio>
#include <yarp/os/Property.h>
#include <yarp/os/Network.h>
#include <yarp/os/BufferedPort.h>
#include "ImagePort.h"
#include "signalhandler.h"
//using namespace yarp::os;

//-------------------------------------------------
// Program Options
//-------------------------------------------------
/*! \struct mOptions
    \brief The struct that stores the options
*/
struct mOptions
{
    unsigned int    m_refreshTime;
    char            m_portName[256];
    char            m_networkName[256];
    int             m_windWidth;
    int             m_windHeight;
    int             m_posX;
    int             m_posY;
    char            m_fileName[256];
    int             m_saveOnExit;
    char            m_outPortName[256];
    char            m_outNetworkName[256];
    int             m_outputEnabled;
    bool            m_synchRate;
    bool            m_autosize;
};
typedef struct mOptions pgmOptions;

/*! \class QtYARPView
    \brief The plugin Core class

    this is the plugin core class which acts as bridge between the QML and c++.
    in the C++ code is implemented the backend logic, instead in the QML is
    implemented the Visual part.
*/
class QtYARPView : public QQuickItem
{
    Q_OBJECT

    Q_DISABLE_COPY(QtYARPView)
    Q_PROPERTY(QObject *videoProducer READ getVideoProducer NOTIFY videoProducerChanged)
    Q_PROPERTY(int posX READ posX NOTIFY posXChanged)
    Q_PROPERTY(int posY READ posY NOTIFY posYChanged)
    Q_PROPERTY(int windowWidth READ windowWidth NOTIFY widthChanged)
    Q_PROPERTY(int windowHeight READ windowHeight NOTIFY heightChanged)
    Q_PROPERTY(int refreshInterval READ refreshInterval NOTIFY refreshIntervalChanged)

public:
    QtYARPView(QQuickItem *parent = 0);
    ~QtYARPView();

    Q_INVOKABLE void freeze(bool check);
    Q_INVOKABLE void synchDisplayPeriod(bool check);
    Q_INVOKABLE void synchDisplaySize(bool check);
    Q_INVOKABLE void changeRefreshInterval(int);
    Q_INVOKABLE void saveFrame();
    Q_INVOKABLE void setFileName(QUrl url);
    Q_INVOKABLE void setFilesName(QUrl url);
    Q_INVOKABLE void startDumpFrames();
    Q_INVOKABLE void stopDumpFrames();
    Q_INVOKABLE bool parseParameters(QStringList);
    Q_INVOKABLE void clickCoords_2(int x, int y);
    Q_INVOKABLE void clickCoords_4(int start_x, int start_y, int end_x, int end_y);

    QObject *getVideoProducer();
    int posX();
    int posY();
    int windowWidth();
    int windowHeight();
    int refreshInterval();

private:
    void createObjects();
    void deleteObjects();
    void printHelp();
    void setOptionsToDefault();
    void setOptions(yarp::os::Searchable& options);
    bool openPorts();
    void closePorts();
    void saveOptFile(char *fileName);
    void periodToFreq(double avT, double mT, double MT, double &avH, double &mH, double &MH);

private:
    VideoProducer videoProducer;

    // This Network yarp must be placed before any other yarp dependent member
    yarp::os::Network yarp;
    SignalHandler sigHandler;
#ifdef YARP_LITTLE_ENDIAN
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelBgra> > *ptr_inputPort;
#else
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgba> > *ptr_inputPort;
#endif
    yarp::os::BufferedPort<yarp::os::Bottle> *_pOutPort;
    InputCallback *ptr_portCallback;
    pgmOptions _options;

signals:
    void refreshIntervalChanged();
    void videoProducerChanged();
    void posXChanged();
    void posYChanged();
    void widthChanged();
    void heightChanged();
    void sizeChanged();
    void sendPortFps(QString avg, QString min, QString max);
    void sendDisplayFps(QString avg, QString min, QString max);
    void synchRate(bool check);
    void autosize(bool check);
    void setName(QString name);
private slots:
    void onSendFps(double portAvg, double portMin, double portMax, double dispAvg, double dispMin, double dispMax);
    void onWindowSizeChangeRequested();
};

#endif // QTYARPVIEW_H
