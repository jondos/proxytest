//Error constants...
const SINT32 E_SUCCESS=0;
#define E_UNKNOWN -1
#define E_SHUTDOWN -2 // this mix has been shut down and will not restart
#define E_CLOSED -3 //something which should be 'open' is in fact 'closed'
#define E_UNSPECIFIED -100 // A Parameter was not specified/not set
#define E_SPACE -101//there was not enough memory (or space in a buffer)
#define E_QUEUEFULL -200 // If a Send Queue contains more data then a defined number
#define E_AGAIN -300 //If something was'nt completed und should request again later..
#define E_TIMEDOUT -301 //An opertion has timed out
#define E_SOCKETCLOSED -302 //An operation which required an open socket uses a closed socket
#define E_SOCKET_LISTEN -303 //An error occured during listen
#define E_SOCKET_ACCEPT -304 //An error occured during accept
#define E_SOCKET_BIND -305 //An error occured during bind
#define E_SOCKET_LIMIT -306 //An error occurde because we run out of available sockets
#define E_UNKNOWN_HOST -400 // A hostname could not be resolved
#define E_FILE_OPEN -500 //Error in opening a file
#define E_FILE_READ -501 //Error in opening a file
#define E_XML_PARSE -600 //Error in parsing XML
#define E_NOT_CONNECTED -700 //Something is not connected that should be
														// (like a TCP/IP connection or a database connection)
#define E_NOT_FOUND -701 //Something was not found
#define E_INVALID -800 // sth is invalid (e.g. signature verifying)
