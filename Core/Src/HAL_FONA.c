
#include "HAL_FONA.h"
#include <stdio.h>
#include <string.h>

char const * const ok_reply_c = "OK";
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

        int16_t timeout = 7000;

        while( timeout )
            {
            flushInput( cell_ptr->uart_ptr );
            if ( send_check_reply( cell_ptr, "AT", ok_reply_c, FONA_DEFAULT_TIMEOUT_MS ) )
                break;
            flushInput( cell_ptr->uart_ptr );

            if ( send_check_reply( cell_ptr, "AT", "AT", FONA_DEFAULT_TIMEOUT_MS ) )
                break;

            HAL_Delay( 500 );
            timeout -= 500;
            } // end while
        
        if( timeout <= 0 )
            {
            printf( "Timed out!\n\r" );
            return false;
            } // end if


        // Turn off Echo
        send_check_reply( cell_ptr, "ATE0", ok_reply_c, FONA_DEFAULT_TIMEOUT_MS );
        HAL_Delay( 100 );

        if ( send_check_reply( cell_ptr, "ATE0", ok_reply_c, FONA_DEFAULT_TIMEOUT_MS ) )
            {
            HAL_Delay( 100 );
            flushInput( cell_ptr->uart_ptr );
            
            printf( "\t---> AT+GMR\n" );

            println( cell_ptr->uart_ptr, "AT+GMR" );
            readline( cell_ptr, 500, true );

            printf( "\t<--- %s\n", cell_ptr->reply_buffer );
            
            if ( strstr( cell_ptr->reply_buffer, "SIM7000A" ) != NULL )
                {
                char buffer[ 1024 ];
                sprintf( buffer,  "AT+CPMS=%s,%s,%s", "\"SM\"", "\"SM\"", "\"SM\"" );
                send_check_reply( cell_ptr, buffer, ok_reply_c, FONA_DEFAULT_TIMEOUT_MS );
                return true;
                } // end if
            else 
                printf( "COuldn't find right revision!\n");
            } // end if
        } // end if
    return false;
    } // end begin( )


bool send_check_reply( Cellular_module_t * const cell_ptr, char const * const send, 
                        char const * const reply, const uint16_t timeout )
    {

    return transmit( cell_ptr, send, timeout ) != REPLY_BUFF_SIZE && 
           !strcmp( cell_ptr->reply_buffer, reply );
    } // end send_check_reply( )

uint8_t transmit( Cellular_module_t * const cell_ptr, char const * const send, uint16_t timeout )
    {
    char send_buff[1024];
    uint8_t idx;
    if ( sprintf( send_buff, "%s\r\n", send ) < 0 ) // At in <CR><LR>
        {
        printf( "Failed to put into sprintf\n\r" );
        return FONA_DEFAULT_TIMEOUT_MS;
        } // end if
    
    flushInput( cell_ptr->uart_ptr );
    printf( "\t---> %s\n", send );

    HAL_UART_Transmit( cell_ptr->uart_ptr, ( uint8_t *) send_buff, strlen( send_buff ), timeout );

    idx = readline( cell_ptr, timeout, false );
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
    uint16_t replyidx = 0;
    HAL_StatusTypeDef ret_type = HAL_OK;
    const int iterations = multiline ? 2 : 1;

    // Multiline ensures that we go e
    for ( int n = 0; n < iterations && ret_type != HAL_ERROR; ++n )
        {
        while( timeout && ret_type != HAL_ERROR ) 
            {
            char c_in = 0;  // Read into from

            // Continue until a newline is received.
            while( c_in != '\n' && ( ret_type = HAL_UART_Receive( cell_ptr->uart_ptr, (uint8_t *)&c_in, 1, 1 ) ) == HAL_OK )
                {
                // Used to skip the first <CR><LR> in a response.
                if ( c_in != '\r' ) // Skip the carrage return character (This is present in responses).
                    {
                    if ( c_in == '\n' )  // Don't insert the <LR> into the return buffer.
                        {
                        if ( !replyidx ) // Typcially used to skip first <LR>
                            c_in = 0; // As to not trigger while loop condition
                        } // end if
                    else
                        {
                        cell_ptr->reply_buffer[ replyidx++ ] = c_in;
                        } // end else
                    } // end if
                } // end while
            // Decrement on a non_HAL_OK (b/c otherwise c_in == '\n')
            if ( ret_type != HAL_OK )
                --timeout;
            if ( ret_type == HAL_TIMEOUT )
               {
            	//printf( "Timed out\n\r" );
               } // end if

            } // end while
        } // end for
    if ( ret_type == HAL_ERROR || !timeout )
        {
        return REPLY_BUFF_SIZE; // Invalid buffer size.
        } // end if

    // Pad the last bit with a null character
    cell_ptr->reply_buffer[ replyidx ] = 0;
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
    while( HAL_UART_Receive( uart_ptr, (uint8_t *)&c_in, 1, 1 ) == HAL_OK );
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
