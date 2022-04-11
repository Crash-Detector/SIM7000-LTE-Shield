
#include "HAL_FONA.h"
#include <stdio.h>
#include <string.h>

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
                char buffer[ 1024 ];
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
        return fona_def_timeout_ms_c;
        } // end if
    
    flushInput( cell_ptr->uart_ptr );
    printf( "\t---> %s\n\r", send );

    if ( HAL_UART_Transmit( cell_ptr->uart_ptr, ( uint8_t *) send_buff, strlen( send_buff ), timeout ) == HAL_OK )
       {
       idx = readline( cell_ptr, timeout, false );
       printf( "Got: %s\n\r", cell_ptr->reply_buffer );
       } // end if
    else
       {
    	printf( "Failed Transmit\n\r" );
    	idx = reply_buff_size_c;
       } // end else

    return idx;
    } // transmit( )


/*
 * REQUIRES: N/A
 * MODIFIES: Cellular_module_t::replay_buffer (where reply exists)
 *  EFFECTS: Receives characters from UARTS Rx pin (from the Nucleo board's perspective)
 *           up to the first \r\n and places it into the reply_buffer.
*/
uint8_t readline( Cellular_module_t * const cell_ptr, uint16_t timeout, bool multiline )
    {
    static char receive_buff[ 1024 ];
    char * buff_ptr = receive_buff;
    uint16_t bytes_recvd, replyidx, newlines_seen;
    bytes_recvd = replyidx = newlines_seen = 0;
    const int iterations = multiline ? 2 : 1;

    // Multiline ensures that we check newline twice

    // Receive as much as possible
    while( HAL_UART_Receive( cell_ptr->uart_ptr, (uint8_t *)( buff_ptr++ ), 1, timeout ) == HAL_OK ) // Keep going
        {
        ++bytes_recvd; // Count bytes received
        } // end while
    for ( char const * ptr = receive_buff, *end_ptr = receive_buff + bytes_recvd; ptr != end_ptr; ++ptr )
    	print_char( *ptr );

    for ( int idx = 0, newlines_seen = 0; idx < bytes_recvd && newlines_seen < iterations; ++idx )
        {
    	const char c_in = receive_buff[ idx ];
        // Used to skip the first <CR><LR> in a response.
        if ( c_in != '\r' ) // Skip the carrage return character (This is present in responses).
            {
            if ( c_in == '\n' )  // Don't insert the <LR> into the return buffer.
                {
                if ( replyidx ) // Used to skip first <LR> (and not count as seen)
                    ++newlines_seen;
                } // end if
            else
                {
                cell_ptr->reply_buffer[ replyidx++ ] = c_in;
                } // end else
            } // end if
        } // end for

    cell_ptr->reply_buffer[ replyidx ] = '\0'; // Null-terminate
    return replyidx;
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
    printf( "Flushing Input\n\r" );
    while( HAL_UART_Receive( uart_ptr, (uint8_t *)&c_in, 1, 100 ) == HAL_OK )
    	print_char( c_in );
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
