

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
				ack_request=0

			};


		};



		template<class MAC_Config,class Radiocontroller,MAC_EVALUATION_ACTIVATION_STATE Mac_Evaluation_activation_state>
		struct MAC_CSMA_CA : public MAC_Base<Radiocontroller,Mac_Evaluation_activation_state>{

		

			protected:

				enum MAC_Special_Adresses{MAC_BROADCAST_ADRESS=255};


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

				//enum mac_attributes{TA,C,S};

				/*
				//all variables needed for measurement and evaluation purposes are encapsulated here
				struct Evaluation{


					Evaluation(){
					
						received_bytes_in_last_second=0;

					}

					uint16_t received_bytes_in_last_second;





				} evaluation;

				*/



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
					if(send_receive_buffer.header.controlfield.frametype!=Data) {
						send_receive_buffer.print(); //just for debug purposes
						has_message_ready_for_delivery=false;  //the application is only interested in application data, special packages have to be filtered out
						if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "has_message_ready_for_delivery=false" << ::logging::log::endl;
						return;
					}

					//evaluation.received_bytes_in_last_second+=send_receive_buffer.size;

					//intern evaluation feature for measurement of bandwith
					this->add_number_of_received_bytes(send_receive_buffer.size);


					//mac_msg->print();

					has_message_ready_for_delivery=true;

					if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "leaving receive message interupt, calling delegate" << ::logging::log::endl;

					} //critial section end


					//send_receive_buffer.print(); 
					//if we reach this instruction, everything went well and we can call a user defined interrupt service routine
					if(!onMessageReceiveDelegate.isEmpty()) onMessageReceiveDelegate();

					

				}

				void callback_periodic_timer_activation_event(){

					avr_halib::locking::GlobalIntLock lock;

					//if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "entered periodic timer interupt" << ::logging::log::endl;

					

					this->clocktick_counter++;

					
					if(this->clocktick_counter>=1000) {

						this->led.toggle();
						this->clocktick_counter=0;
						
						//::logging::log::emit() << "received bytes in last second: "  << evaluation.received_bytes_in_last_second << ::logging::log::endl;

						//::logging::log::emit() << "has_message_ready_for_delivery=" << (int) has_message_ready_for_delivery << ::logging::log::endl;	

						//evaluation.received_bytes_in_last_second=0;

						this->print_and_reset_number_of_received_bytes();

					}

					//if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "leaving periodic timer interupt" << ::logging::log::endl;

				}


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


					return 0;
				}

				int reset(){

					//here we need to call the radio controller directly, because a MAC_Base::init() wouldn't consider the extensions from this class
					Radiocontroller::reset();
					init();

					return 0;
				}


				void get_MAC_Attribut(typename MAC_Base<Radiocontroller,Mac_Evaluation_activation_state>::mac_attributes attributes,  AttributType value){

					



				}

				void set_MAC_Attribut(typename MAC_Base<Radiocontroller,Mac_Evaluation_activation_state>::mac_attributes attributes,  AttributType value){

				}

				int send_async(MAC_Message mac_message){

					return this->send_async(mac_message,MAC_BROADCAST_ADRESS);


				}

				int send_async(MAC_Message& mac_message,DeviceAddress destination_adress){

				   avr_halib::locking::GlobalIntLock lock;

				   if(!has_message_to_send){

					has_message_to_send=true;

					//init message header
					mac_message.header.sequencenumber=this->get_global_sequence_number();
					mac_message.header.source_adress=MAC_Config::mac_adress_of_node; 
					mac_message.header.source_pan=MAC_Config::pan_id;
					mac_message.header.dest_adress=destination_adress;
					mac_message.header.dest_pan=0;
					mac_message.header.controlfield.frametype=Data;
					mac_message.header.controlfield.ackrequest=MAC_Config::ack_request;

	
					//copy message into send message buffer
					send_buffer=mac_message;

					//sends the message that we copied in the send_buffer
					send_async_intern();
					return 0;

				   }else{
					return -1; //the Last Message we wanted to transmit wasn't send yet
				   }

					
				}


				void send_async_intern(){
					
					uint8_t ccaValue;
					armarow::PHY::State status;

					{ //critial section start

					//it can be called per interrupt, so we secure it
					avr_halib::locking::GlobalIntLock lock;

					if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "called async send interrupt handler" << ::logging::log::endl;

					//uncomment this  
					one_shot_timer.stop();

					status=Radiocontroller::doCCA(ccaValue);

					//if(status==armarow::PHY::IDLE){

					if(status==armarow::PHY::SUCCESS && ccaValue)
					{
						//if(!ccaValue){
							
						//::logging::log::emit()
						//<< PROGMEMSTRING("Medium BUSY!!!")		
						//<< ::logging::log::endl << ::logging::log::endl;

							
					
					//we want to send (tranceiver on)
					Radiocontroller::setStateTRX(armarow::PHY::TX_ON);

					//send
					Radiocontroller::send(*send_buffer.getPhysical_Layer_Message());

					//after sending we need to change in the Transive mode again, so that we get received messages per interrupt
					Radiocontroller::setStateTRX(armarow::PHY::RX_ON);

					if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "sending..." << ::logging::log::endl;					

					has_message_to_send=false;

					


					}else{
						//for one shot timer test
						one_shot_timer.stop();

						if(MAC_LAYER_VERBOSE_OUTPUT)  ::logging::log::emit()
           				 					<< PROGMEMSTRING("Medium busy, starting one shot timer...")
            									<< ::logging::log::endl;


						//random waiting time (from 0 to 100 ms) -> should be adjusted for real usage
						
						
						//FIXME: if something doesn't work, look if this type cast cracks everything
					
						
						int randomnumber = rand();
				     		uint32_t waitingtime = ( ((uint32_t)randomnumber * this->maximal_waiting_time_in_milliseconds) / (0x8000)); //0x8000 = RAND_MAX+1 -> Optimization, so that we can do a shift instead of a division

//one shot timer neu stellen, dieser ruft diese Funktion nach einer zufälligen Zeit erneut auf, solange bis Nachricht erfolgreich versendet wurde
						one_shot_timer.start((uint16_t)waitingtime);

						//one_shot_timer.start((uint16_t) 1000); //one shot timer test

					}

					if(MAC_LAYER_VERBOSE_OUTPUT) ::logging::log::emit() << "leaving async send interrupt handler, calling delegate" << ::logging::log::endl;
					
					
				} //critial section end

					

					//call callback, that transmission was succesfull (little workaround, so that the callback funtion is not executed in the context of the critical section)
					if(status==armarow::PHY::SUCCESS && ccaValue && !onMessage_Successfull_Transmitted_Delegate.isEmpty()) onMessage_Successfull_Transmitted_Delegate();



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


