// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
#include <yarp/os/Port.h>
#include <yarp/NameClient.h>
#include <yarp/Companion.h>
#include <yarp/os/Time.h>
#include <yarp/os/Thread.h>
#include <yarp/os/Semaphore.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/PortReaderBuffer.h>
#include <yarp/os/PortWriterBuffer.h>
#include <yarp/os/PortablePair.h>
#include <yarp/os/BinPortable.h>
#include <yarp/Logger.h>
#include <yarp/NetType.h>

#include <yarp/os/BufferedPort.h>
#include <yarp/os/Network.h>

#include "TestList.h"

using namespace yarp::os;

using yarp::String;
using yarp::NetType;
using yarp::Logger;

#ifndef DOXYGEN_SHOULD_SKIP_THIS

class ServiceProvider : public PortReader {
public:

    virtual bool read(ConnectionReader& connection) {
        Bottle receive;
        //printf("service provider reading data\n");
        receive.read(connection);
        //printf("service provider read data\n");
        receive.addInt(5);
        ConnectionWriter *writer = connection.getWriter();
        if (writer!=NULL) {
            //printf("service provider replying\n");
            receive.write(*writer);
            //printf("service provider replied\n");
        }
        return true;
    }
};

class ServiceTester : public Portable {
public:
    yarp::UnitTest& owner;
    Bottle send, receive;
    int ct;

    ServiceTester(yarp::UnitTest& owner) : owner(owner) {}

    virtual bool write(ConnectionWriter& connection) {
        ct = 0;
        //printf("service tester writing data\n");
        send.write(connection);
        //printf("service tester wrote data\n");
        connection.setReplyHandler(*this);
        return true;
    }

    virtual bool read(ConnectionReader& connection) {
        //printf("service tester getting reply\n");
        receive.read(connection);
        //printf("service tester got reply\n");
        ct++;
        return true;
    }

    void finalCheck() {
        owner.checkEqual(receive.size(),send.size()+1,"size incremented");
        owner.checkEqual(ct,1,"just one read");
    }
};


class DelegatedReader : public Thread {
public:
    Port p;

    DelegatedReader() {
        p.open("/reader");
    }
    
    virtual void run() {
        for (int i=0; i<3; i++) {
            Bottle b,b2;
            p.read(b,true);
            b2.addInt(b.get(0).asInt()+1);
            p.reply(b2);
        }
    }
};

class DelegatedWriter : public Thread {
public:
    Port p;
    int total;

    DelegatedWriter() {
        p.open("/writer");
        Network::connect("/writer","/reader");
    }

    virtual void run() {
        total = 0;
        for (int i=0; i<3; i++) {
            Bottle b, b2;
            b.addInt(i);
            p.write(b,b2);
            total += b2.get(0).asInt(); // should be i+1
        }
        // total should be 1+2+3 = 6
    }
};


class DelegatedCallback : public TypedReaderCallback<Bottle> {
public:
    Bottle saved;
    Semaphore produce;

    DelegatedCallback() : produce(0) {}

    virtual void onRead(Bottle& bot) {
        saved = bot;
        produce.post();
    }
};

#endif /*DOXYGEN_SHOULD_SKIP_THIS*/


class PortTest : public yarp::UnitTest {
public:
    virtual yarp::String getName() { return "PortTest"; }

    void testOpen() {
        report(0,"checking opening and closing ports");
        Port out, in;

        in.open("/in");
        out.open(Contact::bySocket("tcp","",9999));

        Contact conIn = in.where();
        Contact conOut = out.where();

        out.addOutput(Contact::byName("/in").addCarrier("udp"));
        //Time::delay(0.2);

        checkEqual(conIn.getName().c_str(),"/in","name is recorded");

        checkTrue(String(conOut.getName().c_str()).strstr("/tmp")==0,
                  "name is created");

        Bottle bot1, bot2;
        bot1.fromString("5 10 \"hello\"");
        out.write(bot1);
        in.read(bot2);
        checkEqual(bot1.get(0).asInt(),5,"check bot[0]");
        checkEqual(bot1.get(1).asInt(),10,"check bot[1]");
        checkEqual(bot1.get(2).asString().c_str(),"hello","check bot[2]");

        bot1.fromString("18");
        out.write(bot1);
        in.read(bot2);
        checkEqual(bot1.get(0).asInt(),18,"check one more send/receive");

        in.close();
        out.close();
    }


    void testReadBuffer() {
        report(0,"checking read buffering");
        Bottle bot1;
        PortReaderBuffer<Bottle> buf;
        buf.setStrict(true);

        bot1.fromString("1 2 3");
        for (int i=0; i<10000; i++) {
            bot1.addInt(i);
        }

        Port input, output;
        input.open("/in");
        output.open("/out");

        buf.setStrict();
        buf.attach(input);

        output.addOutput(Contact::byName("/in").addCarrier("tcp"));
        //Time::delay(0.2);

        report(0,"writing...");
        output.write(bot1);
        output.write(bot1);
        output.write(bot1);
        report(0,"reading...");
        Bottle *result = buf.read();

        for (int j=0; j<3; j++) {
            if (j!=0) {
                result = buf.read();
            }
            checkTrue(result!=NULL,"got something check");
            if (result!=NULL) {
                checkEqual(bot1.size(),result->size(),"size check");
                YARP_INFO(Logger::get(),String("size is in fact ") + 
                          NetType::toString(result->size()));
            }
        }

        output.close();
        input.close();
    }

    void testUdp() {
        report(0,"checking udp");

        Bottle bot1;
        PortReaderBuffer<Bottle> buf;

        bot1.fromString("1 2 3");
        for (int i=0; i<10000; i++) {
            bot1.addInt(i);
        }

        Port input, output;
        input.open("/in");
        output.open("/out");

        buf.setStrict();
        buf.attach(input);

        output.addOutput(Contact::byName("/in").addCarrier("udp"));
        //Time::delay(0.2);

        report(0,"writing/reading three times...");

        report(0,"checking for whatever got through...");
        int ct = 0;
        while (buf.check()) {
            ct++;
            Bottle *result = buf.read();
            checkTrue(result!=NULL,"got something check");
            if (result!=NULL) {
                checkEqual(bot1.size(),result->size(),"size check");
                YARP_INFO(Logger::get(),String("size is in fact ") + 
                          NetType::toString(result->size()));
            }
        }
        if (ct==0) {
            report(0,"NOTHING got through - possible but sad");
        }

        output.close();
        input.close();
    }


    void testHeavy() {
        report(0,"checking heavy udp");

        Bottle bot1;
        PortReaderBuffer<Bottle> buf;

        bot1.fromString("1 2 3");
        for (int i=0; i<100000; i++) {
            bot1.addInt(i);
        }

        Port input, output;
        input.open("/in");
        output.open("/out");

        buf.setStrict();
        buf.attach(input);

        output.addOutput(Contact::byName("/in").addCarrier("udp"));
        //Time::delay(0.2);


        for (int j=0; j<3; j++) {
            report(0,"writing/reading three times...");
            output.write(bot1);
        }

        report(0,"checking for whatever got through...");
        int ct = 0;
        while (buf.check()) {
            ct++;
            Bottle *result = buf.read();
            checkTrue(result!=NULL,"got something check");
            if (result!=NULL) {
                checkEqual(bot1.size(),result->size(),"size check");
                YARP_INFO(Logger::get(),String("size is in fact ") + 
                          NetType::toString(result->size()));
            }
        }
        if (ct==0) {
            report(0,"NOTHING got through - possible but sad");
        }

        output.close();
        input.close();
    }


    void testPair() {
        report(0,"checking paired send/receive");
        PortReaderBuffer<PortablePair<Bottle,Bottle> > buf;

        Port input, output;
        input.open("/in");
        output.open("/out");

        buf.setStrict();
        buf.attach(input);

        output.addOutput(Contact::byName("/in").addCarrier("tcp"));
        //Time::delay(0.2);


        PortablePair<Bottle,Bottle> bot1;
        bot1.head.fromString("1 2 3");
        bot1.body.fromString("4 5 6 7");

        report(0,"writing...");
        output.write(bot1);
        report(0,"reading...");
        PortablePair<Bottle,Bottle> *result = buf.read();

        checkTrue(result!=NULL,"got something check");
        checkEqual(bot1.head.size(),result->head.size(),"head size check");
        checkEqual(bot1.body.size(),result->body.size(),"body size check");

        output.close();
        input.close();
    }


    void testReply() {
        report(0,"checking reply processing");
        ServiceProvider provider;

        Port input, output;
        input.open("/in");
        output.open("/out");
    
        input.setReader(provider);
    
        output.addOutput(Contact::byName("/in").addCarrier("tcp"));
        Time::delay(0.1);
        ServiceTester tester(*this);
        output.write(tester);
        Time::delay(0.1);
        tester.finalCheck();
        Time::delay(0.1);
        output.close();
        input.close();    
    }


    virtual void testBackground() {
        report(0,"test communication in background mode");

        Port input, output;
        input.open("/in");
        output.open("/out");
        output.enableBackgroundWrite(true);

        BinPortable<int> bin, bout;
        bout.content() = 42;
        bin.content() = 20;

        output.addOutput("/in");

        report(0,"writing...");
        output.write(bout);
        report(0,"reading...");
        input.read(bin);

        checkEqual(bout.content(),bin.content(),"successful transmit");

        while (output.isWriting()) {
            report(0,"waiting for port to stabilize");
            Time::delay(0.2);
        }

        bout.content() = 88;

        report(0,"writing...");
        output.write(bout);
        report(0,"reading...");
        input.read(bin);

        checkEqual(bout.content(),bin.content(),"successful transmit");

        output.close();
        input.close();
    }

    void testWriteBuffer() {
        report(0,"testing write buffering");

        Port input, output, altInput;
        input.open("/in");
        altInput.open("/in2");
        output.open("/out");
        output.addOutput("/in");

        report(0,"beginning...");

        BinPortable<int> bin;
        PortWriterBuffer<BinPortable<int> > binOut;
        binOut.attach(output);

        int val1 = 15;
        int val2 = 30;

        BinPortable<int>& active = binOut.get();
        active.content() = val1;
        binOut.write();

        output.addOutput("/in2");
        BinPortable<int>& active2 = binOut.get();
        active2.content() = val2;
        binOut.write();

        input.read(bin);
        checkEqual(val1,bin.content(),"successful transmit");

        altInput.read(bin);
        checkEqual(val2,bin.content(),"successful transmit");

        while (output.isWriting()) {
            report(0,"waiting for port to stabilize");
            Time::delay(0.2);
        }

        report(0,"port stabilized");
		output.close();
        report(0,"shut down output buffering");
    }

    void testBufferedPort() {
        report(0,"checking buffered port");
        BufferedPort<BinPortable<int> > output, input;
        output.open("/out");
        input.open("/in");

        report(0,"is write a no-op when no connection exists?...");
        BinPortable<int>& datum0 = output.prepare();
        datum0.content() = 123;
        report(0,"writing...");
        output.write();

        output.addOutput("/in");
        report(0,"now with a connection...");
        BinPortable<int>& datum = output.prepare();
        datum.content() = 999;
        report(0,"writing...");
        output.write();
        report(0,"reading...");
        BinPortable<int> *bin = input.read();
        checkEqual(bin->content(),999,"good send");
	}

    void testCloseOrder() {
        report(0,"check that port close order doesn't matter...");

        for (int i=0; i<4; i++) {
            // on OSX there is a problem only tickled upon repetition
            {
                Port input, output;
                input.open("/in");
                output.open("/out");
                output.addOutput("/in");
                
                report(0,"closing in");
                input.close();
                
                report(0,"closing out");
                output.close();
            }
            
            {
                Port input, output;
                input.open("/in");
                output.open("/out");
                output.addOutput("/in");
                
                report(0,"closing out");
                output.close();
                
                report(0,"closing in");
                input.close();
            }
        }
    }


    virtual void testDelegatedReadReply() {
        report(0,"check delegated read and reply...");
        DelegatedReader reader;
        DelegatedWriter writer;
        reader.start();
        writer.start();
        writer.stop();
        reader.stop();
        checkEqual(writer.total,6,"read/replies give right checksum");
    }

    virtual void testReaderHandler() {
        report(0,"check reader handler...");
        Port in;
        Port out;
        DelegatedCallback callback;
        out.open("/out");
        in.open("/in");
        Network::connect("/out","/in");
        PortReaderBuffer<Bottle> reader;
        reader.setStrict();
        reader.attach(in);
        reader.useCallback(callback);
        Bottle src("10 10 20");
        out.write(src);
        callback.produce.wait();
        checkEqual(callback.saved.size(),3,"object came through");
        out.close();
        in.close();
    }

    virtual void testReaderHandler2() {
        report(0,"check reader handler, bufferedport style...");
        BufferedPort<Bottle> in;
        Port out;
        DelegatedCallback callback;
        in.setStrict();
        out.open("/out");
        in.open("/in");
        Network::connect("/out","/in");
        in.useCallback(callback);
        Bottle src("10 10 20");
        out.write(src);
        callback.produce.wait();
        checkEqual(callback.saved.size(),3,"object came through");
    }


    virtual void testStrictWriter() {
        report(0,"check strict writer...");
        BufferedPort<Bottle> in;
        BufferedPort<Bottle> out;
        in.setStrict();
        in.open("/in");
        out.open("/out");
        
        Network::connect("/out","/in");
        
        Bottle& outBot1 = out.prepare();
        outBot1.fromString("hello world");
        printf("Writing bottle 1: %s\n", outBot1.toString().c_str());
        out.write(true);
        
        Bottle& outBot2 = out.prepare();
        outBot2.fromString("2 3 5 7 11");
        printf("Writing bottle 2: %s\n", outBot2.toString().c_str());
        out.write(true);
        
        Bottle *inBot1 = in.read();
        checkTrue(inBot1!=NULL,"got 1 of 2 items");
        if (inBot1!=NULL) {
            printf("Bottle 1 is: %s\n", inBot1->toString().c_str());
            checkEqual(inBot1->size(),2,"match for item 1");
        }
        Bottle *inBot2 = in.read();
        checkTrue(inBot2!=NULL,"got 2 of 2 items");
        if (inBot2!=NULL) {
            printf("Bottle 2 is: %s\n", inBot2->toString().c_str());
            checkEqual(inBot2->size(),5,"match for item 1");
        }
    }


    virtual void testRecentReader() {
        report(0,"check recent reader...");
        BufferedPort<Bottle> in;
        BufferedPort<Bottle> out;
        in.setStrict(false);
        in.open("/in");
        out.open("/out");
        
        Network::connect("/out","/in");
        
        Bottle& outBot1 = out.prepare();
        outBot1.fromString("hello world");
        printf("Writing bottle 1: %s\n", outBot1.toString().c_str());
        out.write(true);
        
        Bottle& outBot2 = out.prepare();
        outBot2.fromString("2 3 5 7 11");
        printf("Writing bottle 2: %s\n", outBot2.toString().c_str());
        out.write(true);

        Time::delay(0.25);

        Bottle *inBot2 = in.read();
        checkTrue(inBot2!=NULL,"got 2 of 2 items");
        if (inBot2!=NULL) {
            printf("Bottle 2 is: %s\n", inBot2->toString().c_str());
            checkEqual(inBot2->size(),5,"match for item 1");
        }
    }


    virtual void testUnbufferedClose() {
        report(0,"check that ports that receive data and do not read it can close...");
        BufferedPort<Bottle> sender;
        Port receiver;
        sender.open("/sender");
        receiver.open("/receiver");
        Network::connect("/sender","/receiver");
        Time::delay(0.25);
        Bottle& bot = sender.prepare();
        bot.clear();
        bot.addInt(1);
        sender.write();
        Time::delay(0.25);
        report(0,"if this hangs, PortTest::testUnbufferedClose is unhappy");
        receiver.close();
        sender.close();
    }

    virtual void runTests() {
        yarp::NameClient& nic = yarp::NameClient::getNameClient();
        nic.setFakeMode(true);

        testOpen();
        testReadBuffer();
        testPair();
        testReply();
        testUdp();
        //testHeavy();


        testBackground();
        testWriteBuffer();
        testBufferedPort();
        testCloseOrder();
        testDelegatedReadReply();
        testReaderHandler();
        testReaderHandler2();
        testStrictWriter();
        testRecentReader();
        testUnbufferedClose();

        nic.setFakeMode(false);
    }
};

static PortTest thePortTest;

yarp::UnitTest& getPortTest() {
    return thePortTest;
}

