#include "Deck.h"
#include "Card.h"
#include "Client.h"
#include "Server.h"
#include "Playingfield.h"
#include <iostream>

/*Enum for the game state machine*/
enum State{
	opening,
	game_type_choice,
	ai_turn,
	player_turn,

	game_start_dealer,
	game_start_person,
	game_hit_question,
	game_player_loss,
	game_dealer_loss,
	game_hit_dealer,
	game_compare_scores,

	game_server_setup,
	game_client_setup,

	game_client_hit,
	game_server_hit,
	game_c_loss,
	game_s_loss,
	game_comp_scores_cs
};

/*Local Variables*/
State currentState;
int game_type;
bool done = false;
bool isServer = false;
PlayingField* field;
Server * m_server;
Client * m_client;

/*General Functions*/
void init();
void blankScreen();
void winsockInit();

int main(void){
	/*Initialize the statemachine and the gametype*/
	init();

	/*StateMachine*/
	while (!done){
		switch (currentState){
		case opening:{
			/*Opening Sequence*/

			std::cout << "Welcome to BlackJack" << std::endl;
			currentState = game_type_choice;
		}
			break;

		case game_type_choice:{
			/*The game type choice selection*/

			blankScreen();

			std::cout << "Would you like to play against the dealer or another person" << std::endl;
			std::cout << "Play against dealer - Click 1" << std::endl;
			std::cout << "Play against another person - Click 2" << std::endl;
			std::cout << "Choice: ";
			std::cin >> game_type;

			if (game_type != 1 && game_type != 2){
				std::cout << "Incorrect choice please try again" << std::endl;
				currentState = game_type_choice;
			}
			else if (game_type == 1){
				currentState = game_start_dealer;
				std::cout << "You have chosen to play against the dealer. Good Luck!" << std::endl;
			}
			else {
				currentState = game_start_person;
				std::cout << "You have chosen to play against another player. Good luck!" << std::endl;
			}

			field = new PlayingField(new Deck);
		}
			break;

		case game_start_dealer:{
			/*Game start against computer*/

			/*Draw both player and dealer cards*/
			field->drawDealerCard();
			field->drawDealerCard();
			field->drawPlayerCard();
			field->drawPlayerCard();

			blankScreen();

			std::cout << "The field is set. Both the dealer and the player have both drawn two cards." << std::endl;

			/*Print current cards*/
			field->printField();

			currentState = game_hit_question;
		}
			break;
			
		case game_hit_question:{
			/*Hit state where player chooses to get a hit (Split between wireless and not)*/
				char response = '/0';
				std::cout << "Would you like to hit?" << std::endl;
				std::cout << "Yes : y or No : n -------- ";
				std::cin >> response;

				if (response != 'y' && response != 'n'){
					std::cout << "Error in response please try again." << std::endl;
					currentState = game_hit_question;
				}
				else if (response == 'y'){
					std::cout << "You drew a card." << std::endl;
					field->drawPlayerCard();

					/*Print current cards*/
					field->printField();

					/*Check if game is over*/
					if (!field->checkPlayerValid()){
						currentState = game_player_loss;
					}
					else{
						currentState = game_hit_question;
					}
				}
				else {
					std::cout << "You chose to stay with your current hand" << std::endl;
					currentState = game_hit_dealer;
				}
		}
			break;

		case game_hit_dealer:{
			/*Hit state where dealer draws cards until he is greater than or equal to 15*/

			
				blankScreen();

				/*Draw cards until total is greater than or equal to 15*/
				while (field->checkDealerTotal() < 15){
					std::cout << "The dealer total is less than 15. It draws a card" << std::endl;
					field->drawDealerCard();
					std::cout << "The dealer drew a ";
					field->dealer_Cards.back().printCardType();
				}

				/*Print dealer cards*/
				std::cout << "Dealers Cards:" << std::endl;
				for (int i = 0; i < field->dealer_Cards.size(); i++){
					field->dealer_Cards[i].printCardType();
				}

				/*Check if the dealer lost*/
				if (!field->checkDealerValid()){
					currentState = game_dealer_loss;
				}
				else{
					currentState = game_compare_scores;
				}
		}
			break;

		case game_dealer_loss:{
			/*Finish state for the PvComp gamemode - Dealer Lost*/

			blankScreen();

			std::cout << "The dealer lost with a score of " << field->checkDealerTotal() << ". Player won with score of " << field->checkPlayerTotal() << "." << std::endl;
			std::cout << "Final Playing Field: " << std::endl;

			/*Print current field*/
			field->printField();

			char response = '/0';
			std::cout << "Would you like to play again?" << std::endl;
			std::cout << "Yes : y or No : n -------- ";
			std::cin >> response;

			if (response != 'y' || response != 'n'){
				std::cout << "Error in response exiting." << std::endl;
			}
			else if (response == 'y'){
				currentState = game_type_choice;
			}
			else{
				std::cout << "Exiting" << std::endl;
				/*Set done flag to exit statemachine*/
				done = true;
			}
		}
			break;

		case game_player_loss:{
			/*Finish state for the PvComp gamemode - Player Lost*/

			blankScreen();

			std::cout << "The player lost with a score of " << field->checkPlayerTotal() << ". Dealer won with score of " << field->checkDealerTotal() << "." << std::endl;
			std::cout << "Final Playing Field: " << std::endl;

			/*Print current field*/
			field->printField();

			char response = '/0';
			std::cout << "Would you like to play again?" << std::endl;
			std::cout << "Yes : y or No : n -------- ";
			std::cin >> response;

			if (response != 'y' && response != 'n'){
				std::cout << "Error in response exiting." << std::endl;
			}
			else if (response == 'y'){
				currentState = game_type_choice;
			}
			else{
				std::cout << "Exiting" << std::endl;
				/*Set done flag to exit out of the statemachine*/
				done = true;
			}
		}
			break;

		case game_compare_scores:{
			/*Check both scores at the end of the hit rounds*/

			if (field->checkDealerTotal() <= field->checkPlayerTotal()){
				currentState = game_dealer_loss;
			}
			else {
				currentState = game_player_loss;
			}
		}
			break;

		case game_start_person:{
			/*Game start against another person over sockets*/

			blankScreen();

			char choice = '\0';
			std::cout << "Would you like to host or connect to a game." << std::endl;
			std::cout << "Host-h Connect-c ------ ";
			std::cin >> choice;

			if (choice != 'c' && choice != 'h'){
				std::cout << "Incorrect input. Try again" << std::endl;
				currentState = game_start_person;
			}
			else if(choice == 'c'){
				currentState = game_client_setup;
			}
			else{
				currentState = game_server_setup;
			}
		}
			break;

		case game_client_setup:{
			/*Set up a client connection to an available server*/

			winsockInit();

			m_client = new Client("127.0.0.1", 1111);
			
			isServer = false;

			if (!m_client->connectToServer()){
				/*Error connecting to the server*/
				std::cout << "Failed to connect to server" << std::endl;
			}
			else{
				std::cout << "You have connected to the host" << std::endl;

				/*Get message back from the server*/
				char recievedMessage[256];
				m_client->retrieveFromServer(recievedMessage, sizeof(recievedMessage));

				std::cout << "From Host: " << recievedMessage << std::endl;

				currentState = game_client_hit;
			}


		}
			break;

		case game_server_setup:{
			/*Set up a server for a client to connect to.*/

			winsockInit();

			m_server = new Server("127.0.0.1", 1111);

			isServer = true;

			std::cout << "Waiting for a client to connect to the server" << std::endl;
			m_server->listenForClient();

			if (!m_server->acceptClient()){
				std::cout << "Failed to accept the other connection." << std::endl;
			}
			else{
				std::cout << "You have connected to the other player" << std::endl;
				
				/*Send message to the client*/
				char sendMessage[256] = "Welcome player. Good luck! - Host";
				m_server->sendToClient(sendMessage, sizeof(sendMessage));

				currentState = game_client_hit;
			}
		}
			break;

		case game_server_hit:{
			/*For the wireless side where the client plays first*/
			if (!isServer){

				blankScreen();

				/*Print current cards*/
				field->printField();

				std::cout << "Waiting for the server to hit or not." << std::endl;

				/*Wait to retrieve data from the client*/
				char data[256];
				unsigned char i = 0;
				while (!m_client->retrieveFromServer(data, sizeof(data))){
					std::cout << "Waiting";
					for (char j = 0; j < i; j++){
						std::cout << ".";
					}
					if (i < 4){
						i = i + 1;
					}
					else{
						i = 0;
					}
					std::cout << std::endl;
				}

				/*Data was recieved*/
				std::cout << "Server: " << data << std::endl;
				currentState = game_comp_scores_cs;
			}
			else{

				blankScreen();

				/*Print current cards*/
				field->printField();

				char response = '/0';
				std::cout << "Would you like to hit?" << std::endl;
				std::cout << "Yes : y or No : n -------- ";
				std::cin >> response;

				if (response != 'y' && response != 'n'){
					std::cout << "Error in response please try again." << std::endl;
					/*Need to loop back to the response*/
				}
				else if (response == 'y'){
					std::cout << "You drew a card." << std::endl;
					field->drawDealerCard();

					/*Print current cards*/
					field->printField();

					/*Check if game is over*/
					if (!field->checkDealerValid()){
						currentState = game_s_loss;
					}
					else{
						currentState = game_comp_scores_cs;
					}
				}
				else {
					std::cout << "You chose to stay with your current hand" << std::endl;
					currentState = game_comp_scores_cs;
				}

				/*Send message to client that it is done choosing cards*/
				m_server->sendToClient();
			}
		}
			break;

		case game_client_hit:{
			/*For the wireless side where the server plays next*/
			if (isServer){
				field->drawDealerCard();
				field->drawDealerCard();

				blankScreen();

				std::cout << "The field is set. Both the dealer and the player have both drawn two cards." << std::endl;

				/*Print current cards*/
				field->printField();

				std::cout << "Waiting for the client to hit or not." << std::endl;

				/*Wait to retrieve data from the client*/
				char data[256];
				unsigned char i = 0;
				while (!m_server->retrieveFromClient(data, sizeof(data))){
					std::cout << "Waiting";
					for (char j = 0; j < i; j++){
						std::cout << ".";
					}
					if (i < 4){
						i = i + 1;
					}
					else{
						i = 0;
					}
					std::cout << std::endl;
				}

				/*Data was recieved*/
				std::cout << "Client: " << data << std::endl;
				currentState = game_server_hit;
			}
			else{		
				field->drawPlayerCard();
				field->drawPlayerCard();

				blankScreen();

				std::cout << "The field is set. Both the dealer and the player have both drawn two cards." << std::endl;

				/*Print current cards*/
				field->printField();

				char response = '/0';
				std::cout << "Would you like to hit?" << std::endl;
				std::cout << "Yes : y or No : n -------- ";
				std::cin >> response;

				if (response != 'y' && response != 'n'){
					std::cout << "Error in response please try again." << std::endl;
					/*Need to loop back to the response*/
				}
				else if (response == 'y'){
					std::cout << "You drew a card." << std::endl;
					field->drawPlayerCard();

					/*Print current cards*/
					field->printField();

					/*Check if game is over*/
					if (!field->checkPlayerValid()){
						currentState = game_c_loss;
					}
					else{
						currentState = game_server_hit;
					}
				}
				else {
					std::cout << "You chose to stay with your current hand" << std::endl;
					currentState = game_server_hit;
				}

				/*Send message to server that it is done choosing cards*/
				m_client->sendToServer();
			}
		}
			break;

		case game_c_loss:{

		}
			break;

		case game_s_loss:{

		}
			break;
	}

	/*Clean up memory leaks*/
	delete field;
	delete m_client;
	delete m_server;
}

void init(){
	game_type = 0;
	currentState = opening;
}

void blankScreen(){
	/*Adds an extra amount of spaces to clear the screen*/
	for (int i = 0; i < 50; i++){
		std::cout << '\n';
	}
}

void winsockInit(){
	/*Winsock Startup*/
	WSAData wsaData;
	WORD DllVersion = MAKEWORD(2, 1);
	if (WSAStartup(DllVersion, &wsaData) != 0){
		MessageBoxA(NULL, "Winsock startup failed", "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}
}