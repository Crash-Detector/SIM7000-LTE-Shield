
#include "HAL_FONA.h"


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

bool begin( Cellular_module_t * const cell_ptr )
    {
    
    if ( cell_ptr->uart_ptr )
        {
        
        printf( "Attempting to open comm with ATs" );

        int16_t timeout = 7000;

        while( timeout )
            {
            
            while(  )
            
            HAL_Delay( 500 );
            timeout -= 500;
            } // end while

        } // end if
    return false;
    } // end begin( )

void flushInput( UART_HandleTypeDef * const uart_ptr )
    {
    char c;
    while( HAL_UART_Receive( uart_ptr, (uint8_t *)&c_in, 1, 1 ) == HAL_OK );
    } // end flush_Input

bool send_check_reply( Cellular_module_t * const cell_ptr, char const * const send, 
                        char const const * const reply, const uint16_t timeout )
    {
    uint8_t idx;
    
    flushInput( cell_ptr->uart_ptr );
    printf( "\t---> %c\n", send );
    HAL_UART_Transmit( cell_ptr->uart_ptr, ( uint8_t *) send, strlen( send ), timeout );

    idx = readline( cell_ptr, false, timeout );
    return idx;
    } // end send_check_reply( )

bool transmitWReply( Cellular_module_t * const cell_ptr, char const * const send );
    {

    } // end 

/*
 * REQUIRES: N/A
 * MODIFIES: Cellular_module_t::replay_buffer (where reply exists)
 *  EFFECTS: Receives characters from UARTS Rx pin (from the Nucleo board's perspective)
 *           up to the first \r\n and places it into the reply_buffer.
*/
uint8_t readline( Cellular_module_t * const cell_ptr, boolean multiline, uint16_t timeout )
    {
    uint16_t replyidx = 0;
    HAL_StatusTypeDef ret_type;
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
                if ( c_in != '\r' ) // Skip the carrage return character.
                    {
                    if ( c_in == '\n' && replyidx == 0 )
                        c_in = 0; // As to not trigger while loop condition
                    else
                        cell_ptr->reply_buffer[ replyidx++ ] = c_in;
                    } // end if
                } // end while
            // Decrement on a non_HAL_OK
            if ( ret_type != HAL_OK )
                --timeout;
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


boolean begin(  )


/*
 typedef enum
{
  HAL_OK       = 0x00,
  HAL_ERROR    = 0x01,
  HAL_BUSY     = 0x02,
  HAL_TIMEOUT  = 0x03
} HAL_StatusTypeDef;

 */