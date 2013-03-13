/*
Copyright (c) 2000, The JAP-Team
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

	- Redistributions of source code must retain the above copyright notice,
		this list of conditions and the following disclaimer.

	- Redistributions in binary form must reproduce the above copyright notice,
		this list of conditions and the following disclaimer in the documentation and/or
		other materials provided with the distribution.

	- Neither the name of the University of Technology Dresden, Germany nor the names of its contributors
		may be used to endorse or promote products derived from this software without specific
		prior written permission.


THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/
#include "../StdAfx.h"
#include "../CACmdLnOptions.hpp"

/*
#ifdef _DEBUG
int sockets;
#endif
*/
CACmdLnOptions* pglobaloptions;

/**
* This function runs all tests of the project an is needed if no gui testrunner is
* available. It prints the results to the standard output.
*/
int main(void) {

	#if defined(HAVE_CRTDBG)
//	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
//	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
//	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
//	_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
//	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
//	_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );
	WSADATA wsadata;
	WSAStartup(0x0202,&wsadata);

	UINT32 tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	tmpDbgFlag |= _CRTDBG_ALLOC_MEM_DF;
	tmpDbgFlag |=_CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(tmpDbgFlag);
	_CrtMemState s1, s2, s3;
	#endif




  //--- Create the event manager and test controller
  CPPUNIT_NS::TestResult controller;

  //--- Add a listener that colllects test result
  CPPUNIT_NS::TestResultCollector result;
  controller.addListener( &result );

  //--- Add a listener that print dots as test run.
  CPPUNIT_NS::BriefTestProgressListener progress;
  controller.addListener( &progress );

  //--- Add the top suite to the test runner
  CPPUNIT_NS::TestRunner runner;
  runner.addTest( CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest() );
  runner.run( controller );
	CPPUNIT_NS::CompilerOutputter outputter( &result, std::cerr );
  outputter.write();
	return result.wasSuccessful() ? 0 : 1;
}
	/*
	CppUnit::TextUi::TestRunner runner;
	// Add your test suites here.
    runner.addTest(AllTestsCA::suite());
    runner.run();
    return 0;
}*/

