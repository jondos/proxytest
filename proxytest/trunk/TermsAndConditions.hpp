/*
Copyright (c) The JAP-Team, JonDos GmbH

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation and/or
       other materials provided with the distribution.
    * Neither the name of the University of Technology Dresden, Germany, nor the name of
       the JonDos GmbH, nor the names of their contributors may be used to endorse or
       promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef TERMSANDCONDITIONS_HPP_
#define TERMSANDCONDITIONS_HPP_

#define RESIZE_STORE 3

#include "StdAfx.h"
#include "CAMutex.hpp"

enum tcAnswerCode_t
{
	TC_FAILED = -1, TC_CONFIRMED = 0, TC_UNFINISHED = 1
};

typedef struct
{
	tcAnswerCode_t result;
	XERCES_CPP_NAMESPACE::DOMDocument* xmlAnswer;
} termsAndConditionMixAnswer_t;

void cleanupTnCMixAnswer(termsAndConditionMixAnswer_t *answer);

typedef struct
{
	UINT8 *tnc_id; /* The id of the Terms & Conditions is the operator ski. */
	UINT8 *tnc_locale; /* language code of the T&C translation. */
	/*UINT8 *tnc_date;  the date when the terms andCondtions became valid */
	DOMNode *tnc_customized; /* the operator specific Terms & Conditions definitions */
	DOMNode *tnc_template; /* the template needed to render the whole Terms and Conditions translation */
} termsAndConditionsTranslation_t;

void cleanupTnCTranslation(termsAndConditionsTranslation_t *tnCTranslation);

class TermsAndConditions
{

private:
	/* The id of the Terms & Conditions is the operator ski. */
	UINT8 *tnc_id;
	/* the overall number of translations (capacity) */
	UINT32 translations;
	/* array index pointing to where the last translation was stored */
	UINT32 currentTranslationIndex;
	/* array containing pointers to the Terms & Conditions translations */
	termsAndConditionsTranslation_t **allTranslations;
	/* needed to import the customized sections XML elements. ensures that these
	 * are not released accidently by former owner documents.
	 */
	XERCES_CPP_NAMESPACE::DOMDocument *customizedSectionsOwner;

public:

	/* all caller threads must lock for the whole time they are working with a translation entry.
	 * the class methods returning entry refernces are not thread-safe.
	 */
	CAMutex *synchLock;

public:

	/**
	 * Constructor with the id (Operator SKI), the number of translations and those elements
	 * to be imported to all translations
	 */
	TermsAndConditions(UINT8* id, UINT32 nrOfTranslations);
	virtual ~TermsAndConditions();

	/**
	 * returns the specific TermsAndconditions translation for the the language with the
	 * specified language code including the template AND the customized sections.
	 * If no translation exists for the specified language code NULL is returned.
	 */
	const termsAndConditionsTranslation_t *getTranslation(const UINT8 *locale);

	/**
	 * returns only the template of the translation specified by the language code
	 * or NULL if no such translation exist.
	 */
	const DOMNode *getTranslationTemplate(const UINT8 *locale);

	/**
	 * returns only the customized sections of the translation specified by the language code
	 * or NULL if no such translation exist.
	 */
	const DOMNode *getTranslationCustomizedSections(const UINT8 *locale);

	/**
	 * removes the specific Terms & Conditions translation for the the language with the
	 * specified language code including the template AND the customized sections.
	 */
	void removeTranslation(const UINT8 *locale);

	/**
	 * add a language specific terms and Conditions document, which can be
	 * retrieved by *getTermsAndConditionsDoc with the language code
	 */
	void addTranslation(const UINT8 *locale, DOMNode *tnc_customized, DOMNode *tnc_template);

	/*
	 * returns a POINTER, NOT A COPY of the ID of these T&Cs (the operator subject key identifier).
	 */
	const UINT8 *getID();

private:

	void setIndexToNextEmptySlot();
};

#endif /* TERMSANDCONDITIONS_HPP_ */
