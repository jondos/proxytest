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
#ifndef __SAMPLETEST__
#define __SAMPLETEST__

/* As Sample is a fictive class, it can not really be imported. */
//#include "../Sample.hpp"

/**
 * Tests a (fictive) class with the name "Sample".
 * It is an example of a unit test implementation.
 */
class SampleTest : public CppUnit::TestFixture
{
	public:
	SINT32 m_One;
	bool m_bTwo;

	/**
	* Method is run before each test, for example for object creation.
	*/
	void setUp();

	/**
	* Method is run after each test, for example for object deletion.
	*/
	void tearDown();

	/**
	* Tests a fictive method of the Sample class with the name "doSomethingMethod".
	*/
	void testDoSomethingMethod();

	/**
	* Tests a fictive method of the Sample class with the name "doAnything".
	*/
	void testDoAnything();

	/* These macros define the performed tests. */
	CPPUNIT_TEST_SUITE(SampleTest);
	CPPUNIT_TEST(testDoSomethingMethod);
	CPPUNIT_TEST(testDoAnything);
	CPPUNIT_TEST_SUITE_END();
};

#endif
