
#include "HAL_FONA.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })


char const * const ok_reply_c = "OK";
const int reply_buff_size_c = 256;
const int fona_def_timeout_ms_c = 500;

const HAL_GPIO_t pwr_pin = { GPIOE, GPIO_PIN_9 };
const HAL_GPIO_t rst_pin = { GPIOF, GPIO_PIN_13 };

// Prints characters \r and \n in a way that doesn't case issues.
void print_char( const char ch )
    {
	char buff[5];
	if ( ch == '\n' )
		sprintf( buff, "<LR>" );
	else if ( ch == '\r' )
		sprintf( buff, "<CR>" );
	else
		sprintf( buff, "%c", ch );
	printf( "%s 0x%x D:%d\n\r", buff, ch, ch );
    } // end print_char

void GPIO_Write( HAL_GPIO_t const * const gpio_ptr, const GPIO_PinState pin_state )
    {
    HAL_GPIO_WritePin( gpio_ptr->GPIOx, gpio_ptr->GPIO_Pin, pin_state );
    } // GPIO_Write( )

/*
int available( Cellular_module_t * cell_mod_ptr )
    {
    while( uart_ptr )
    } // end available( )
*/
//------------------------------------------------------------------------------------------------
//
//                                  Cellular_module_t Definitions
//
//------------------------------------------------------------------------------------------------

/*
 * The basic AT Command Sysntax is as follows:
 * <AT | at | aT | At ><COMMAND><CR>
 * 
 * Responses:
 *  <CR><LF><response><CR><LF>
*/
bool begin( Cellular_module_t * const cell_ptr )
    {
    
    if ( cell_ptr->uart_ptr )
        {
        printf( "Attempting to open comm with ATs\n\r" );

        int16_t timeout = 14000;

        while( timeout > 0 )
            {
            flushInput( cell_ptr->uart_ptr );
            if ( send_check_reply( cell_ptr, "AT", ok_reply_c, fona_def_timeout_ms_c ) )
                break;
            //printf( "Failed \n\r\n\r" );
            flushInput( cell_ptr->uart_ptr );

            if ( send_check_reply( cell_ptr, "AT", "AT", fona_def_timeout_ms_c ) )
                break;
            // printf( "Failed \n\r\n\r" );
            HAL_Delay( 500 );
            timeout -= 500;
            } // end while
        
        if( timeout <= 0 )
            {
            printf( "Timed out!\n\r" );
            return false;
            } // end if


        // Turn off Echo
        send_check_reply( cell_ptr, "ATE0", ok_reply_c, fona_def_timeout_ms_c );
        HAL_Delay( 100 );

        if ( send_check_reply( cell_ptr, "ATE0", ok_reply_c, fona_def_timeout_ms_c ) )
            {
            HAL_Delay( 100 );
            flushInput( cell_ptr->uart_ptr );
            
            printf( "\t---> AT+GMR\n" );

            transmit( cell_ptr,  "AT+GMR" , timeout );

            printf( "\t<--- %s\n", cell_ptr->reply_buffer );
            
            // Nucleo confirms operating with right Cell Module.
            if ( strstr( cell_ptr->reply_buffer, "SIM7000A" ) != NULL )
                {
                char buffer[ 32 ];
                sprintf( buffer,  "AT+CPMS=%s,%s,%s", "\"SM\"", "\"SM\"", "\"SM\"" );
                send_check_reply( cell_ptr, buffer, ok_reply_c, fona_def_timeout_ms_c );
                return true;
                } // end if
            else 
                printf( "Couldn't find right revision!\n");
            } // end if
        } // end if
    	return false;
    } // end begin( )


bool send_check_reply( Cellular_module_t * const cell_ptr, char const * const send, 
                        char const * const reply, const uint16_t timeout )
    {

    return transmit( cell_ptr, send, timeout ) != reply_buff_size_c &&
           !strcmp( cell_ptr->reply_buffer, reply );
    } // end send_check_reply( )

uint8_t transmit( Cellular_module_t * const cell_ptr, char const * const send, uint16_t timeout )
    {
	*cell_ptr->reply_buffer = '\0';
    char send_buff[1024];
    uint8_t idx;
    if ( sprintf( send_buff, "%s\r", send ) < 0 ) // At in <CR><LR>
        {
        printf( "Failed to put into sprintf\n\r" );
        return reply_buff_size_c;
        } // end if
    
    flushInput( cell_ptr->uart_ptr );

#ifdef DEBUG_CELL
    printf( "\t---> %s\n\r", send );
#endif


    if ( HAL_UART_Transmit( cell_ptr->uart_ptr, ( uint8_t *) send_buff, strlen( send_buff ), timeout ) == HAL_OK )
        {
        idx = readline( cell_ptr, timeout, false );
#ifdef DEBUG_CELL
        printf( "Got: %s\n\r", cell_ptr->reply_buffer );
#endif
        } // end if
    else
        {
#ifdef DEBUG_CELL
        printf( "Failed Transmit\n\r" );
#endif
        idx = reply_buff_size_c;
        } // end else

    return idx;
    } // transmit( )


/*
 * REQUIRES: N/A
 * MODIFIES: Cellular_module_t::replay_buffer (where reply exists)
 *  EFFECTS: Receives characters from UARTS Rx pin (from the Nucleo board's perspective)
 *           up to the first (if multiline == false, else second) \r\n and places it into the reply_buffer.
*/
uint8_t readline( Cellular_module_t * const cell_ptr, const uint16_t timeout, const bool multiline )
    {
    static char receive_buff[ 1024 ];
    static char const * const buff_end_c = receive_buff + sizeof( receive_buff );

    char *buff_ptr = receive_buff;  //! buff_ptr will eventually point to the end of the bytes received.
    char *reply_ptr = cell_ptr->reply_buffer; // Points to the reply_buffer
    uint16_t newlines_seen;
    newlines_seen = 0;

    // Multiline ensures that we check newline twice
    const int iter_c = multiline ? 2 : 1;

    // Receive everything until we time out OR run out of space
    while( buff_ptr != buff_end_c && HAL_UART_Receive( cell_ptr->uart_ptr, (uint8_t *)( buff_ptr ), 1, timeout ) == HAL_OK )
        {
        ++buff_ptr;    // Increment pointer
        } // end while
#ifdef DEBUG_CELL
    for ( char const *ptr = receive_buff, * const end_ptr = buff_ptr; ptr != end_ptr; ++ptr )
    	print_char( *ptr );
    if ( buff_ptr == buff_end_c )
        printf( "Ran out of space in receive_buff\n\r" );
#endif

    for ( char const *ptr = receive_buff, *const end_ptr = buff_ptr;
        ptr != end_ptr && newlines_seen < iter_c; ++ptr )
        {
        const char c_in = *ptr;
        // Used to skip the first <CR><LR> in a response.
        if ( c_in != '\r' ) // Skip the carrage return character (This is present in responses).
            {
            if ( c_in == '\n' )  // Don't insert the <LR> into the return buffer.
                {
                if ( reply_ptr != cell_ptr->reply_buffer )
                    ++newlines_seen;
                //Else Don't count first <LR> seen (before anything's been inserted)
                } // end if
            else
                {
                *reply_ptr++ = c_in; // Dereferences, assigns, and then post_increment.
                } // end else
            } // end if
        } // end for
    if ( reply_ptr == cell_ptr->reply_buffer + sizeof( cell_ptr->reply_buffer ) )
        {
        printf( "Reply exceeded buffer size!\n\r" );
        reply_ptr = cell_ptr->reply_buffer + sizeof( cell_ptr->reply_buffer ) - 1; // To prevent out of bounds indexing.
        } // end if
    *reply_ptr = '\0'; // Null-terminate
    return ( uint8_t)( reply_ptr - cell_ptr->reply_buffer );
    } // end readline( )

//------------------------------------------------------------------------------------------------
//
//                                  UARTS Functions Definition
//
//------------------------------------------------------------------------------------------------

// Clear everything W/O reading it in.
void flushInput( UART_HandleTypeDef * const uart_ptr )
    {
    char c_in;
#ifdef DEBUG_CELL
    printf( "Flushing Input\n\r" );
#endif
    while( HAL_UART_Receive( uart_ptr, (uint8_t *)&c_in, 1, 100 ) == HAL_OK )
        {
#ifdef DEBUG_CELL
        print_char( c_in );
#endif
        } // end while
    } // end flush_Input

void println( UART_HandleTypeDef * const uart_ptr, const char * const message )
    {
    HAL_UART_Transmit( uart_ptr, ( uint8_t * ) message, strlen( message ), 0xFFFF );
    } // end println( )
/*
 typedef enum
{
  HAL_OK       = 0x00,
  HAL_ERROR    = 0x01,
  HAL_BUSY     = 0x02,
  HAL_TIMEOUT  = 0x03
} HAL_StatusTypeDef;

 */

//------------------------------------------------------------------------------------------------
//
//                                  Network Settings
//
//------------------------------------------------------------------------------------------------

bool setNetworkSettings(Cellular_module_t * const cell_ptr){

	//if ( send_check_reply( cell_ptr, "AT+CGDCONT=1,\"IP\",\"hologram"", ok_reply_c, 10000 ) )
	flushInput( cell_ptr->uart_ptr );
	if ( send_check_reply( cell_ptr, "AT+CGDCONT=1,\"IP\",\"hologram\"", ok_reply_c, 10000 ) ){
		return true;
	}
	return false;
}

bool sendSMS(Cellular_module_t * const cell_ptr){

	flushInput( cell_ptr->uart_ptr );

	if ( !send_check_reply( cell_ptr, "AT+CMGF=1", ok_reply_c, fona_def_timeout_ms_c ) ){
	printf("Failed sendCheckReply");
	return false;
	}

	//if (! sendCheckReply("AT+CMGS=\"18024246417\"", "> ")) return false;
	if ( !send_check_reply( cell_ptr, "AT+CMGS=\"18024246417\"", "> ", fona_def_timeout_ms_c ) ){
	printf("Failed establishing phone address");
	return false;
	}

	char *pass_buff = "eecs373";
	uint8_t buffer_pass[1024];
	sprintf(buffer_pass,"%s%c",pass_buff,26);
	HAL_UART_Transmit( cell_ptr->uart_ptr, ( uint8_t * ) buffer_pass, strlen( buffer_pass ), fona_def_timeout_ms_c );
	//transmit( cell_ptr,  "hi" , fona_def_timeout_ms_c );
	//transmit( cell_ptr,  &pass_buff , fona_def_timeout_ms_c );
	readline(cell_ptr, 200, false);

	readline(cell_ptr, 200, false);

	readline(cell_ptr, 30000, false);

	if (strstr(cell_ptr->reply_buffer, "+CMGS") == 0) {
		return false;
	}

	readline(cell_ptr, 1000, false);


	if (strcmp(cell_ptr->reply_buffer, "OK") != 0) {
	    return false;
	}

	return true;

}
//------------------------------------------------------------------------------------------------
//
//                                  GPS Settings
//
//------------------------------------------------------------------------------------------------

bool enableGPS(Cellular_module_t * const cell_ptr, bool onoff) {

	uint8_t state;
	//if (! sendParseReply(F("AT+CGNSPWR?"), F("+CGNSPWR: "), &state) )
    //return false;
	//if ( !send_check_reply( cell_ptr, "AT+CGNSPWR?", "+CGNSPWR: ", fona_def_timeout_ms_c ) ){}
	uint8_t ret = transmit( cell_ptr,  "AT+CGNSPWR?" , fona_def_timeout_ms_c);

	printf ("%d\n", ret);

	if (strstr(cell_ptr->reply_buffer, "+CGNSPWR: 0") == 0) {
		state =1;
	}
	else state =0;

	if (onoff && !state) {
		//if (! sendCheckReply(F("AT+CGNSPWR=1"), ok_reply))
		if ( !send_check_reply( cell_ptr, "AT+CGNSPWR=1", ok_reply_c, fona_def_timeout_ms_c ) ){
        return false;
		}
	}

	else if (!onoff && state) {
	if ( !send_check_reply( cell_ptr, "AT+CGNSPWR=0", ok_reply_c, fona_def_timeout_ms_c ) ){
		return false;
		}
	}

	return true;
}

uint8_t get_GPS(Cellular_module_t * const cell_ptr, uint8_t arg, char *buffer, uint8_t maxbuff){

	transmit( cell_ptr,  "AT+CGNSINF" , fona_def_timeout_ms_c);

	char *p = strstr(cell_ptr->reply_buffer, "SINF");

	printf("%s\n",cell_ptr->reply_buffer );

	if (p == 0) {
	    buffer[0] = 0;
	    return 0;
	  }

	  p+=6;

	  uint8_t len = max(maxbuff-1, (int)strlen(p));
	  strncpy(buffer, p, len);
	  buffer[len] = 0;

	  //readline(); // eat 'OK'
	  return len;

	}

bool gGPS(Cellular_module_t * const cell_ptr ,float *lat, float *lon)
        {

	flushInput( cell_ptr->uart_ptr );

	char gpsbuffer[120];

	uint8_t res_len = get_GPS(cell_ptr, 32, gpsbuffer, 120);

	if (res_len == 0) {
	    return false;
	}

	const int commas_to_see = 5;
	int commas_seen = 0;

	for ( const char *gps_start_ptr = gpsbuffer, * const gps_end = gpsbuffer + res_len;
				gps_start_ptr != gps_end && commas_seen != commas_to_see; ++commas_seen )
		{
		const char *gps_ptr = ++gps_start_ptr; // Skips comma (All except the first execution)
		while( gps_ptr != gps_end && *gps_ptr != ',' ) { ++gps_ptr; } // Skipping until finds comma
		if ( commas_seen == 3 ) // Seen the <gps status, run status, time>
			{
			*lat = ( gps_ptr != gps_start_ptr ) ? atof( gps_start_ptr ) : 0.0;
			} // end if
		else if ( commas_seen == 4 ) // Seen the <gps status, run status, time, latitude>
			{
			*lon = ( gps_ptr != gps_start_ptr ) ? atof( gps_start_ptr ) : 0.0;
			} // end else if
		gps_start_ptr = gps_ptr;
		} // end for
	
	return commas_seen == commas_to_see;
	}
