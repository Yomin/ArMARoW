/*******************************************************************************
 *
 * Copyright (c) 2010 Thomas Kiebel <kiebel@ivs.cs.uni-magdeburg.de>
 * All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions
 *    are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *
 *    * Neither the name of the copyright holders nor the names of
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * $Id$
 *
 ******************************************************************************/
/*! \file   examples/applixation/sniffer.cc
 *  \brief  Example implementation of a sniffer on the physical layer.
 */
/* === includes ============================================================= */
/*#include "platform-cfg.h"               // platform dependent software config
#include "avr-halib/share/delay.h"      // delays and timings

#include "armarow/armarow.h"            // main ArMARoW include
#include "armarow/debug.h"              // ArMARoW logging and debugging
#include "armarow/phy/phy.h"            // physical layer
*/


/*
void* operator new (size_t , void* buffer){

return buffer;

} 

#include "mac_message.h"
*/

//#include "mac.h"
#include "mac_csma_ca.h"


/* === globals ============================================================== */
platform::config::mob_t message = {0,{0}};
//platform::config::rc_t  rc;             // radio controller

//armarow::MAC::MAC_Base mac;
armarow::MAC::MAC_CSMA_CA mac;
armarow::MAC::mob_t messageobject;

uint8_t channel = 11;                   // channel number
/* === functions ============================================================ */
/*! \brief  Callback triggered by an interrupt of the radio controller.
 *  \todo   Add Information for LQI and RSSI values.
 */


//erst notifizieren, und dann receive aufrufen, wo man receive puffer übergibt
void callback_recv() {

 	if(mac.receive(messageobject)!=0){
		/*
		struct mesg{
			uint32_t counter;
			char message[];
		};
		
		::logging::log::emit()
        	<< PROGMEMSTRING("[Content:] ") << ((mesg*)&messageobject.payload)->message
		<< PROGMEMSTRING("Message Number: ") << ((mesg*)&messageobject.payload)->counter
        	<< ::logging::log::endl << ::logging::log::endl;
		*/
		
		::logging::log::emit()
        	<< PROGMEMSTRING("[Content:] ") << messageobject.payload << ::logging::log::endl
		<< PROGMEMSTRING("Message Sequence Number: ") << (int) messageobject.header.sequencenumber
        	<< ::logging::log::endl << ::logging::log::endl;

	}else{

		::logging::log::emit()
        	<< PROGMEMSTRING("Failed receiving message!") 
        	<< ::logging::log::endl << ::logging::log::endl;

	}

}
/*! \brief  Initializes the physical layer.*/
void test_asynchron_receive() {

    mac.onMessageReceiveDelegate.bind<callback_recv>();
    while(1);
}
/* === main ================================================================= */
int main() {

    

    sei();                              // enable interrupts
    ::logging::log::emit()
        << PROGMEMSTRING("Starting sniffer!")
        << ::logging::log::endl << ::logging::log::endl;

    test_asynchron_receive();                            

   //sychron receive test
   





    do {                                // duty cycle
        //delay_ms(1000);
    //::logging::log::emit()
    //    << PROGMEMSTRING("Starting sniffer!")
    //    << ::logging::log::endl << ::logging::log::endl;

	/*
		int numberofreceivedbytes = mac.receive(buffer,buffersize);

		 ::logging::log::emit()
        << PROGMEMSTRING("Number of Received bytes: ") << numberofreceivedbytes
        << ::logging::log::endl << ::logging::log::endl;

	*/

	//messageobject.header.printFrameFormat();

	if(mac.receive(messageobject)!=0){
		/*
		struct mesg{
			uint32_t counter;
			char message[];
		};
		
		::logging::log::emit()
        	<< PROGMEMSTRING("[Content:] ") << ((mesg*)&messageobject.payload)->message
		<< PROGMEMSTRING("Message Number: ") << ((mesg*)&messageobject.payload)->counter
        	<< ::logging::log::endl << ::logging::log::endl;
		*/
		
		::logging::log::emit()
        	<< PROGMEMSTRING("[Content:] ") << messageobject.payload << ::logging::log::endl
		<< PROGMEMSTRING("Message Sequence Number: ") << (int) messageobject.header.sequencenumber
        	<< ::logging::log::endl << ::logging::log::endl;

	}else{

		::logging::log::emit()
        	<< PROGMEMSTRING("Failed receiving message!") 
        	<< ::logging::log::endl << ::logging::log::endl;

	}


		 

    } while (true);
}
