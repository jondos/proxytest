#ifndef CHECK_MIX_H_
#define CHECK_MIX_H_

#include "../../monitoringDefs.h"

#define CMP_EQUAL 0

#define PARSE_MATCH 0
#define PARSE_NO_MATCH 1

#define MAX_CMD_LINE_OPT 100
#define ERROR -1
#define SUCCESS 0

#define NETWORKING_STATUS_OPT_NAME "networking"
#define PAYMENT_STATUS_OPT_NAME "payment"
#define SYSTEM_STATUS_OPT_NAME "system"

/* must correspond to enum status_types */
static const char *STATUS_OPT_NAMES[NR_STATUS_TYPES] =
{
		NETWORKING_STATUS_OPT_NAME,
#ifdef PAYMENT
		PAYMENT_STATUS_OPT_NAME,
#endif
		SYSTEM_STATUS_OPT_NAME
};

enum curr_state_field
{
	field_st_ignore = -1,
	field_st_type = 0, st_stateLevel, field_st_description
};

typedef unsigned int status_type_flag_t;
typedef enum curr_state_field curr_state_field_t;

struct state_parse_data
{
	status_type_t curr_status;
	status_type_flag_t desired_status;
	int all_states_code;
	curr_state_field_t curr_state_field;
};

struct checkMix_cmdLineOpts
{
	status_type_flag_t desired_status;
	char mix_address[MAX_CMD_LINE_OPT];
	int mix_port;
};

typedef struct checkMix_cmdLineOpts checkMix_cmdLineOpts_t;
typedef struct state_parse_data state_parse_data_t;

static int setCurrentStatusType(state_parse_data_t *parseData, const char *name, int elemNameLength);
static int setCurrentStateField(state_parse_data_t *parseData, const char *name, int elemNameLength);
static int unsetCurrentStateField(state_parse_data_t *parseData, const char *name, int elemNameLength);
static int processMixStatusMessage(int mixSocket, XML_Parser *parser, char *readBuf, int readBufLen);
static int handleCmdLineOptions(int argc, char *argv[], checkMix_cmdLineOpts_t *cmdLineOpts);
static int connectToMix(char* mix_address, int mix_port);

static void printDefinedStatusTypes();
static void printUsage();

#endif /*CHECK_MIX_H_*/
