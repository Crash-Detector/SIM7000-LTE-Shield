
#ifndef HAL_FONA_H
#define HAL_FONA_H

enum boolean { false, true };
#include <stdbool.h>
#include <assert.h>
#include <cstddef> // For size_t

char const * const ok_reply_c = "OK";

#define BUFF_SIZE 32
#define REPLY_BUFF_SIZE 256

typedef struct {
    UART_HandleTypeDef *uart_ptr;
    char reply_buffer[ 256 ];
} Cellular_module_t;

typedef struct {
    size_t s_elem, n_elem;
    uint8_t buf[BUFF_SIZE];
    volatile size_t head;
    volatile size_t tail;
} Ring_Buffer_t;


bool begin( Cellular_module_t * const cell_ptr );

int available( Cellular_module_t * const cell_mod_ptr );
size_t write(Cellular_module_t * const cell_mod_ptr, uint8_t x);
boolean read( Cellular_module_t * const cell_mod_ptr );
int peek(void);
void flush();

boolean parseReply(  Cellular_module_t * const cell_mod_ptr, FONAFlashStringPtr toreply,
        uint16_t *v, char divider  = ',', uint8_t index=0);
boolean parseReplyFloat( Cellular_module_t * const cell_mod_ptr, FONAFlashStringPtr toreply,
           float *f, char divider, uint8_t index);
boolean parseReply(FONAFlashStringPtr toreply,
           char *v, char divider  = ',', uint8_t index=0);
boolean parseReplyQuoted(FONAFlashStringPtr toreply,
           char *v, int maxlen, char divider, uint8_t index);

// Network connection (AT+CNACT)
boolean openWirelessConnection(bool onoff);
boolean wirelessConnStatus(void);

// GPS handling
boolean enableGPS(boolean onoff);
int8_t GPSstatus(void);
uint8_t getGPS(uint8_t arg, char *buffer, uint8_t maxbuff);
boolean getGPS(float *lat, float *lon, float *speed_kph=0, float *heading=0, float *altitude=0);
//boolean getGPS(float *lat, float *lon, float *speed_kph, float *heading, float *altitude,
// uint16_t *year = NULL, uint8_t *month = NULL, uint8_t *day = NULL, uint8_t *hour = NULL, uint8_t *min = NULL, float *sec = NULL);
boolean enableGPSNMEA(uint8_t nmea);

// Power, battery, and ADC
void powerOn(uint8_t FONA_PWRKEY);
boolean powerDown(void);
boolean getADCVoltage(uint16_t *v);
boolean getBattPercent(uint16_t *p);
boolean getBattVoltage(uint16_t *v);

// Functionality and operation mode settings
boolean setFunctionality(uint8_t option); // AT+CFUN command
boolean enableSleepMode(bool onoff); // AT+CSCLK command

// SIM query
uint8_t unlockSIM(char *pin);
uint8_t getSIMCCID(char *ccid);
uint8_t getNetworkStatus(void);

// SMS handling
boolean setSMSInterrupt(uint8_t i); // Probably can't be used since we can't see.
uint8_t getSMSInterrupt(void);      
int8_t getNumSMS(void);
boolean readSMS(uint8_t i, char *smsbuff, uint16_t max, uint16_t *readsize);
boolean sendSMS(const char *smsaddr, const char *smsmsg); //! Needed
boolean deleteSMS(uint8_t i);
boolean deleteAllSMS(void);
boolean getSMSSender(uint8_t i, char *sender, int senderlen);
boolean sendUSSD(char *ussdmsg, char *ussdbuff, uint16_t maxlen, uint16_t *readlen);

#endif // HAL_FONA_H