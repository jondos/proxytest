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
#ifndef __CAXMLERRORMESSAGE__
#define __CAXMLERRORMESSAGE__

#include "CAUtil.hpp"
#include "CAAbstractXMLEncodable.hpp"
#include "CAXMLCostConfirmation.hpp"

#define XML_ELEMENT_ERROR_MSG "ErrorMessage"
#define XML_ELEMENT_ERROR_MSG_ENVELOPE "ErrorMessages"

/**
 * This class encapsulates an error or success message.
 * In order to be independent from the HTTP protocol on the higher layer,
 * this is now used instead of http errorcodes.
 *
 * @author Bastian Voigt, Elmar Schraml
 */
class CAXMLErrorMessage : public CAAbstractXMLEncodable
	{
		public:
			//codes corresponding to various errors
			static const UINT32 ERR_OK = 0;
			static const UINT32 ERR_INTERNAL_SERVER_ERROR = 1;
			static const UINT32 ERR_WRONG_FORMAT = 2;
			static const UINT32 ERR_WRONG_DATA = 3;
			static const UINT32 ERR_KEY_NOT_FOUND = 4;
			static const UINT32 ERR_BAD_SIGNATURE = 5;
			static const UINT32 ERR_BAD_REQUEST = 6;
			static const UINT32 ERR_NO_ACCOUNTCERT = 7;
			static const UINT32 ERR_NO_BALANCE = 8;
			static const UINT32 ERR_NO_CONFIRMATION = 9;
			static const UINT32 ERR_ACCOUNT_EMPTY = 10;
			static const UINT32 ERR_CASCADE_LENGTH = 11;
			static const UINT32 ERR_DATABASE_ERROR = 12;
			static const UINT32 ERR_INSUFFICIENT_BALANCE = 13;
			static const UINT32 ERR_NO_FLATRATE_OFFERED = 14;
			static const UINT32 ERR_INVALID_CODE = 15;
			static const UINT32 ERR_OUTDATED_CC = 16;
			static const UINT32 ERR_INVALID_PRICE_CERT = 17;
			static const UINT32 ERR_MULTIPLE_LOGIN = 18;
			static const UINT32 ERR_NO_RECORD_FOUND = 19;
			static const UINT32 ERR_SUCCESS_BUT_WITH_ERRORS = 20;
			static const UINT32 ERR_BLOCKED = 21;
			static const UINT32 ERR_NO_ERROR_GIVEN = 100;


			/**
			* Creates an errorMessage object. The errorcode should be one of the
			* above ERR_* constants.
			* @param errorCode UINT32 one of the above constants
			* @param message String a human-readable description of the error
			*/
			CAXMLErrorMessage(const UINT32 errorCode, UINT8 * message);

			/**
			* Uses a default description String
			* @param errorCode UINT32
			*/
			CAXMLErrorMessage(UINT32 errorCode);


			/**
			 * attaches an object to the error message
			*/
			CAXMLErrorMessage(const UINT32 errorCode, UINT8* message, CAAbstractXMLEncodable* messageObject);


			/**
			* Parses the string XML representation
			*/
			CAXMLErrorMessage(UINT8 * strXmlData);

			CAXMLErrorMessage(DOMElement* elemRoot);

			~CAXMLErrorMessage();

			SINT32 toXmlElement( XERCES_CPP_NAMESPACE::DOMDocument* a_doc, DOMElement* &elemRoot);
			UINT32 getErrorCode()
			{
				return m_iErrorCode;
			}

			UINT8* getDescription()
			{
				return m_strErrMsg;
			}

			void* getMessageObject()
			{
				return m_messageObject;
			}

		private:
			SINT32 setValues(DOMElement* elemRoot);
			UINT32 m_iErrorCode;
			UINT8 *m_strErrMsg;
			void* m_messageObject;
	};

#endif
