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

#define PAYMENT

#include <stdlib.h>
#include <expat.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "check_mix.h"

/**
 * @author Simon Pecher, JonDos GmbH
 * Nagios plugin for mix-server monitoring
 *
 * This plugin performs connects to the monitoring reads the
 * status information send by the mix and performs SAX based
 * parsing of the status information.
 */


/**
 * writes the the name of the current statusType to stdout
 * the status type names are defined in monitoringDefs.h
 */
static int setCurrentStatusType(state_parse_data_t *parseData, const char *name, int elemNameLength)
{
	int i = 0;
	for(i = FIRST_STATUS; i < NR_STATUS_TYPES; i++)
	{
		if(strncmp(name, STATUS_NAMES[i], elemNameLength) == CMP_EQUAL)
		{
			if( (parseData->desired_status & STATUS_FLAG(i)) )
			{
				parseData->curr_status = i;
				printf("%s: ",STATUS_NAMES[i]);
			}
			else
			{
				parseData->curr_status = stat_undef;
			}
			return PARSE_MATCH;
		}
	}
	return PARSE_NO_MATCH;
}

/**
 * sets the current state field of the parse_data struct to the current type of information which
 * is contained by the current status element so the handleText method can identify the
 * context of the information to be written to stdout.
 */
static int setCurrentStateField(state_parse_data_t *parseData, const char *name, int elemNameLength)
{
	if(strncmp(name, DOM_ELEMENT_STATE_NAME, elemNameLength) == CMP_EQUAL)
	{
		parseData->curr_state_field = field_st_type;
		return PARSE_MATCH;
	}
	else if(strncmp(name, DOM_ELEMENT_STATE_LEVEL_NAME, elemNameLength) == CMP_EQUAL)
	{
		parseData->curr_state_field = st_stateLevel;
		return PARSE_MATCH;
	}
	else if(strncmp(name, DOM_ELEMENT_STATE_DESCRIPTION_NAME, elemNameLength) == CMP_EQUAL)
	{
		parseData->curr_state_field = field_st_description;
		return PARSE_MATCH;
	}
	return PARSE_NO_MATCH;
}

/**
 * invalidates the current selected status type if a corresponding end tag is read.
 */
static int unsetCurrentStateField(state_parse_data_t *parseData, const char *name, int elemNameLength)
{
	if( (strncmp(name, DOM_ELEMENT_STATE_NAME, elemNameLength) == CMP_EQUAL) )
	{
		parseData->curr_state_field = field_st_ignore;
		return PARSE_MATCH;
	}
	if(	(strncmp(name, DOM_ELEMENT_STATE_LEVEL_NAME, elemNameLength) == CMP_EQUAL) )
	{
		fprintf(stdout," - ");
		parseData->curr_state_field = field_st_ignore;
		return PARSE_MATCH;
	}
	if( (strncmp(name, DOM_ELEMENT_STATE_DESCRIPTION_NAME, elemNameLength) == CMP_EQUAL) )
	{
		fprintf(stdout," ");
		parseData->curr_state_field = field_st_ignore;
		return PARSE_MATCH;
	}
	return PARSE_NO_MATCH;
}

static void XMLCALL startElement(void *userData, const char *name, const char **atts)
{
	state_parse_data_t *parseData = (state_parse_data_t *) userData;
	int elemNameLength = strlen(name);
	if(setCurrentStatusType(parseData, name, elemNameLength)==PARSE_NO_MATCH)
	{
		if(parseData->curr_status != stat_undef)
		{
			setCurrentStateField(parseData, name, elemNameLength);
		}
	}
}

static void XMLCALL endElement(void *userData, const char *name)
{
	state_parse_data_t *parseData = (state_parse_data_t *) userData;
	int elemNameLength = strlen(name);
	if(parseData->curr_status != stat_undef)
	{
		unsetCurrentStateField(parseData, name, elemNameLength);
	}
}

static void XMLCALL handleText(void *userData, const XML_Char *s, int len)
{
	state_parse_data_t *parseData = (state_parse_data_t *) userData;
	char *text = (char *) malloc(sizeof(char)*(len+1));
	int retStateCode = -1;
	int i = -1;

	if(parseData->curr_status != stat_undef)
	{
		strncpy(text, s, len);
		text[len] = 0;
		switch(parseData->curr_state_field)
		{
			case st_stateLevel:
			{
				//output the name of the level of the current state
				//(e. g. OK, CRITICAL, etc.) and set the corresponding
				//return type for nagios
				for(i = stl_ok; i < NR_STATE_LEVELS; i++)
				{
					if(strncmp(text, STATUS_LEVEL_NAMES[i], len) == CMP_EQUAL)
					{
						retStateCode = i;
						break;
					}
				}
				if(retStateCode != -1)
				{
					fprintf(stdout,"%s",STATUS_LEVEL_NAMES[i]);
					if(parseData->all_states_code != stl_critical)
					{
						parseData->all_states_code =
							(retStateCode > parseData->all_states_code) ?
									retStateCode : parseData->all_states_code;
					}
				}
				break;
			}
			case field_st_description:
			{
				fprintf(stdout,"%s",text);
				break;
			}
			/* Nothing to do yet */
			case field_st_ignore:
			{
				break;
			}
			case field_st_type:
			{
				break;
			}
		}
	}
	free(text);
}

int main(int argc, char *argv[])
{
  char buf[BUFSIZ+1];
  int sd = 0, ret = 0;

  checkMix_cmdLineOpts_t cmdLineOpts;
  state_parse_data_t parseData;
  XML_Parser parser = XML_ParserCreate("UTF-8");

  memset(&parseData, 0, sizeof(state_parse_data_t));
  memset(&cmdLineOpts, 0, sizeof(checkMix_cmdLineOpts_t));
  memset(buf, 0, (sizeof(char)*(BUFSIZ+1)));

  parseData.curr_status = stat_undef;
  parseData.all_states_code = -1;

  XML_SetUserData(parser, &parseData);
  XML_SetElementHandler(parser, startElement, endElement);
  XML_SetCharacterDataHandler(parser, handleText);

  //read cmdline options
  if(handleCmdLineOptions(argc, argv, &cmdLineOpts) != SUCCESS)
  {
	  printUsage();
	  fprintf(stderr, "check_mix: exiting due to invalid options.\n");
	  exit(1);
  }
  parseData.desired_status = cmdLineOpts.desired_status;

  sd = connectToMix(cmdLineOpts.mix_address, cmdLineOpts.mix_port);
  if(sd <= ERROR)
  {
	 fprintf(stderr, "check_mix: an error occured while "
			 "trying to connect to mix %s. exiting now.\n", cmdLineOpts.mix_address);
	 exit(1);
  }

  /* read in and parse StatusMessage */
  ret = processMixStatusMessage(sd, &parser,buf, BUFSIZ);
  close(sd);

  if(ret != SUCCESS)
  {
	  fprintf(stderr, "check_mix: an error occured while "
			  "processing mix status message. exiting now.\n");
	  exit(1);
  }
  XML_ParserFree(parser);
  if(parseData.all_states_code < 0)
  {
	  fprintf(stderr, "check_mix: return code is invalid. exiting now.\n");
	  exit(1);
  }
  fprintf(stdout, "\n");
  return parseData.all_states_code;
}

static int connectToMix(char* mix_address, int mix_port)
{
	int sd = 0, stat = 0;
	struct sockaddr_in mixInetAddr;
	struct hostent *mixInfo = NULL;
	struct in_addr mix_ip;

	memset(&mix_ip, 0, sizeof(struct in_addr));

	mixInfo = gethostbyname(mix_address);
	if(mixInfo != NULL)
	{
		if(mixInfo->h_addr != NULL)
		{
			memcpy(&mix_ip, mixInfo->h_addr, mixInfo->h_length);
		}
		else
		{
			fprintf(stderr, "connection to mix failed: cannot resolve address %s\n", mix_address);
			return ERROR;
		}

	}
	else
	{
		fprintf(stderr, "connection to mix failed: host %s not found\n", mix_address);
		return ERROR;
	}
	mixInetAddr.sin_family = AF_INET;
	mixInetAddr.sin_addr = mix_ip;
	mixInetAddr.sin_port = (in_port_t) htons(mix_port);

	sd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
	if(sd < 0)
	{
		fprintf(stderr, "failed to open socket: bad file descriptor %d\n", sd);
		return ERROR;
	}
	stat = connect(sd, (struct sockaddr *)&mixInetAddr, sizeof(struct sockaddr_in));
	if(stat != SUCCESS)
	{
		perror("cannot connect to mix");
		close(sd);
		return ERROR;
	}
	return sd;
}

static int handleCmdLineOptions(int argc, char *argv[],
					checkMix_cmdLineOpts_t *cmdLineOpts)
{
  int i = 0, c = 0;
  int optArgLen = 0;
  int ret = SUCCESS;

  while (1)
  {
     c = getopt(argc, argv, "a:i:p:");
     if (c == -1 || c == EOF)
     {
    	 break;
     }
     switch (c) {
	     case 'a':
	 	 {
	 		 for(i = FIRST_STATUS; i < NR_STATUS_TYPES; i++)
	 		 {
	 			 if(strncasecmp(optarg,
	 				STATUS_OPT_NAMES[i],
	 				strlen(STATUS_OPT_NAMES[i])) == CMP_EQUAL )
	 			 {
	 				cmdLineOpts->desired_status |= STATUS_FLAG(i);
	 				break;
	 			 }
	 		 }
	 		 if(cmdLineOpts->desired_status == 0)
	 		 {
	 			fprintf(stderr, "unknown status type \"%s\". ", optarg);
	 			printDefinedStatusTypes();
	 			return ERROR;
	 		 }
	 		 break;
	     }
	  	 case 'i':
	 	 {
	 		 optArgLen = strlen(optarg);
	 		 if(optArgLen > MAX_CMD_LINE_OPT)
	 		 {
	 			fprintf(stderr, "Mix address parameter too long");
	 			return ERROR;
	 		 }
	 		 strncpy(cmdLineOpts->mix_address, optarg, optArgLen);
	 		 break;
	 	 }
	  	 case 'p':
	 	 {
	 		optArgLen = strlen(optarg);
	 		 if(optArgLen > MAX_CMD_LINE_OPT)
	 		 {
	 			fprintf(stderr, "Mix port parameter too long");
	 			return ERROR;
	 		 }
	 		 cmdLineOpts->mix_port = atoi(optarg);
	 		 if(cmdLineOpts->mix_port == 0)
	 		 {
	 			fprintf(stderr, "invalid mix port parameter: %s\n", optarg);
	 			return ERROR;
	 		 }
	 		 break;
	 	 }
     }
  }

  if(cmdLineOpts->desired_status == 0)
  {
	fprintf(stderr, "status type was not specified.\n");
	printDefinedStatusTypes();
	ret = ERROR;
  }
  if(strlen(cmdLineOpts->mix_address) == 0)
  {
  	 fprintf(stderr, "mix address was not specified.\n");
  	 ret = ERROR;
  }
  if(cmdLineOpts->mix_port == 0)
  {
	  fprintf(stderr, "mix port was not specified.\n");
	  ret = ERROR;
  }
  return ret;
}

static int processMixStatusMessage(int mixSocket, XML_Parser *parser,
									char *readBuf, int readBufLen)
{

  char *xmlStatusMessage = NULL;
  char xmlStartCmp[strlen(XML_STATUS_MESSAGE_START)];
  char *cmp_start = NULL;
  int xmlStartCmpOffset = 0, theRest = 0, cmpSiz = 0, compareSuccess = 0;
  int done = 0, bytesRead = 0, xmlPrefixRead = 0;

  /* this should never happen */
  if((readBuf == NULL) || (mixSocket < 0) || (parser == NULL) )
  {
	  fprintf(stderr, "check_mix: fatal error.\n");
	  return ERROR;
  }

  memset(xmlStartCmp, 0, (sizeof(char)*strlen(XML_STATUS_MESSAGE_START)));
  do
  {
	  bytesRead = read(mixSocket, readBuf, readBufLen);
	  if(bytesRead < 0)
	  {
		  fprintf(stderr,"check_mix: an error occured while receiving "
				  "the status message.\n");
		  return ERROR;
	  }
	  done = !bytesRead;
	  if(!xmlPrefixRead)
	  {
		 if(bytesRead <= 0)
		 {
			 fprintf(stderr,
					 "check_mix: received status message "
					 "does not contain a valid start tag\n");
			 return ERROR;
		 }
		 if((cmp_start == NULL) && ((cmp_start = strchr(readBuf, '<')) != NULL) )
		 {
			 while(cmp_start != NULL)
			 {
				 /* read in first char of start tag: begin comparison */
				 cmpSiz = (strlen(XML_STATUS_MESSAGE_START) < (bytesRead - (cmp_start - readBuf)) ) ?
				 		   strlen(XML_STATUS_MESSAGE_START) : (bytesRead - (cmp_start - readBuf));
				 if(strncmp(XML_STATUS_MESSAGE_START, cmp_start, cmpSiz) == CMP_EQUAL)
				 {
 		  			 xmlStartCmpOffset += cmpSiz;
 		  			 compareSuccess = 1;
 		  			 break;
				 }
				 else
				 {
					 cmp_start = strchr(cmp_start+1,'<');
				 }
			 }
		  	 if(cmp_start == NULL)
		  	 {
		  		continue;
		  	 }
		 }
		 else
		 {
		  	  /* Comparing already started */
			  cmp_start = readBuf;
			  theRest = (strlen(XML_STATUS_MESSAGE_START)-xmlStartCmpOffset);
			  cmpSiz = (theRest < bytesRead) ? theRest : bytesRead;
			  if(strncmp((XML_STATUS_MESSAGE_START+xmlStartCmpOffset),
					     cmp_start, cmpSiz) == CMP_EQUAL)
			  {
				  xmlStartCmpOffset += cmpSiz;
				  compareSuccess = 1;
			  }
			  else
			  {
				  /* Comparing not equal: start again */
				  cmp_start = NULL;
				  xmlStartCmpOffset = 0;
				  continue;
			  }
		 }
		 if(compareSuccess)
		 {
			 if(xmlStartCmpOffset == strlen(XML_STATUS_MESSAGE_START))
			 {
				xmlPrefixRead = 1;
				/* will not fail */
				XML_Parse(*parser, XML_STATUS_MESSAGE_START, strlen(XML_STATUS_MESSAGE_START), done);
				if(cmpSiz < BUFSIZ)
				{
					/* a part of buf needs to be parsed as well */
					xmlStatusMessage = (cmp_start+cmpSiz);
				}
				/* Start of Status message already parsed: wait for next buf */
				else
				{
					continue;
				}
			 }
			 else
			 {
				 compareSuccess = 0;
				 continue;
			 }
		 }
	  }
	  else
	  {
		  xmlStatusMessage = readBuf;
	  }
	  if (XML_Parse(*parser, xmlStatusMessage, strlen(xmlStatusMessage), done) == XML_STATUS_ERROR)
	  {
		  fprintf(stderr,
		          "%s at line %lu, column %lu\n",
		           XML_ErrorString(XML_GetErrorCode(*parser)),
		           XML_GetCurrentLineNumber(*parser),
		           XML_GetCurrentColumnNumber(*parser));
		  fprintf(stderr, "check_mix: cannot parse StatusMessage\n");
		  return ERROR;
	  }
	  memset(readBuf, 0, readBufLen);
  } while(bytesRead > 0);

  return SUCCESS;
}

static void printDefinedStatusTypes()
{
	int i = 0;
	fprintf(stderr, "Defined status types are: ");
	for(i = FIRST_STATUS; i < NR_STATUS_TYPES; i++)
	{
		fprintf(stderr, " \"%s\"%s",STATUS_OPT_NAMES[i],
				(i != NR_STATUS_TYPES - 1) ? "," : "\n");
	}
}

static void printUsage()
{
	fprintf(stderr, "usage: check_mix -a [networking | payment | system] -i <mix> -p <mixport>\n");
}
