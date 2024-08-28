/*
 *
 * NAME: EvHandl_client.cpp
 *
 * COPYRIGHT Ericsson Utvecklings AB, Sweden 2012.
 * All rights reserved.
 *
 *  The Copyright to the computer program(s) herein
 *  is the property of Ericsson Telecom AB, Sweden.
 *  The program(s) may be used and/or copied only with
 *  the written permission from Ericsson Telecom AB or in
 *  accordance with the terms and conditions stipulated in the
 *  agreement/contract under which the program(s) have been
 *  supplied.
 *
 * .DESCRIPTION
 *  This is the main program for the gmlog and rpmo command. This a updated
 *  version of old GMLOG client with added support for R-PMO.
 *
 * DOCUMENT NO
 *      -
 *
 * AUTHOR
 *      2012-10-25 FJG/DO EERITAU
 *
 * REVISION
 *
 * CHANGES
 *
 * RELEASE REVISION HISTORY
 *
 * REV NO       DATE            NAME            DESCRIPTION
 * PA1          20121025        EERITAU         First Revision
 * R1A01        2013????        EERITAU         Version in APG43L 1.1
 * P2A0X        20140225        UABJOOL         Code cleanup
 *                                              Added support for quota
 * R2A01        20140401        UABJOOL         Version in APG43L 1.2
 * R2C01	20181101	EANNCZE		Version in APG43L 3.6
 */


// Module Include Files
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/capability.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ctime>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cstring>
#include <cstdint>
#include <unistd.h> 

using namespace std;


// Macros
#define debug false
#define BUFFER_SIZE 41000
#define IMSI_ID 0
#define TLLI_ID 1
#define NO_ID 2
#define VERSION "CAA 139 2066 R2C01"

// Enumerations
struct InvokedAs {
  enum { unknown, GMLog, RPMO };
};

struct MsId {
  enum { IMSI, TLLI, none };
};

// Constants
// Max supported output file size is 10GB
const uint64_t  MAX_FILE_SIZE = 10000000000ULL;

// Max supported logging time, unit is seconds, i.e. 60 minutes
const uint32_t  MAX_LOGGING_TIME = 3600;

const int32_t  MAX_EVENT_IDS  = 64;
const int32_t  MAX_CELLS      = 2048;

const int32_t  HEADER_LENGTH  = 4;

const int32_t  IMSI_LENGTH        = 10;  // Encoded IMSI length
const int32_t  IMSI_FILTER_ID     = 1;
const int32_t  IMSI_FILTER_LENGTH = (IMSI_LENGTH/2)+1;  // Length in words

const int32_t  TLLI_LENGTH        = 4;
const int32_t  TLLI_FILTER_ID     = 2;
const int32_t  TLLI_FILTER_LENGTH = (TLLI_LENGTH/2)+1;  // Length in words

const int32_t  GMLOG_EID_TROUBLE_SHOOTING_RP = 17;
const int32_t  GMLOG_EID_TROUBLE_SHOOTING_CP = 18;

// All system events are between 0xFF00 and 0xFFFF
const int32_t  EID_SYSTEM_EVENT_MASK = 0xFF00;

const string GMLOG_COMMAND_NAME = "gmlog";
const string RPMO_COMMAND_NAME  = "rpmo";

// Absolute path to the output directory in the NBFS (North Bound File
// System) What is visible when connecting to APG using FTP or SFTP is
// just what follows "/data/opt/ap/internal_root". This path is
// assumed to end with "/".
const string BASE_DIRECTORY = "/data/opt/ap/internal_root/";

// Subpath within NBFS where Event Handler Client is allowed to write
const string DESTINATION_DIRECTORY = "tools/evhandlclient/";

// The Event Handler Client is only allowed to write files to
// "BASE_DIRECTORY/tools/evhandlclient/". This path is assumed to end
// with "/".
const string  OUTPUT_DIRECTORY = BASE_DIRECTORY + DESTINATION_DIRECTORY;

const mode_t OUTPUT_DIRECTORY_MODES = S_IRWXU | S_IRWXG | S_IRWXO;


// Filename suffix to use for gmlog generated files
const string GMLOG_SUFFIX = ".gml";

// Filename used for gmlog data when no filename is specified
const string DEFAULT_GMLOG_FILENAME = "logfile_gmlog.gml";


// Filename suffix to use for rpmo generated files
const string RPMO_SUFFIX = ".rpm";

// Filename used for rpmo data when no filename is specified
const string DEFAULT_RPMO_FILENAME = "logfile_rpmo.rpm";


// Clear any system resource capability if running as root.
void clear_sys_resource_capability();

// Send request and wait on the reply.
int send_receive_data(char *buffer, int bytes_to_send, int socket_fd);

// Send the buffer to the socket.
int send_buffer(char *buffer, int bytes_to_send, int socket_fd);

// receive data from the socket.
int receive_buffer(char *buffer, int bytes_to_receive, int socket_fd);

// receive data from the socket with timeout
int receive_buffer_timeout(char *buffer, int bytes_to_receive, int socket_fd);

// Write the eventdata to the file or to the stdout if the out == null.
void write_to_file(char *buffer, int number_of_bytes, uint64_t& bytesWritten);

// Send a request of the event. Packing of packet with the filter and eid.
void request_event_subscription(int eid, int *cell_list, int socket_fd, char *msFlt, int msId, int cmd);

// Check the result from an connection request.
void check_connection_result(char result);

// Decode the events id arguments. Comma separated to a array of int.
int decode_event_list(char *events, int *event_list);

// Decode the cell pointers arguments. Comma separated to a array of int.
int decode_cell_list(char *cells, int *cell_list);

// Prints usage (depends on GMLog and RPMO specific print usage functions).
void print_usage(int cmd);

// Print syntax and example text for GMLog usage.
void print_gmlog_help_txt();

// Print syntax and example text for R-PMO usage.
void print_rpmo_help_txt();

// Assemble the IMSI as coded in GMLOG IWD
void assemble_imsi(char *imsi_buff, const char *imsi);

// Assemble TLLI as coded in GMLOG IWD
void assemble_tlli(char *tlli_buff, char *tlli);

// Print hex buffer
void printBuffer(char * buff, int len);

// Quit thread function. Listens for 'q' or 'Q' followed by <Enter> or
// <Return> on stdin to quit
void* quit_request_checker(void* pParams);

// Print statistics thread function. Periodically (total number of
// collected events and kilobytes written to file)
void* print_statistics(void* pParams);


ofstream   out;
string     filename;
uint32_t   numberOfEvents;
uint64_t   bytesWritten   = 0;
uint64_t   maxFileSize    = MAX_FILE_SIZE;    // Default is max (10 GB)
uint32_t   maxLoggingTime = MAX_LOGGING_TIME; // Default is max (60 minutes)

int main(int argc, char *argv[])
{
  int cmd; // Act as GMLog or RPMO command?

  int  event_list[MAX_EVENT_IDS];
  int  cell_list[MAX_CELLS];
  int  msId = MsId::none;

  char *tlli = NULL; // Pointer to tlli found in argv
  char *imsi = NULL; // Pointer to imsi found in argv
  
  char *buffer, *msIdBuff; // Buffers allocated using malloc


  cell_list[0] = -1;  // -1 indicates end of cells in list

  buffer   = (char *)malloc(BUFFER_SIZE);
  msIdBuff = (char *)malloc(IMSI_LENGTH);

  if ((buffer == NULL) || (msIdBuff == NULL)) {
    printf("\nOut of memory, aborting!\n\n");
    exit(1);
  }

  // On APG43L we are running as root when called from COM CLI. Due to
  // this the quota setting used for APG43L 1.2 and onwards does not
  // apply unless we clear that capability from the Linux capability
  // set for this process. The Linux capability function is based on a
  // withdrawn POSIX 1e draft specification, i.e. this is only
  // applicable for Linux.
  clear_sys_resource_capability();
  
  {
    struct stat  st;
    // Check if directory exists if not create evhandlclient directory.
    // (This code assumes the root directory for the NBFS has already been
    // created)
    //
    // TODO: Be more dynamic, i.e. depending on if running on APG or
    // not the base directory is either "." or the NBFS directory. Can
    // be controlled via #define set in the makefile as there is no
    // obvious way to determine wether we are running on APG43L or
    // not.
    if (stat(OUTPUT_DIRECTORY.c_str(), &st) == -1) {
      mkdir(OUTPUT_DIRECTORY.c_str(), OUTPUT_DIRECTORY_MODES);
    }
  }

  // Check whether it is R-PMO or GMLog client user is trying to run
  // and initialize the default filename used accordingly
  //for (int n=0; n<argc; n++) {
  //  printf("argv[%d] = %s\n", n, argv[n]);
  //}

  if (GMLOG_COMMAND_NAME == argv[1]) {
    cmd = InvokedAs::GMLog;
    // Set default filename
    filename += OUTPUT_DIRECTORY + DEFAULT_GMLOG_FILENAME;
  }
  else if (RPMO_COMMAND_NAME == argv[1]) {
    cmd = InvokedAs::RPMO;
    // Set default filename
    filename += OUTPUT_DIRECTORY + DEFAULT_RPMO_FILENAME;
  }
  else {
    print_usage(InvokedAs::unknown);
  }
  
  // Check that number of parameters are correct
  if (argc < 5) {
    printf("\nMissing arguments...\n\n");
    print_usage(cmd);
  }
  
  // Add events to event_list array
  if (decode_event_list((char *) argv[4], event_list) != 0) {
    print_usage(cmd);
  }
  
  //Get cmd line options
  for (int n=5; n< argc; n++) {
    if (argv[n][0] == '-') {
      if (argc == n+1) {
        printf("\nMissing arguments...\n");
        print_usage(cmd);
      }
      
      if (argv[n][1] == 'f') {
        // Log file name option
        string  suffix_to_use =
          (cmd == InvokedAs::GMLog)? GMLOG_SUFFIX : RPMO_SUFFIX;
        
        //printf("\nSuffix to use is: %s\n",suffix_to_use.c_str());
        n++;
        filename.clear();
        filename += OUTPUT_DIRECTORY + argv[n];
        
        //printf("\nAbsolute filename is: %s\n",filename.c_str());
        if ((filename.length()-OUTPUT_DIRECTORY.length()) >=
            suffix_to_use.length()) {
          //printf("\nGiven filename is longer than the suffix\n");
          if (filename.compare(filename.size() - suffix_to_use.size(),
                               suffix_to_use.size(),
                               suffix_to_use) != 0) {
            int    len     = BASE_DIRECTORY.length()-1;
            string subpath = filename.substr(len, filename.length());

            printf("\n%s must end with %s\n\n",
                   subpath.c_str(),
                   suffix_to_use.c_str());
            print_usage(cmd);
          }
        }
        else {
          int    len     = BASE_DIRECTORY.length()-1;
          string subpath = filename.substr(len, filename.length());
          
          printf("\n%s must end with %s\n\n",
                 subpath.c_str(),
                 suffix_to_use.c_str());
          print_usage(cmd);
        }
        
        // Check if file already exists
        struct stat  st;
        char ch;
        char new_file_name[100];
        while (true) {
          if (stat(filename.c_str(), &st) != -1) {
            printf("\nLogfile already exists. Overwrite existing file(y/n) "
                   "or quit(q)?\n");
            scanf("%c", &ch);
            
            if (ch == 'n') {
              printf("Please enter new filename + <ENTER>: \n");
              scanf("%99s", new_file_name);
              filename.clear();
              filename += OUTPUT_DIRECTORY + new_file_name;
              //printf("\nAbsolute filename is: %s\n",filename.c_str());

              if ((filename.length()-OUTPUT_DIRECTORY.length()) >=
                  suffix_to_use.length()) {
                //printf("\nGiven filename is longer than the suffix\n");
                if (filename.compare(filename.size() - suffix_to_use.size(),
                                     suffix_to_use.size(),
                                     suffix_to_use) != 0) {
                  int    len     = BASE_DIRECTORY.length()-1;
                  string subpath = filename.substr(len, filename.length());
                  
                  printf("\n%s must end with %s\n\n",
                         subpath.c_str(),
                         suffix_to_use.c_str());
                  print_usage(cmd);
                }
              }
              else {
                int    len     = BASE_DIRECTORY.length()-1;
                string subpath = filename.substr(len, filename.length());
                
                printf("\n%s must end with %s\n\n",
                       subpath.c_str(),
                       suffix_to_use.c_str());
                print_usage(cmd);
              }
            }
            else if (ch == 'y') {
              // Overwrite existing file
              break;
            }
            else if (ch == 'q') {
              exit(0);
            }
          }
          else {
            break;
          }
        }
      }
      else if (argv[n][1] == 's') {
        // Log file size option
        n++;
        maxFileSize = ((uint64_t)(atoi((char *)argv[n])) * 1000000ULL);
        if (maxFileSize > MAX_FILE_SIZE) {
          printf("\nMax supported output file size is 10000 megabytes.\n");
          print_usage(cmd);
        }
      }
      else if (argv[n][1] == 'h' ) {
        // Maximum time for logging option
        n++;
        maxLoggingTime = (atoi((char *)argv[n]) * 60);
        if (maxLoggingTime > MAX_LOGGING_TIME) {
          printf("\nMax supported logging time is 60 minutes.\n");
          print_usage(cmd);
        }
      }
      else if ( argv[n][1] == 'c' ) {
        // Cell list option
        if (cmd == InvokedAs::GMLog) {
          if (!((msId == MsId::none) && (cell_list[0] == -1))) {
            printf("\nOne and only one of options: -t, -i, -c shall be "
                   "given.\n\n");
            print_usage(cmd);
          }
        }
        else if (cell_list[0] != -1) {
          printf("\nOnly one -c shall be given.\n\n");
          print_usage(cmd);
        }
        n++;
        if (decode_cell_list((char *) argv[n], cell_list) != 0) {
          printf("\nToo many cells specified in the cell list; max is %d.\n\n",
                 MAX_CELLS);
          print_usage(cmd);
        }
      }
      else if (argv[n][1] == 'i') {
        // IMSI option only allowed for GMLog
        if (cmd == InvokedAs::GMLog) {
          if (!((msId == MsId::none) && (cell_list[0] == -1))) {
            printf("\nOne and only one of options: -t, -i, -c shall be "
                   "given.\n\n");
            print_usage(cmd);
          }
        }
        else {
          printf("\nIMSI only allowed for GMLog.\n");
          print_usage(cmd);
        }
        n++;
        imsi = argv[n];
        
        int length = strlen(imsi);
        if (length > 15) {
          printf("\nToo many digits (%d) in given IMSI value. 14 or 15 digits "
                 "shall be used.\n\n", length);
          print_usage(cmd);
        }
        else if (length < 14) {
          printf("\nToo few digits (%d) in given IMSI value. 14 or 15 digits "
                 "shall be used.\n\n", length);
          print_usage(cmd);
        }

        msId = MsId::IMSI;
        assemble_imsi(msIdBuff, imsi);
      }
      else if (argv[n][1] == 't') {
        // TLLI option only allowed for GMLog
        if (cmd == InvokedAs::GMLog) {
          if (!((msId == MsId::none) && (cell_list[0] == -1))) {
            printf("\nOne and only one of options: -t, -i, -c shall be "
                   "given.\n\n");
            print_usage(cmd);
          }
        }
        else {
          printf("\nTLLI only allowed for GMLog.\n");
          print_usage(cmd);
        }
        n++;
        tlli = argv[n];
        
        int length = strlen(tlli);
        if (length > 10) {
          printf("\nToo many digits (%d) in given TLLI value. Up to 10 digits "
                 "shall be used.\n\n", length);
          print_usage(cmd);
        }
        
        assemble_tlli(msIdBuff, tlli);
        msId = MsId::TLLI;
      }
    }
  }// FOR
  
  // Open file to write binary data into
  out.open(filename.c_str(), ios::out|ios::binary);
  if (out.is_open() == false) {
    printf("Unable to open the file\n");
    printf("Reason: %s\n\n", strerror(errno));
    exit(1);
  }

  // Setup for the remote side (BSC).
  struct sockaddr_in bsc_address;
  bsc_address.sin_family = AF_INET;
  bsc_address.sin_port = htons(atoi((char *)argv[3]));
  bsc_address.sin_addr.s_addr = inet_addr((char *)argv[2]);

  printf("\nOpen connection to: %s...",argv[2]);
  fflush(stdout);

  int  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    printf("Could not create a socket.\n");
    printf("Reason: %s\n\n", strerror(errno));
    exit(1);
  }
  
  {
    int flag = 1;
    if (setsockopt(socket_fd,
                   IPPROTO_TCP,
                   TCP_NODELAY,
                   (char *) &flag,  // this cast is historical cruft
                   sizeof(int)) < 0) {
      printf("Failed to disable Nagle's algorithm on socket.\n");
      printf("Reason: %s\n\n", strerror(errno));
    }
  }
  
  if (connect(socket_fd, (struct sockaddr *)&bsc_address, sizeof(bsc_address)) < 0) {
    printf("Socket connection failed.\n");
    printf("Reason: %s\n\n", strerror(errno));
    exit(1);
  }
  printf("done\n\n");
  fflush(stdout);

  // Create connect request message
  buffer[0] = 0; // Length of the data
  buffer[1] = 1; // Length of the data
  buffer[2] = 0; // Channel
  buffer[3] = 0; // Channel
  buffer[4] = 0; // CMN = Connect
  buffer[5] = 1; // CMN = Connect

  printf("Sending connection request to application (%s)...",argv[1]);
  fflush(stdout);
  // Send connection request and store result

  // DW1 == Number of Data Words
  // DW2 == Channel
  // DW3 == Control Message Number (CMN)
  //
  // => only 6 octets should be sent.
  send_buffer(buffer, 6, socket_fd);
  write_to_file(buffer, 6, bytesWritten);
  
  // DW1 == Number of Data Words
  // DW2 == Channel
  // DW3 == Control Message Number (CMN)
  // DW4 == Result
  // DW5 == Protocol Version + Application Version
  //
  // => only 10 octets should be received.
  receive_buffer(buffer, 10, socket_fd);
  // Since only 15 result codes are defined, bits b00-b04 contains the
  // result, i.e. the 8th octet or index 7 in the connectResponse
  // buffer contains the result...
  check_connection_result(buffer[7]);
  write_to_file(buffer, 10, bytesWritten);

  printf("connected\n\n");
  fflush(stdout);

  // Send subscribe request for all given eids (event IDs)
  for (int n=0; event_list[n] != -1; n++) {
    request_event_subscription(event_list[n], cell_list, socket_fd, msIdBuff,
                               msId, cmd);
  }

  // Start thread to listen after q or Q on stdin. If received then quit
  printf("\n\nTo quit press: 'q' or 'Q' + <ENTER> or <RETRUN>\n\n\n");
  fflush(stdout);

  
  pthread_t quit_thread;
  pthread_t statistics_thread;
  pthread_create(&quit_thread, NULL, &quit_request_checker, NULL);
  pthread_create(&statistics_thread, NULL, &print_statistics, NULL);


  // Receive event data until stopped
  numberOfEvents = 0;
  int    number_of_bytes;
  int    bytes_received;
  time_t last_sec = time(NULL);
  while (true) {
    bytes_received = receive_buffer(buffer, HEADER_LENGTH, socket_fd);

    number_of_bytes = ((unsigned char)buffer[0] * 256 +
                       (unsigned char)buffer[1]) * 2;
    if (number_of_bytes > BUFFER_SIZE) {
      printf("\nERROR: Reading from socket event length too long.\n"
             "%10d bytes stated in received event, expected max %d bytes.\n",
             number_of_bytes, BUFFER_SIZE);
      exit(1);
    }
    write_to_file(buffer, bytes_received, bytesWritten);
    
    bytes_received = receive_buffer(buffer, number_of_bytes, socket_fd);
    write_to_file(buffer, bytes_received, bytesWritten);
    
    numberOfEvents++;
    
    if (time(NULL) > (last_sec + 1)) {
      // flush file max every second
      out.flush();
      last_sec = time(NULL);
      
      if (bytesWritten > maxFileSize) {
        printf("\nMaximum file size reached. Logging stopped.\n");
        out.close();
        exit(1);
      }
    }
  }

  out.close();
  shutdown(socket_fd, SHUT_RDWR); // Shutdown socket for both reading and writing

  return 0;
}

//===============================================================================
//      Clear the system resource capabilty bit in case we are running as
//      root so we do not overstep any assigned quota on the file system we
//      are writing to.
//
//===============================================================================
void clear_sys_resource_capability()
{
  // Intent is to clear the CAP_SYS_RESOURCE bit so that root user
  // cannot override disk quota limits
  
  cap_t caps = cap_get_proc();

  if (caps == NULL) {
    printf("\nERROR: Failed to get capabilities\n");
    printf("Reason: %s", strerror(errno));
    exit(1);
  }
  else {
    cap_value_t cap_list[] = { CAP_SYS_RESOURCE };
    
    // Clear capability CAP_SYS_RESOURCE
    if (cap_set_flag(caps, CAP_EFFECTIVE, sizeof(cap_list),
                    cap_list, CAP_CLEAR) != 0) {
      printf("\nERROR: Failed to clear capability flag\n");
      printf("Reason: %s\n", strerror(errno));
      exit(1);
    }
    else {
      // Change process capability
      if (cap_set_proc(caps) != 0) {
        printf("\nERROR: Failed to change capability flag\n");
        printf("Reason: %s\n", strerror(errno));
        exit(1);
      }
    }
    
    if (cap_free(caps) != 0) {
      printf("\nERROR: Failed to free capability struct\n");
      printf("Reason: %s\n", strerror(errno));
      exit(1);
    }
  }
}


//===============================================================================
//      Assemble TLLI as coded in the GMLog IWD
//
//===============================================================================
void assemble_tlli(char *tlli_buff, char *tlli) {
  uint32_t  tlli_int = atoi(tlli);

  tlli_buff[0] = (tlli_int & 0xff00) >> 8;
  tlli_buff[1] =  tlli_int & 0xff;
  tlli_buff[2] = (tlli_int & 0xff000000) >> 24;
  tlli_buff[3] = (tlli_int & 0xff0000) >> 16;
}

//===============================================================================
//      Assemble IMSI as coded in the GMLog IWD
//
//===============================================================================
void assemble_imsi(char *imsi_buff, const char *imsi) {
  const int imsi_length = strlen(imsi);
  
  // IWD specifies contents using DW16 and here we use octets, i.e.
  //
  // |    DW0    |     DW1     |     DW2     |     DW3     |     DW4     |
  // +-----+-----+------+------+------+------+------+------+------+------+
  // |  0  |  1  |   2  |   3  |   4  |   5  |   6  |   7  |   8  |   9  |
  // +-----+-----+------+------+------+------+------+------+------+------+
  // | b15 - b00 | b15  -  b00 | b15  -  b00 | b15  -  b00 | b15  -  b00 |
  //             | d3 d2  d1 --| d7 d6  d5 d4|d11 d10 d9 d8|d12d13 d14d15|
  //             | n3 n2  n1 --| n3 n2  n1 n0| n3 n2  n1 n0| n3 n2  n1 n0|
  //
  // DWx == Data Word x
  // bxx == bit xx
  // dxx == digit xx
  // nxx == nibble xx in Data Word (nibble == 4 bit section of octet)

  imsi_buff[0] = 0; // DW0, b08-b15; spare
  imsi_buff[1] = 8; // DW0, b00-b07; length in octets

  // DW1, b00-b02; type of identity to "1", i.e. IMSI
  // DW1,     b03; odd/even flag (even = 0, odd = 1)
  switch (imsi_length) {
  case 14:
    /**  Digit #1 -+        even -+       ID -+
    //   __________|_________     |     ______|_______
    //  /                    \    |    /              \
    // +-----+-----+-----+-----+-----+-----+-----+-----+
    // |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  1  |
    // +-----+-----+-----+-----+-----+-----+-----+-----+
    //   b07   b06   b05   b04   b03   b02   b01   b00
    **/
    imsi_buff[3] = 0x01;
    break;
  case 15:
    /**  Digit #1 -+         odd -+       ID -+
    //   __________|_________     |     ______|______
    //  /                    \    |    /             \
    // +-----+-----+-----+-----+-----+-----+-----+-----+
    // |  0  |  0  |  0  |  0  |  1  |  0  |  0  |  1  |
    // +-----+-----+-----+-----+-----+-----+-----+-----+
    //   b07   b06   b05   b04   b03   b02   b01   b00
    **/
    imsi_buff[3] = 0x09;
    break;
  default:
    printf("\nERROR: Illegal IMSI length, %d characters.\n", imsi_length);
    exit(1);
  }
    
  // IMSI value encoded using BCD, i.e. each digit stored as
  // a nibble, where first BCD digit should be put in DW1, b04-b07.
  // This translates to b04-b07 of index 3 in the buffer.
  
  const int32_t IMSI_OFFSET = 3; // Octet where IMSI starts in imsi_buff
  
  for (int offset=0, nibble=0, word=0, i=1, digit=0; i<=imsi_length; i++) {
    word   = i/4; // Data word relative to IMSI part in buffer
    nibble = i%4; // Nibble within word in IMSI part in buffer
    offset = word*2 - nibble/2 + IMSI_OFFSET;
    
    digit = imsi[i-1]-'0'; // Convert ASCII-encoded digit to integer
    if ((digit<0) || (digit>9)) {
      // A non-digit character has been found; only 0-9 are valid digits
      // in an IMSI value
      printf("\nERROR: Position %d in IMSI contains illegal character '%c'\n\n",
             i, imsi[i-1]);
      exit(1);
    }

    // BCD-encode the digit and store it within the correct nibble in
    // the octet located at the offset in the IMSI buffer.
    if (i%2) {
      // We should store in MSB part of octet (b04 - b07)
      // Due to that storing the nibble in the LSB part of the octet
      // automatically cleared any bits in the MSB part, we just need
      // to shift and do a logical OR to store the bits. Thus the code
      // below is an optimized version of this code in combination with
      // LSB part:
      // 
      //imsi_buff[offset] &= 0x0F; // Clear previous contents
      //imsi_buff[offset] |= (uint8_t)(digit<<4); // Store value
      
      imsi_buff[offset] |= (uint8_t)(digit<<4); // Store value
    }
    else {
      // We should store in LSB part of octet (b00 - b03)
      // Due to that we store the nibble in the LSB part of the octet
      // before the nibble in the MSB part we do not need to consider
      // any value stored in the MSB part. As a side effect we also
      // automatically clear the bits in the MSB part of the octet,
      // i.e. the code below is an optimized version of this code in
      // combination with optimization done for MSB part:
      //
      // imsi_buff[offset] &= 0xF0; // Clear previous contents
      // imsi_buff[offset] |= (uint8_t)(digit); // Store value
      
      imsi_buff[offset] = digit; // Store value
    }
  }
  
  // FIXME: ????
  if (true) {
    printBuffer(imsi_buff, IMSI_LENGTH);
  }
}

//===============================================================================
//      Send request and waits for specified CMN reply
//
//===============================================================================

int send_receive_data(char *buffer, int bytes_to_send, int socket_fd )
{
  int n, number_of_bytes;

  //printf("\n-->send_receive_data: bytes_to_send=%d\n", bytes_to_send);

  // Send a control message on channel 0
  //printf("\n---send_receive_data: sending data\n");
  send_buffer(buffer, bytes_to_send, socket_fd);

  // Read header in reply;
  // buffer[0] and buffer[1] encodes the length of the following message
  // buffer[2] abd buffer[3] encodes the channel used
  // 
  // Channel == 0 contains answer on the control channel
  // Channel == 2 contains event data
  //printf("\n---send_receive_data: reading header\n");
  n = receive_buffer(buffer, HEADER_LENGTH, socket_fd);
  
  // Skip any event data received on channel 2
  while ((buffer[2] == 0) && (buffer[3] == 2)) {
    number_of_bytes = ((unsigned char)buffer[0]*256 +
                       (unsigned char)buffer[1])*2;
    
    if (number_of_bytes > BUFFER_SIZE) {
      printf("\nERROR: Reading from socket event length too long.\n"
             "%10d bytes stated in received event, expected max %d bytes.\n",
             number_of_bytes, BUFFER_SIZE);
      exit(1);
    }
    // Skip the data
    // printf("\n---send_receive_data: skipping data received on channel 2\n");
    n = receive_buffer(buffer, number_of_bytes, socket_fd);
    // Read next header
    // printf("\n---send_receive_data: reading header\n");
    n = receive_buffer(buffer, HEADER_LENGTH, socket_fd);
  }
  
  if (debug) {
    printf("\nFirst four bytes in the reply (NDW and Channel) : ");
    for (int i = 0; i < n ; i++ ) {
      printf("%.2x ", (unsigned char)buffer[i]);
    }
    printf("\n");
    fflush(stdout);
  }
  
  number_of_bytes = ((unsigned char)buffer[0]*256 +
                     (unsigned char)buffer[1])*2;
  
  if (debug) {
    printf("\nNumber of bytes in the reply message = %i\n", number_of_bytes);
    fflush(stdout);
  }
  
  if (number_of_bytes > BUFFER_SIZE) {
    printf("\nERROR: Reading from socket event length too long.\n"
           "%10d bytes stated in received event, expected max %d bytes.\n",
           number_of_bytes, BUFFER_SIZE);
    exit(1);
  }
  
  //printf("\n---send_receive_data: reading payload; expecting %d bytes...\n",
  //       number_of_bytes);
  number_of_bytes = receive_buffer(buffer, number_of_bytes, socket_fd);
  
  //printf("\n<--send_receive_data: number_of_bytes=%d\n", number_of_bytes);
  return number_of_bytes;
}

//===============================================================================
//      Send the buffer to the socket
//
//===============================================================================
int send_buffer(char *buffer, int bytes_to_send, int socket_fd)
{
  int n, i;

  //printf("\n-->send_buffer: bytes_to_send=%d\n", bytes_to_send);

  n = send(socket_fd, buffer, bytes_to_send, 0);
  if (n < 0) {
    printf("ERROR: Writing to socket\n"
           "Reason: %s\n\n", strerror(errno));
    exit(1);
  }
  
  if (debug) {
    printf("\nSent: ");
    for ( i = 0; i < n ; i++ ) {
      printf("%.2x ", (unsigned char)buffer[i]);
    }
    printf("\n");
    fflush(stdout);
  }
  
  //printf("\n<--send_buffer: bytes_sent=%d\n", n);
  return n;
}

int recvtimeout(int s, char *buf, int len, int timeout)
{
  fd_set fds;
  int n;
  struct timeval tv;

  // set up the file descriptor set
  FD_ZERO(&fds);
  FD_SET(s, &fds);

  // set up the struct timeval for the timeout
  tv.tv_sec = timeout;
  tv.tv_usec = 0;

  // wait until timeout or data received
  n = select(s+1, &fds, NULL, NULL, &tv);
  if (n == 0) return -2; // timeout!
  if (n == -1) return -1; // error

  // data must be here, so do a normal recv()
  printf("RECIVE\n");
  fflush(stdout);
  return recv(s, buf, len, 0);
}

// FIXME: Not used!
int receive_buffer_timeout(char *buffer, int bytes_to_receive, int socket_fd)
{
  int n, i;
  n = recvtimeout(socket_fd, buffer, bytes_to_receive, 10);
  if (n == -1) {
    printf("ERROR reading from socket 2...\n");
    exit(1);
  }
  if ( debug ) {
    //printf("Rcvd: ");
    for ( i = 0; i < n ; i++ ) {
      printf("%.2x ", (unsigned char)buffer[i]);
    }
    //printf("\n");
  }
  return n;
}

//===============================================================================
//      Receive data from the socket
//
//===============================================================================
int receive_buffer(char *buffer, const int bytes_to_receive, const int socket_fd)
{
  int delta              = 0;
  int bytes_read         = 0;
  int bytes_left_to_read = bytes_to_receive;

  //printf("\n-->receive_buffer: bytes_to_receive=%d\n", bytes_to_receive);
  
  // Even though we specify that recv should wait for all bytes to appear,
  // there are special cases where this is not so. Due to this we have to
  // loop and re-read until we got everything that was asked for.
  while (bytes_left_to_read > 0) {
    //printf("---receive_buffer: bytes_left_to_read=%d\n", bytes_left_to_read);
    bytes_read = recv(socket_fd, buffer+delta, bytes_left_to_read, MSG_WAITALL);
    //printf("---receive_buffer: bytes_read=%d\n", bytes_read);
    if (bytes_read < 0) {
      printf("\nERROR: Reading from socket.\n"
             "Reason: %s\n\n", strerror(errno));
      exit(1);
    }
    else {
      delta += bytes_read;
      bytes_left_to_read -= delta;
    }    
  }
  
  if (debug) {
    printf("\nRcvd: ");
    for (int i=0; i < bytes_to_receive; i++ ) {
      printf("%.2x ", (unsigned char)buffer[i]);
    }
    printf("\n");
    fflush(stdout);
  }

  //printf("<--receive_buffer\n");
  
  return delta;
}

//===============================================================================
//      Write the eventdata to the file
//
//===============================================================================
void write_to_file(char *buffer,
                   const int number_of_bytes,
                   uint64_t& bytesWritten)
{
  out.write(buffer, number_of_bytes);
  if (out.bad()) {
    int    len     = BASE_DIRECTORY.length()-1;
    string subpath = filename.substr(len, filename.length());

    printf("\nERROR: Write operation to log file\n"
           "%s "
           "failed.\n"
           "Reason: %s\n\n", subpath.c_str(), strerror(errno));
    exit(1);
  }
  
  bytesWritten += (uint64_t)number_of_bytes;
}

//===============================================================================
//      Send a request of the event. Packing of packet with eid and filter
//
//===============================================================================
void request_event_subscription(int eid, int *cell_list, int socket_fd,
                                char *msFlt, int msId, int cmd)
{
  char *buffer;
  int buffPos = 0;
  
  if ((buffer = (char *)malloc(BUFFER_SIZE)) == NULL) {
    printf("\nOut of memory, aborting!\n\n");
    exit(1);
  }
  // Assemble a Subscribe request
  buffer[0] = 0;   // Length.msb
  buffer[1] = 0;   // Length.lsb will be set later
  buffer[2] = 0;   // Channel.msb
  buffer[3] = 0;   // Channel.lsb
  buffer[4] = 0;   // CMN, subscribe on events.msb
  buffer[5] = 11;  // CMN, subscribe on events.lsb
  buffer[6] = (uint8_t)((eid & 0xFF00) >> 8); // EID, msb
  buffer[7] = (uint8_t)(eid & 0xFF); // EID, lsb
  
  // If the EID is one of the System Events, then filtering is not allowed.
  // System Events are defined as EID between 0xFF00 and 0xFFFF, this is
  // captured in the EID_SYSTEM_EVENT_MASK constant.
  //
  // But there are a also two special cases when GMLog application is used
  // that we must check for (the trouble-shooting events for RP and CP)
  if ((eid & EID_SYSTEM_EVENT_MASK) ||
      ((cmd == InvokedAs::GMLog) &&
       ((eid == GMLOG_EID_TROUBLE_SHOOTING_RP) ||
        (eid == GMLOG_EID_TROUBLE_SHOOTING_CP)))) {
    // This eid does not support any filter
    buffer[8] = 0;   // cell pointer list len.msb
    buffer[9] = 0;   // cell pointer list len.lsb
    buffPos = 10;
    buffer[buffPos++] = 0;  // filterlength.msb
    buffer[buffPos++] = 0;  // filterlength.lsb
  }
  else {
    // Check cell option and add cells into message
    if (cell_list[0] == -1) {
      // No cells, set cell pointer list length to zero.
      // Option only available for GMLog
      buffer[8] = 0; // msb
      buffer[9] = 0; // lsb
      buffPos = 10;
    }
    else if (cell_list[0] == 0xFFFF) {
      // All cells. Option only available for RPMO
      buffer[8] = 0xFF;
      buffer[9] = 0xFF;
      buffPos = 10;
    }
    else if ((cell_list[0] != -1) && (cell_list[0] != 0xFFFF)){
      // One or several cells. Several cells only available for RPMO
      int cellIndex = 0;
      buffPos       = 10;

      while (cell_list[cellIndex] != -1) {
        buffer[buffPos++] = (cell_list[cellIndex] & 0xFF00) >> 8; // Cellpointer.msb
        buffer[buffPos++] = (cell_list[cellIndex] & 0xFF);      // Cellpointer.lsb
        cellIndex++;
      }
      // Set cell pointer list length
      buffer[8] = (cellIndex & 0xFF00) >> 8; // Cellpointer list length.msb
      buffer[9] = (cellIndex & 0xFF);        // Cellpointer list length.lsb
    }

    //If any filter on MS id active
    switch (msId) {
    case MsId::IMSI:
      buffer[buffPos++] = 0;                  //filter length.msb
      buffer[buffPos++] = IMSI_FILTER_LENGTH; //filter length.lsb in words
      buffer[buffPos++] = 0;                  //filterid.msb
      buffer[buffPos++] = IMSI_FILTER_ID;     //filterid.lsb,

      // Copy filter into buffer
      for (int i = 0; i < IMSI_LENGTH; i++) {
        buffer[buffPos++] = msFlt[i];
      }
      break;
    case MsId::TLLI:
      buffer[buffPos++] = 0;                  //filter length.msb
      buffer[buffPos++] = TLLI_FILTER_LENGTH; //filter length.lsb
      buffer[buffPos++] = 0;                  //filterid.msb
      buffer[buffPos++] = TLLI_FILTER_ID;     //filterid.lsb

      // Copy filter into buffer
      for (int i = 0; i < TLLI_LENGTH; i++) {
        buffer[buffPos++] = msFlt[i];
      }
      break;
    case MsId::none:
      buffer[buffPos++] = 0;
      buffer[buffPos++] = 0;
      break;
    }
  }

  printf("\nSending Event subscription request for event = %i...", eid);
  fflush(stdout);
  
  buffer[1] = (buffPos/2)-2; // Set length of message in words

  send_receive_data(buffer, buffPos, socket_fd);

  if ((buffer[2] != 0) || (buffer[3] != 0)) {
    printf("\nERROR while subscribeing to event(s):\n");
    switch ( buffer[3] ) {
    case 1:
      printf("The subscription failed for the following cells.");
      break;
    case 3:
      printf("The BSC can not handle the request. Try again later.");
      break;
    case 4:
      printf("The CMN is invalid.");
      break;
    case 6:
      printf("The client is not connected.");
      break;
    case 7:
      printf("Submitted EID is invalid.");
      break;
    case 8:
      printf("Submitted Cell Pointer List is invalid.");
      break;
    case 9:
      printf("The subscription failed. The reason is unknown.");
      break;
    case 10:
      printf("It is not allowed to subscribe to the EID.");
      break;
    case 11:
      printf("Submitted filter is invalid.");
      break;
    case 13:
      printf("High load.");
      break;
    }
    printf("\n");

    if (debug) {
      printf("Dump of received buffer: ");
      for (int i=0, n=(buffPos/2)-2; i < n ; i++ ) {
        printf("%.2x ", (uint8_t)buffer[i]);
      }
      printf("\n");
    }
    fflush(stdout);
    exit(1);
  }
  else {
    printf("ok");
  }
  fflush(stdout);

  free(buffer);

  return;
}

//===============================================================================
//      Check the result from a connection request
//
//===============================================================================
void check_connection_result(char result)
{
  if (result != 0) {
    printf("\nERROR: Connection failed. ");
    switch (result) {
    case 2:
      printf("The client is not authorised to connect.\n");
      break;
    case 3:
      printf("The BSC can not handle the request. Try again later.\n");
      break;
    case 4:
      printf("The CMN is invalid.\n");
      break;
    case 5:
      printf("The client is already connected.\n");
      break;
    default:
      printf("\n");
    }
    exit(1);
  }
}

//===============================================================================
//      Decode the events id arguments. Comma separated to a array of int.
//
//===============================================================================
int decode_event_list(char *events, int *event_list)
{
  int index = 0;
  char * pch;

  pch = strtok(events," ,");
  while (pch != NULL) {
    // The atoi function below will return 0 if presented with a
    // non-integer, and due to this we need to verify that the given
    // event ID is an integer number
    for (int i=0, len=strlen(pch); i<len; i++) {
      if (!isdigit(pch[i])) {
        // We found a non-digit number, return an error code
        printf("\n%s is not a valid Event ID\n\n", pch);
        return -1;
      }
    }
    event_list[index] = atoi(pch);
    index++;
    if (index == 64) {
      printf("\nToo many Event IDs specified; max is %d.\n\n", MAX_EVENT_IDS);
      return -1;
    }
    pch = strtok (NULL, ",");
  }
  event_list[index] = -1; //End of list indication
  
  return 0;
}

//===============================================================================
//      Decode the cellpointer arguments. Comma separated to a array of int.
//
//===============================================================================
int decode_cell_list(char *cells, int *cell_list)
{
  int index = 0;
  char * pch;

  pch = strtok (cells," ,");
  while (pch != NULL) {
    cell_list[index] = atoi(pch);
    index++;
    if (index == MAX_CELLS) {
      return -1;
    }
    pch = strtok(NULL, ",");
  }
  cell_list[index] = -1; //End of list indication

  return 0;
}

//===============================================================================
//      Print Hex buffer
//
//===============================================================================
void printBuffer(char * buff, int len)
{
  printf("Buffer: ");
  for ( int i = 0; i < len ; i++ ) {
    printf("%.2x ", (unsigned char)buff[i]);
  }
  fflush(stdout);
}

//===============================================================================
//      Quit thread function. Listens for "Q" on stdin to quit
//
//===============================================================================
void* quit_request_checker(void* /*pParams*/) //pParams unused
{
  // Setting the 6th bit (0x20) for an uppercase ASCII character
  // results in a lowercase ASCII character. In order to simplify the
  // check, the output from getchar() is always converted to lowercase
  // before checking which character it is.
  //
  // Wait until user presses 'q' or 'Q' followed by <Enter> or <Return>
  for (char ch=0; ch!='q'; ch=(getchar() | (char)0x20)) {
    ;
  }
  
  out.close();
  printf("\nLogging stopped by user\n");
  exit(1);
  
  return NULL;
}

//===============================================================================
//      Prints statistics periodically
//
//===============================================================================
void *print_statistics(void* /*pParams*/) // pParams unused
{
  time_t start_time_sec = time(NULL);
  while (true) {
    printf("Events: %10d  FileSize: %7lu KB\r", numberOfEvents, bytesWritten/1000);
    fflush(stdout);
    
    usleep(1000000); // Sleep for 1 second
    
    if ((start_time_sec + maxLoggingTime) < time(NULL)) {
      out.close();
      printf("\nMax logging time exceeded. Logging Stopped\n");
      fflush(stdout);
      exit(1);
    }
  }
  
  return NULL;
}

//===============================================================================
//      Prints usage text
//
//===============================================================================
void print_usage(int cmd)
{
  // String literals are automatically concatenated
  printf("Event Handling Client version: " VERSION "\n\n");

  if (cmd == InvokedAs::GMLog){
    print_gmlog_help_txt();
  }
  else if (cmd == InvokedAs::RPMO){
    print_rpmo_help_txt();
  }
  else {
    printf("Event Handling Client invoked using unknown application name\n\n");
    printf("Please use one of:\n");
    printf("evhandlclient gmlog <options>\n");
    printf("evhandlclient rpmo <options>\n\n");
  }

  exit(1);
}

//===============================================================================
//      Prints GMLog help text
//
//===============================================================================
void print_gmlog_help_txt()
{
  printf("gmlog <ip> <port> <eid,eid,...> -c <cellind>\n"
         "      [-f <file>] [-s <maxFileSize>] [-h <maxLoggingTime>]\n");
  printf("gmlog <ip> <port> <eid,eid,...> -i <imsi>\n"
         "      [-f <file>] [-s <maxFileSize>] [-h <maxLoggingTime>]\n");
  printf("gmlog <ip> <port> <eid,eid,...> -t <tlli>\n"
         "      [-f <file>] [-s <maxFileSize>] [-h <maxLoggingTime>]\n");
  printf("\n");
  printf("<ip>              IP address is found with BSC MML command: RRAPP:APL=GML\n");
  printf("<port>            Port is found with BSC MML command: RRPPP:APL=GML\n");
  printf("<eid,eid,...>     List of Event IDs to subscribe to (max %d)\n",
         MAX_EVENT_IDS);
  printf("<file>            Output file name to use, must end with .gml\n");
  printf("<maxFileSize>     Automatically stop when reaching specified size in MB,\n"
         "                  max is 1000 MB\n");
  printf("<maxLoggingTime>  Automatically stop after specified amount of minutes,\n"
         "                  max is 60 minutes\n");
  printf("<cellind>         Cell indicator to subscribe to.\n");
  printf("<imsi>            IMSI to subscribe to (14 or 15 digits)\n");
  printf("<tlli>            TLLI to subscribe to\n");
  printf("\n");
  printf("file, maxFileSize (MB) and maxLoggingTime (Minutes) are optional\n");
  printf("Default values are logfile_gmlog.gml, 10GB, 1Hour, respectively\n");
  printf("\n");
  printf("Output file located in /%s\n", DESTINATION_DIRECTORY.c_str());
  printf("\n");
  printf("\nExample with IMSI filter: \n");
  printf("gmlog 150.132.96.187 6000 0,1,2,11,12 -i 123456789101112\n");
  printf("\nExample with cell filter: \n");
  printf("gmlog 150.132.96.187 6000 0,1,2,3,4,5,6,7,8,9,10 -c 12\n");
  printf("\nExample with TLLI filter: \n");
  printf("gmlog 150.132.96.187 6000 0,1,2,11,12 -t 1234567\n");
  printf("\n");
  printf("For Event ID list information, please see GMLog IWD\n\n");
}
//===============================================================================
//      Prints R-PMO help text
//
//===============================================================================
void print_rpmo_help_txt()
{
  printf("rpmo <ip> <port> <eid,eid,...> -c <cellind>\n"
         "     [-f <file>] [-s <maxFileSize>] [-h <maxLoggingTime>]\n");
  printf("rpmo <ip> <port> <eid,eid,...> -c <all>\n"
         "     [-f <file>] [-s <maxFileSize>] [-h <maxLoggingTime>]\n");
  printf("\n");
  printf("<ip>              IP address is found with BSC MML command: RRAPP:APL=RPM\n");
  printf("<port>            Port is found with BSC MML command: RRPPP:APL=RPM\n");
  printf("<eid,eid,...>     List of Event IDs to subscribe to (max %d)\n",
         MAX_EVENT_IDS);
  printf("<file>            Output file name to use, must end with .gml\n");
  printf("<maxFileSize>     Automatically stop when reaching specified size in MB,\n"
         "                  max is 1000 MB\n");
  printf("<maxLoggingTime>  Automatically stop after specified amount of minutes,\n"
         "                  max is 60 minutes\n");
  printf("<cellind>         List of cell indicators to subscribe to (max %d)\n",
         MAX_CELLS);
  printf("<all>             Cell indicator 65535 used for selecting all cells\n");
  printf("\n");
  printf("file, maxFileSize (MB) and maxLoggingTime (Minutes) are optional\n");
  printf("Default values are logfile_rpmo.rpm, 10GB, and 1 Hour, respectively\n");
  printf("\n");
  printf("Output file located in /%s\n", DESTINATION_DIRECTORY.c_str());
  printf("\n");
  printf("Example with cell filter: \n");
  printf("rpmo 150.132.96.187 22000 0,1,2,3,5,6,7,8,9,10 -c 12,13,14\n");
  printf("\nExample with all cells: \n");
  printf("rpmo 150.132.96.187 22000 0,1,2,3,5,6,7,8,9,10 -c 65535\n");
  printf("\n");
  printf("For Event ID list information, please see R-PMO IWD\n\n");
}
