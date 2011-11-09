

#ifndef __MAC_CSMA_CA__
#define __MAC_CSMA_CA__


#define MAC_LAYER_VERBOSE_OUTPUT false


#include "mac.h"
#include "atmega1281_timer3_regmap.h"
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_base_of.hpp>
//#include <boost/type_traits.hpp>


//#define LOGGING_DISABLE
#undef LOGGING_DISABLE

UseInterrupt(SIG_OUTPUT_COMPARE3A);

//ArMARoW/external/avr-halib/experimental/include/avr-halib/share$
//TODO: GlobalIntlock globaler interrupt lock, zum schützen der Interruptservice routinen voreinander -> einbauen




namespace armarow{

	namespace MAC{

	typedef MAC_Message mob_t;

		//this namespace contains all ERROR Messages neccessary for STATIC ASSERT ERROR MESSAGES 
		//namespace MAC_ERROR_MESSAGES{

			class INVALID_MAC_CONFIGURATION__TYPE_DOESNT_INHERIT_FROM_CLASS__MAC_CONFIGURATION;

		//}


		

		typedef void* AttributType;
		typedef uint16_t DeviceAddress; 
		

		
		struct MAC_Configuration{

			enum {

				channel=11,
				mac_adress_of_node=28, //Node ID	
				pan_id=0,
				ack_request=1,
				minimal_backoff_exponend=0,  //means, we wait 2^0=1 ms until we send a message 
				maximal_number_of_retransmissions=3,
				promiscuous_mode=0 //if 1 turns out the message filter and accepts all messages 

			};


		};

		enum MAC_Special_Adresses{MAC_BROADCAST_ADRESS=255};

		template<class MAC_Config,class Radiocontroller,MAC_EVALUATION_ACTIVATION_STATE Mac_Evaluation_activation_state>
		struct MAC_CSMA_CA : public MAC_Base<Radiocontroller,Mac_Evaluation_activation_state>{

			typedef MAC_CSMA_CA<MAC_Config,Radiocontroller,Mac_Evaluation_activation_state> MAC_LAYER;

			protected:

				


				// CLOCK 
				MAC_Clock clock;
				//typename avr_halib::drivers::
				ExactEggTimer<Timer3> one_shot_timer;				
				//avr_halib::drivers::Timer<avr_halib::config::DefaultTimerConfig<avr_halib::regmaps::local::Timer2>> a;


				//since we can either send orreceive, but not both at the same time, we just need one buffer
				MAC_Message send_receive_buffer;
				typename Radiocontroller::mob_t physical_layer_receive_buffer;

				//we don't want to deliver the same message twice, so we need a flag for that
				volatile bool has_message_ready_for_delivery; //and we declare it as volatile, so that the compiler doesn't do anything fishy to it (optimization)
				
				//bit we need for timer interrupt routine, to decide if there is a message to send (asynchron message delivery)
				volatile bool has_message_to_send;
				
				MAC_Message& send_buffer;




				struct Acknolagement_Handler{

					enum ACK_ERROR_CODE {SUCCESS,TIMEOUT};

				//if a maximal waiting time is exceeded, then waits for ack will be set to false and an error is reported
				//received_ack_for_last_transmitted_message is set only if an ack was received, if the timeout occur beforhand, it will still be false and indicates an error -> retransmission, or if number of retransmission exceeds a limit, than report an error

					//volatile bool waits_for_ack; //if false, drop ack immediatly

					//volatile bool received_ack_for_last_transmitted_message;

					//volatile bool initialized_ack_mechanism; //we need that, so we know at the beginning of the programm, if we have an error situation or are in setup state


					volatile struct {

						bool waits_for_ack : 1; //if false, drop ack immediatly

						bool received_ack_for_last_transmitted_message : 1;

						bool initialized_ack_mechanism : 1; //we need that, so we know at the beginning of the programm, if we have an error situation or are in setup state
						bool timeout_occured : 1;
					};


				//an experiment
				//structuring feature related variables and constands in a sub class for better readability and maintainability
				//(idea: reduce code scattering)
				struct Backoff_Timing{

				uint8_t current_backoff_exponend;

				//ieee maximal backoff exponend
				static const uint8_t maximal_backoff_exponend = 7; //TODO: add IEEE number here -> its a pain finding it in the standard, so we just take this value, since 2^7= 128 ms, that is quite a long backoff time

				//static const uint8_t minimal_backoff_exponend = MAC_Config::minimal_backoff_exponend;

				//uint8_t number_of_backoffs;  //TODO: not really used until now, remove if unneccessary

				//static const uint8_t maximum_number_of_backoffs = 10; //TODO: add IEEE number here


				uint8_t maximal_number_of_retransmissions;

				uint8_t current_number_of_retransmissions;

					Backoff_Timing(){

						//init variables
						this->reset();
						maximal_number_of_retransmissions=MAC_Config::maximal_number_of_retransmissions; //3;

					}

					//idea: if(is_in_boundaries()) oneshottimer.start(get_random_backoff_time_in_ms()); else drop_message();

					//has the exponend grown above the allowed limit?
					/*
					bool is_in_boundaries(){
						if(this->current_backoff_exponend<=maximal_backoff_exponend){
							return true;
						}else{
							return false;
						}
					}
					*/

					//this method is intended for the one shot timer, to get the waiting time easier 
					uint16_t get_random_backoff_time_in_ms(){
						
						//compute ieee compatible maximal backoff waiting time, that will grow exponentionally with number of backoffs
						uint8_t current_maximal_backofftime = 2^(this->current_backoff_exponend);
						this->current_backoff_exponend++;

						//get random number
						uint32_t randomnumber = rand();

						//0x8000 = RAND_MAX+1 -> Optimization, so that we can do a shift instead of a division
				     		uint32_t random_waitingtime = ( ((uint32_t)randomnumber * current_maximal_backofftime) / (0x8000)); 

						//return random waiting time as uint16_t, so that the one shot timer can just use it
						return (uint16_t)random_waitingtime;

					}

					//for easily resetting the counters, intended to use in the send function, when you just accepted a new message for transmission
					void reset(){

						current_backoff_exponend=MAC_Config::minimal_backoff_exponend; //0;

						//number_of_backoffs=0;

						current_number_of_retransmissions=0;

						

						
					}

					


				} backoff_timing;




					uint8_t sequence_number_of_last_transmitted_message;

					uint8_t destination_id_of_last_transmitted_message;

					uint8_t destination_panid_of_last_transmitted_message;

					//uint8_t maximal_number_of_retransmissions;

					//uint8_t current_number_of_retransmissions;

					uint8_t timeout_counter_in_ms;

					uint8_t timeout_duration_in_ms;

					ACK_ERROR_CODE result_of_last_send_operation_errorcode;

					//volatile bool& has_message_to_send;

					//Send_Operation_Result& result_of_last_send_operation;

					//MAC_CSMA_CA<MAC_Config,Radiocontroller,Mac_Evaluation_activation_state>& mac_instance;

					//Acknolagement_Handler(Send_Operation_Result a_result_of_last_send_operation) : result_of_last_send_operation(a_result_of_last_send_operation){ //(MAC_CSMA_CA<MAC_Config,Radiocontroller,Mac_Evaluation_activation_state>& a_mac_instance) : mac_instance(a_mac_instance){

					Acknolagement_Handler(){//(volatile bool& a_has_message_to_send) : has_message_to_send(a_has_message_to_send){
						reset();

						timeout_duration_in_ms=100;
						//maximal_number_of_retransmissions=3;
						initialized_ack_mechanism=false;
						result_of_last_send_operation_errorcode=SUCCESS;

					}

					//resets variables to default value
					void reset(){


						received_ack_for_last_transmitted_message=false;
						waits_for_ack=false;
						//current_number_of_retransmissions=0;
						timeout_counter_in_ms=0;
						timeout_occured=false;

					}

					void print(){
						::logging::log::emit() << "received_ack_for_last_transmitted_message: " << (int)received_ack_for_last_transmitted_message << ::logging::log::endl;
						::logging::log::emit() << "waits_for_ack: " << (int)waits_for_ack << ::logging::log::endl;
						::logging::log::emit() << "current_number_of_retransmissions: " << (int)backoff_timing.current_number_of_retransmissions << ::logging::log::endl;
						::logging::log::emit() << "timeout_counter_in_ms: " << (int)timeout_counter_in_ms << ::logging::log::endl;

						::logging::log::emit() << "timeout_counter_in_ms: " << (int) timeout_occured << ::logging::log::endl;

					}

					//if an ACK message is received, this method decides, whether its the ACK frame we are waiting for or not
					//if yes, we set the corrosponding bits, if not we just ignore the ACK
					void handle_received_ACK(MAC_Message& ack,volatile bool& has_message_to_send,Delegate<>& onMessage_Successfull_Transmitted_Delegate){
					//void handle_received_ACK(MAC_Message& ack,Delegate<>& onMessage_Successfull_Transmitted_Delegate){

						if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "ack was received, validating..." << ::logging::log::endl;

						//if we are not waiting for an ACK, we just drop it
						//if(waits_for_ack==false) return; 


						if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit()
						<< "last message sequence number: " << (int) sequence_number_of_last_transmitted_message << ::logging::log::endl
						<< "ack sequence number: " << (int) ack.header.sequencenumber << ::logging::log::endl;




						if (sequence_number_of_last_transmitted_message == ack.header.sequencenumber)
						//if(send_buffer.header.sequencenumber == ack.header.sequencenumber)
						//&&  destination_id_of_last_transmitted_message == ack.header.dest_adress
						//&& destination_panid_of_last_transmitted_message == ack.header.dest_pan)
						{

							//this is the ACK for the last transmitted message
							received_ack_for_last_transmitted_message=true;

							//mechanism has to be initialized again by calling "init_waiting_mechanism_for_ACK_for_MAC_Message"
							initialized_ack_mechanism=false;
			
							waits_for_ack=false;

							has_message_to_send=false;

							result_of_last_send_operation_errorcode=SUCCESS;							

							//if(MAC_LAYER_VERBOSE_OUTPUT) 
							::logging::log::emit() << "received ACK for message " << (int) sequence_number_of_last_transmitted_message << ::logging::log::endl;
							//if(MAC_LAYER_VERBOSE_OUTPUT) 
							::logging::log::emit() << "waiting time in ms: " << (int) timeout_counter_in_ms << " current timeout duration: " << (int) timeout_duration_in_ms << ::logging::log::endl;

							if(!onMessage_Successfull_Transmitted_Delegate.isEmpty()) onMessage_Successfull_Transmitted_Delegate();

						}


					}


					//called by the periodic timer ISR every ms
					void decrement_timeout_counter_every_ms(volatile bool& has_message_to_send,MAC_LAYER& mac_layer){
		//void (MAC_LAYER::*pointer_to_async_sending_ISR)()){
					
						static int loopcounter=0;

						if(loopcounter>=100){

						//print();
						loopcounter=0;
						}else{
						loopcounter++;
						}

						if(waits_for_ack){
							if(timeout_counter_in_ms < timeout_duration_in_ms){
								timeout_counter_in_ms++;
							}else{
								backoff_timing.current_number_of_retransmissions++;
							
								waits_for_ack=false;  //timeout event, delete bit, so the busy wait in the sender function will end and 
								timeout_counter_in_ms=0;

								//this is the ACK for the last transmitted message
								received_ack_for_last_transmitted_message=false;

								//mechanism has to be initialized again by calling "init_waiting_mechanism_for_ACK_for_MAC_Message"
								initialized_ack_mechanism=false;
			
								//void (MAC_LAYER::*pt2Member)() = NULL;	

								//pt2Member=0x0;
								
								timeout_occured=true;
								

								result_of_last_send_operation_errorcode=TIMEOUT;

								
								

								if(backoff_timing.current_number_of_retransmissions<= backoff_timing.maximal_number_of_retransmissions){ 

									if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "retry transmitting..." << ::logging::log::endl;
									//FIXME: TODO: uncomment this
									//send_async_intern(); //retry sending
									mac_layer.send_async_intern();

									//pointer_to_async_sending_ISR
									

								}else{
									//number of retries exceeded boundaries
									has_message_to_send=false;
									if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "TIMEOUT..." << ::logging::log::endl;

									//FIXME: TODO: call send operation finished delegate

								}								

							}
						}

					}

					//may not be called in a critical section!!!
					ACK_ERROR_CODE init_waiting_mechanism_for_ACK_for_MAC_Message(MAC_Message& msg){

						if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "wait for ACK..." << ::logging::log::endl;

						//if(msg.header.controlfield.ackrequest==0) return SUCCESS; //message header indicates, that the sender doesn't want an ACK

						sequence_number_of_last_transmitted_message = msg.header.sequencenumber;
						destination_id_of_last_transmitted_message = msg.header.dest_adress;
						destination_panid_of_last_transmitted_message = msg.header.dest_pan;
						
						reset();

						//timeout_counter_in_ms=0;
						
						//we wait for an ack, so we set the bit
						waits_for_ack=true;

						initialized_ack_mechanism=true;

						//if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "enter busy wait..." << ::logging::log::endl;

						//while(waits_for_ack && !received_ack_for_last_transmitted_message){}

						//if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "leaving busy wait..." << ::logging::log::endl;

						//if(!received_ack_for_last_transmitted_message) return TIMEOUT;

						return SUCCESS;

					}

					//TODO: verschiebe Methode aus dieser Kalsse direkt in CSMA CA dass sollte klappen, bei ACK_ERROR_CODE muss dann ein Type davor (Acknolagement_Handler::)


				/*idea: sender: 
						//busy wait
						while(received_ack_for_last_transmitted_message && waits_for_Ack){}
						


					receiver: sends ack with its node id only, if received message has the destination id of the receiver or 255 (braodcast message)
				*/

							

				} acknolagement_handler;
				//Acknolagement_Handler acknolagement_handler(this);

				

				//automatically evaluates all relevant information from the mac message, to avoid errors, and then creates aquivalent ACK Message
					int send_ACK_for_MAC_Message(MAC_Message& msg){
						/* Acknolagement_Handler::ACK_ERROR_CODE
						MAC_Message ack_message = msg;

						ack_message.setPayloadNULL();
						ack_message.size=0;						
						*/				

						//TODO:FIXME:TEST for delayed ACK, if that gets to its destination, remove LATER!!!!
						//delay_ms(1);


						if(msg.header.controlfield.ackrequest==0) return Acknolagement_Handler::SUCCESS;

						if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "sending ACK..." << ::logging::log::endl;

						//if the sender doesnt want an ACK, he won't get one
						//if(msg.header.controlfield.ackrequest==0) return Acknolagement_Handler::SUCCESS;


	//MAC_Message(IEEE_Frametype msgtyp, DeviceAddress source_adress, DeviceAddress dest_adress, char* pointer_to_databuffer, uint8_t size_of_databuffer)
						//sende es wieder dorthin, wo es her kam
						MAC_Message ack_message(Acknowledgment,  //frametype is ACK
							MAC_Config::mac_adress_of_node, //source adress of ACK, take it from Config
							msg.header.source_adress, //dest adress, is source adress of data message
							(char*)"ACK",4);//(char*) 0,0);   //ACK doesn't contain data, all relevant information are stored in the header

						ack_message.header.sequencenumber=msg.header.sequencenumber; //the ACK is for this data message, so we need the same seuqnce numbers
						ack_message.header.dest_pan = msg.header.source_pan; //destination_panid_of_last_transmitted_message;
						ack_message.header.dest_adress = msg.header.source_adress;
						ack_message.header.source_adress = MAC_Config::mac_adress_of_node;

						ack_message.header.controlfield.frametype = Acknowledgment;

						//ack_message.header.source_pan = 0; 

						//sequence_number_of_last_transmitted_message = msg.header.sequencenumber;
						//destination_id_of_last_transmitted_message = msg.header.dest_adress;
						

						ack_message.header.controlfield.ackrequest=0;

						if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "=====> start ACK msg..." << ::logging::log::endl << ::logging::log::endl;
						if(MAC_LAYER_VERBOSE_OUTPUT) ack_message.print();
						if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "=====> end ACK msg..." << ::logging::log::endl << ::logging::log::endl;


						//TODO: clear channel accessmant machen

						//we want to send (tranceiver on)
						Radiocontroller::setStateTRX(armarow::PHY::TX_ON);

						//send
						Radiocontroller::send(*ack_message.getPhysical_Layer_Message());

						//after sending we need to change in the Transive mode again, so that we get received messages per interrupt
						Radiocontroller::setStateTRX(armarow::PHY::RX_ON);



						return Acknolagement_Handler::SUCCESS;
					}



			public:

				Delegate<> onMessageReceiveDelegate;

				Delegate<> onMessage_Successfull_Transmitted_Delegate;

				MAC_CSMA_CA() : send_buffer(send_receive_buffer){   // : channel(11), mac_adress(0){


				//compile time verification, whether MAC_Configuration is the baseclass of the parameter MAC_Config, just to be shure we get a valid configuration
				 static const bool k=boost::is_base_of<MAC_Configuration,MAC_Config>::value;

				 ARMAROW_STATIC_ASSERT_ERROR(k,INVALID_MAC_CONFIGURATION__TYPE_DOESNT_INHERIT_FROM_CLASS__MAC_CONFIGURATION,(MAC_Config));

				 //BOOST_STATIC_ASSERT(k);


					MAC_Message msg;

					send_buffer = msg; 
			// = IEEE_Frametype msgtyp, DeviceAddress source_adress, DeviceAddress dest_adress, char* pointer_to_databuffer, uint8_t size_of_databuffer

					this->channel=MAC_Config::channel; //11;
					this->mac_adress_of_node=MAC_Config::mac_adress_of_node;      //28;          //this parameter can be configured 

					//maximal_waiting_time_in_milliseconds=100;
					init();	

		
				}

				/*
				struct Send_Operation_Result{

					enum ErrorCodes{SUCCESS,TIMEOUT};

					ErrorCodes errorcode;

					Send_Operation_Result(){
						errorcode=SUCCESS;
					}

				} result_of_last_send_operation;
				*/

				//=============================================================================================================================
				//============== Start Interrupt Service Routines =============================================================================
				//=============================================================================================================================






				/*receiver Thread, if the mac protocol needs an asyncron receive routine*/
				void callback_receive_message(){

				//FIXME: test for finding race condition
				avr_halib::locking::GlobalIntLock lock;

				{ //critial section start

                                 //avr_halib::locking::GlobalIntLock lock;


				if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "entered receive message interupt" << ::logging::log::endl;

					

					Radiocontroller::receive(physical_layer_receive_buffer);

					has_message_ready_for_delivery=true;
				

					armarow::MAC::MAC_Message* mac_msg = armarow::MAC::MAC_Message::create_MAC_Message_from_Physical_Message(physical_layer_receive_buffer);

					if(mac_msg == (armarow::MAC::MAC_Message*) 0 ) {

						::logging::log::emit() << "has_message_ready_for_delivery=false" << ::logging::log::endl;

						has_message_ready_for_delivery=false; //message is somehow invalid 
						return;

					}

					send_receive_buffer = *mac_msg;

					//===========================================================
					//================= MESSAGE FILTERING START =================


					if(MAC_Config::promiscuous_mode==0){

					if(send_receive_buffer.header.dest_adress==MAC_Config::mac_adress_of_node  						  
					   || send_receive_buffer.header.dest_adress==MAC_BROADCAST_ADRESS){
					//TODO: && mac_message.header.dest_pan==MAC_Config::pan_id
						if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "accepted message..." << ::logging::log::endl;

					}else{
						//message is not for us, so we drop it
						if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "dropped message..." << ::logging::log::endl;
						has_message_ready_for_delivery=false; //we are not interested in messages, that are not for us

						//::logging::log::emit() << "dropped message..." << ::logging::log::endl;
						//send_receive_buffer.print();
						return;

					}

					//at this point, we can be shure, that the received message is for us

					} //end if promiscuous mode

					//================= MESSAGE FILTERING END ==================
					//===========================================================



					if(send_receive_buffer.header.controlfield.frametype!=Data) {

						if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "===> got meta msg..." << ::logging::log::endl << ::logging::log::endl;

						if(MAC_LAYER_VERBOSE_OUTPUT) send_receive_buffer.print(); //just for debug purposes

						if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "===> end meta msg..." << ::logging::log::endl << ::logging::log::endl;

						has_message_ready_for_delivery=false;  //the application is only interested in application data, special packages have to be filtered out
					


						//TODO: set bit that sended message was acknolaged
						if(send_receive_buffer.header.controlfield.frametype==Acknowledgment){

							//set bit to 1
							acknolagement_handler.handle_received_ACK(send_receive_buffer,has_message_to_send,onMessage_Successfull_Transmitted_Delegate); //this bit has to be set to false if neccessary, so we have to provide it via reference


						}


						return;
					}


					//if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "=> start data message:" << ::logging::log::endl;
					if(MAC_LAYER_VERBOSE_OUTPUT) send_receive_buffer.print();
					//if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "=> end data message:" << ::logging::log::endl;

					//at this point, we know that we have received a data frame, so we have to send an ack
					//TODO: send ACK
					//acknolagement_handler.

					send_ACK_for_MAC_Message(send_receive_buffer);


					//evaluation.received_bytes_in_last_second+=send_receive_buffer.size;

					//intern evaluation feature for measurement of bandwith
					this->add_number_of_received_bytes(send_receive_buffer.size);


					//mac_msg->print();

					has_message_ready_for_delivery=true;

					if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "leaving receive message interrupt, calling delegate" << ::logging::log::endl;

					} //critial section end


					//send_receive_buffer.print(); 
					//if we reach this instruction, everything went well and we can call a user defined interrupt service routine
					if(!onMessageReceiveDelegate.isEmpty()) onMessageReceiveDelegate();

					

				}

				void callback_periodic_timer_activation_event(){

					avr_halib::locking::GlobalIntLock lock;

					//if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "entered periodic timer interupt" << ::logging::log::endl;

					acknolagement_handler.decrement_timeout_counter_every_ms(has_message_to_send,*this);//&MAC_LAYER::send_async_intern);

					this->clocktick_counter++;

					
					if(this->clocktick_counter>=1000) {

						this->led.toggle();
						this->clocktick_counter=0;
						
						//::logging::log::emit() << "received bytes in last second: "  << evaluation.received_bytes_in_last_second << ::logging::log::endl;

						//::logging::log::emit() << "has_message_ready_for_delivery=" << (int) has_message_ready_for_delivery << ::logging::log::endl;	

						//evaluation.received_bytes_in_last_second=0;

						this->print_and_reset_number_of_received_bytes();

						if(MAC_LAYER_VERBOSE_OUTPUT){
						 //::logging::log::emit() << "decrement_timeout_counter..." << ::logging::log::endl;
						  //acknolagement_handler.print();
						}

					}

					//if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "leaving periodic timer interupt" << ::logging::log::endl;

				}



				void send_async_intern(){
					
					//attempt_retransmission_mark:; //if we couldn't get an ack, we have to retransmit
					//FIXME: think about a better solution than that

					uint8_t ccaValue;
					armarow::PHY::State status;


					{ //critial section start

					//it can be called per interrupt, so we secure it
					avr_halib::locking::GlobalIntLock lock;

					/*
					if(send_buffer.header.controlfield.ackrequest==1){

					
					//validation: if we have a message to send, then we want an ack, if we are no longer waiting for an ack and we didn't receive one, it means the timeout for receiving an ACK was reached and the "periodic timer interrupt" reset the acknolagement_handler.waits_for_ack bit (it is set, as long as we wait for a Bit)
					if(has_message_to_send==true && acknolagement_handler.initialized_ack_mechanism==true && acknolagement_handler.waits_for_ack==false && acknolagement_handler.received_ack_for_last_transmitted_message == false){

					if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "ERROR: didn't become an ACK!!! " << (int) acknolagement_handler.sequence_number_of_last_transmitted_message << "," << (int) send_buffer.header.sequencenumber <<  ::logging::log::endl;
						//(int) send_buffer.header.sequencenumber << ::logging::log::endl;
						 acknolagement_handler.initialized_ack_mechanism=false;

					}


					//polling, bei dem die ISR verlassen wird, es wird pro ms einmal gepollt
					if(acknolagement_handler.waits_for_ack && !acknolagement_handler.received_ack_for_last_transmitted_message){
						one_shot_timer.stop();
						if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "async send: start one shot timer for waiting of ack" << ::logging::log::endl;
						//FIXME: increased default waiting period (waiting time, until we call ourself again to check if the ack is there)
						one_shot_timer.start(20); //one_shot_timer.start(1);
						return;
					}

					//wenn wir eine Nachricht gesendet haben, warten wir auf ein ACK, wenn wir das ACK empfangen haben, war das senden erfolgreich
					if(acknolagement_handler.waits_for_ack && acknolagement_handler.received_ack_for_last_transmitted_message){

							//typename Acknolagement_Handler::ACK_ERROR_CODE errorcode = 								acknolagement_handler.init_waiting_mechanism_for_ACK_for_MAC_Message(send_buffer);
					
						//if(errorcode==Acknolagement_Handler::SUCCESS){
						has_message_to_send=false;
						acknolagement_handler.waits_for_ack=false;

						if(!onMessage_Successfull_Transmitted_Delegate.isEmpty()) onMessage_Successfull_Transmitted_Delegate();

						if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "received ACK" << ::logging::log::endl;

						return;
						//}

						

					}

					}

					*/

					if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "called async send interrupt handler" << ::logging::log::endl;

					//uncomment this  
					one_shot_timer.stop();

					status=Radiocontroller::doCCA(ccaValue);

					if(status==armarow::PHY::SUCCESS && ccaValue)
					{
						
					//we want to send (tranceiver on)
					Radiocontroller::setStateTRX(armarow::PHY::TX_ON);

					if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "transmit message..." << ::logging::log::endl;
					if(MAC_LAYER_VERBOSE_OUTPUT) send_buffer.print();

					//send
					Radiocontroller::send(*send_buffer.getPhysical_Layer_Message());

					//after sending we need to change in the Transive mode again, so that we get received messages per interrupt
					Radiocontroller::setStateTRX(armarow::PHY::RX_ON);

					if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "sending..." << ::logging::log::endl;					

					if(send_buffer.header.controlfield.ackrequest==1){

					//init variables of acknolagement_handler, so that it waits for the ACK message for the transmitted message (ONLY that one)
					acknolagement_handler.init_waiting_mechanism_for_ACK_for_MAC_Message(send_buffer);

					//one_shot_timer.start(1); //minimal waiting time, we can make the one shot timer wait, the ACK needs some time anyway

					}else{

						has_message_to_send=false;
						//FIXME: TODO: this delegate should be called outside of the critical section
						 if(!onMessage_Successfull_Transmitted_Delegate.isEmpty()) onMessage_Successfull_Transmitted_Delegate();

					}

					//has_message_to_send=false;

					
					}else{
						//for one shot timer test
						one_shot_timer.stop();

						if(MAC_LAYER_VERBOSE_OUTPUT)  ::logging::log::emit()
           				 					<< PROGMEMSTRING("Medium busy, starting one shot timer...")
            									<< ::logging::log::endl;


						//random waiting time (from 0 to 100 ms) -> should be adjusted for real usage
						
						
						//FIXME: if something doesn't work, look if this type cast cracks everything
					
						
						//int randomnumber = rand();
				     		//uint32_t waitingtime = ( ((uint32_t)randomnumber * this->maximal_waiting_time_in_milliseconds) / (0x8000)); //0x8000 = RAND_MAX+1 -> Optimization, so that we can do a shift instead of a division

//one shot timer neu stellen, dieser ruft diese Funktion nach einer zufälligen Zeit erneut auf, solange bis Nachricht erfolgreich versendet wurde
						//one_shot_timer.start((uint16_t)waitingtime);
						one_shot_timer.start(acknolagement_handler.backoff_timing.get_random_backoff_time_in_ms());

						//one_shot_timer.start((uint16_t) 1000); //one shot timer test

					}

					//if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "leaving async send interrupt handler, calling delegate" << ::logging::log::endl;
					
					
				} //critial section end

				

				} //end send_async_intern




				//=============================================================================================================================
				//============== END Interrupt Service Routines ===============================================================================
				//=============================================================================================================================




				int init(){
					//message={0,{0}};

					//mac specific callback for received messages 
					this->onReceive.template bind<MAC_CSMA_CA, &MAC_CSMA_CA::callback_receive_message>(this);

					//function for one shot timer
					this->one_shot_timer.onTimerDelegate.template bind<MAC_CSMA_CA, &MAC_CSMA_CA::send_async_intern>(this);


					//this->onReceive.template bind<mac_static_receive_callback>();



					//Basic initialization
					MAC_Base<Radiocontroller,Mac_Evaluation_activation_state>::init();

					
					//typeof *this = MAC_CSMA_CA
					clock.registerCallback<typeof *this, &MAC_CSMA_CA::callback_periodic_timer_activation_event>(*this);
					//this->onReceive.template bind<MAC_CSMA_CA, &MAC_CSMA_CA::callback_periodic_timer_activation_event>(this);
					//this->onReceive.template bind<MAC_CSMA_CA, &MAC_CSMA_CA::callback_receive_message>(this);

					

					//setDelegateMethod(this->onReceive, MAC_CSMA_CA, MAC_CSMA_CA::callback_periodic_timer_activation_event, *this);



					has_message_ready_for_delivery=false;
					//has_message_to_send=true;

					has_message_to_send=false;

					// Set a method as timer event handler
					//setDelegateMethod(b.timer.onTimerDelegate, Blinker, Blinker::onTimer1, b);

					//setDelegateMethod(clock.timer.onTimerDelegate, MAC_Base, MAC_Base::callback_periodic_timer_activation_event, *this);

					//turn LED on
					this->led.toggle();

					return 0;
				}

				int reset(){

					//here we need to call the radio controller directly, because a MAC_Base::init() wouldn't consider the extensions from this class
					Radiocontroller::reset();
					init();

					return 0;
				}


				//Acknolagement_Handler::ACK_ERROR_CODE
				int get_result_of_last_send_operation(){
					return acknolagement_handler.result_of_last_send_operation_errorcode;
				}

				void get_MAC_Attribut(typename MAC_Base<Radiocontroller,Mac_Evaluation_activation_state>::mac_attributes attributes,  AttributType value){

					



				}

				void set_MAC_Attribut(typename MAC_Base<Radiocontroller,Mac_Evaluation_activation_state>::mac_attributes attributes,  AttributType value){

				}


		
				int send_async(MAC_Message& mac_message){

				   avr_halib::locking::GlobalIntLock lock;

				   if(!has_message_to_send){

					has_message_to_send=true;

					//init message header
					mac_message.header.sequencenumber=this->get_global_sequence_number();

					//TODO; in überladenen Konstruktor packen
					//optional source adress pan und message type dürfen vom nutzen nicht geändert werden -> private machen und MAC ALyer als frind deklaieren
					mac_message.header.source_adress=MAC_Config::mac_adress_of_node; 
					mac_message.header.source_pan=MAC_Config::pan_id;
					//mac_message.header.controlfield.frametype=Data;

					acknolagement_handler.backoff_timing.reset(); //backoff timing includes retransmission related variables. Since we start a new transmission, they have to be set to default values

					//vom nutzer setzbar
					//mac_message.header.dest_adress=destination_adress;
					//mac_message.header.dest_pan=0;
					//mac_message.header.controlfield.ackrequest=MAC_Config::ack_request;

	
					//copy message into send message buffer

					send_buffer=mac_message;

					//sends the message that we copied in the send_buffer
					send_async_intern();
					return 0;

				   }else{
					return -1; //the Last Message we wanted to transmit wasn't send yet
				   }

					
				}



				int send(MAC_Message mac_message){

					return this->send(mac_message,MAC_BROADCAST_ADRESS);


				}
				
				int send(MAC_Message mac_message,DeviceAddress destination_adress){


					//init message header
					mac_message.header.sequencenumber=this->get_global_sequence_number();
					mac_message.header.source_adress=MAC_Config::mac_adress_of_node; 
					mac_message.header.source_pan=MAC_Config::pan_id;
					mac_message.header.dest_adress=destination_adress; //MAC_BROADCAST_ADRESS;
					mac_message.header.dest_pan=0;
					mac_message.header.controlfield.frametype=Data;
					mac_message.header.controlfield.ackrequest=MAC_Config::ack_request;



					//random waiting time (from 0 to 100 ms) -> should be adjusted for real usage
					int randomnumber = rand();
					uint32_t waitingtime = ( ((uint32_t)randomnumber * this->maximal_waiting_time_in_milliseconds) / (0x8000)); //0x8000 = RAND_MAX+1 -> Optimization, so that we can do a shift instead of a division

					delay_ms(waitingtime);	
			

					if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << ::logging::log::endl << ::logging::log::endl 
									<< "Sending MAC_Message... " << ::logging::log::endl;
					//mac_message.print();


					//for a Clear Channel assessment we need to change into Receive State
					Radiocontroller::setStateTRX(armarow::PHY::RX_ON);

					uint8_t ccaValue;
					armarow::PHY::State status=Radiocontroller::doCCA(ccaValue);


					if(status==armarow::PHY::SUCCESS)
					{
						if(!ccaValue){
							
							if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit()
										<< PROGMEMSTRING("Medium BUSY!!!")		
										<< ::logging::log::endl << ::logging::log::endl;

							return -1;
						}

					}else return -1;

						if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit()
										<< PROGMEMSTRING("Medium frei!!!")		
										<< ::logging::log::endl << ::logging::log::endl;




					//we want to send (tranceiver on)
					Radiocontroller::setStateTRX(armarow::PHY::TX_ON);

					Radiocontroller::send(*mac_message.getPhysical_Layer_Message());

					//after sending we need to change in the Transive mode again, so that we get received messages per interrupt
					Radiocontroller::setStateTRX(armarow::PHY::RX_ON);

					//Radiocontroller::setStateTRX(armarow::PHY::TX_OFF);
					//::logging::log::emit() << ::logging::log::endl << ::logging::log::endl 
					//<< "End of SEND Methode reached" <<::logging::log::endl;
					return 0;
				}


				/*

					receive blocks until a message is received (or it returns an alredy received, but not delivered message (delivered with respect to the application))
				*/

				int receive(MAC_Message& mac_message){

					//::logging::log::emit() << "enter receive function: onreceive delegate empty: " << (int) this->onReceive.isEmpty() << ::logging::log::endl;


					mac_message.setPayloadNULL();


					Radiocontroller::setStateTRX(armarow::PHY::RX_ON);
					//Radiocontroller::receive_blocking(message);


					//TODO: replace busy wait with something like sleep that wakes up if an interupt occures to avoid energy waste
					while(has_message_ready_for_delivery==false){

						//if(has_message_ready_for_delivery) 
						//::logging::log::emit() << "has_message_ready_for_delivery=" << (int) has_message_ready_for_delivery << ::logging::log::endl;


						//delay_ms(1);

					}

					//if we get here, an interrupt occoured in the meantime, and we can deliver a message
					//the first thing we do is setting the value false again and returning then the message

					has_message_ready_for_delivery=false;


					//Radiocontroller::receive(message);

					//armarow::MAC::MAC_Message* mac_msg = armarow::MAC::MAC_Message::create_MAC_Message_from_Physical_Message(message);
					mac_message = send_receive_buffer;
					//evaluation.received_bytes_in_last_second+=send_receive_buffer.size;
						/*::logging::log::emit()
							<< PROGMEMSTRING("leaving receive method...")
							<< ::logging::log::endl << ::logging::log::endl;
						*/
					return mac_message.size;
				}

			};


		} //end namespace mac

	} //end namespace armarow



#endif


