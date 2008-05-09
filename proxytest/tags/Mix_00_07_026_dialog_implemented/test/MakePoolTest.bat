#!/bin/sh
rm ./a.out
gcc  -DDEBUG -I/usr/local/include -I/usr/local/include/xercesc -I/usr/include/xercesc PoolSpeedTest.cpp ../CASocket.o ../CASocketGroup.o ../CAUtil.o -L/usr/local/lib ../CABase64.o ../CAASymCipher.o ../CASocketAddrINet.o -lxerces-c  -lpthread -lcrypto ../xml/libxml.a ../CAQueue.o ../CAThread.o ../CAMsg.o ../CACmdLnOptions.o ../CACertificate.o ../CAListenerInterface.o ../CASignature.o ../CACertStore.o ../CASocketAddrUnix.o ../popt/libpopt.a -lcrypto ../CASocketGroupEpoll.o ../CASingleSocketGroup.o -lepoll
