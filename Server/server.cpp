#include <iostream>
#include <SFML/Network.hpp>
#include "..\Minesweeper\Minesweeper.h"
#include <random>
#include <vector>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>

#define port 2002

using namespace std::chrono_literals;

#define WaitFor(x) std::this_thread::sleep_for(x)

// Struct to store client data
struct Client
{
	Client()
		: state(Minesweeper::Playing)
	{

	}

	std::string name;
	sf::TcpSocket socket;
	Minesweeper::State state;
	bool connected = true;
	bool finished = false;
};

std::string TimeToStr(const float& time)
{
	int seconds = (int)time % 60;
	int minuts = (int)(time / 60.f);
	int fract = (int)(time * 100.f) % 100;

	std::stringstream ss;
	ss << std::setfill('0') << std::setw(2) << minuts << ":" << std::setw(2) << seconds << "." << std::setw(2) << fract;

	return ss.str();
}

std::ostream& operator<<(std::ostream& os, const Minesweeper::State& state)
{
	switch (state)
	{
	case Minesweeper::Start:
		os << "Start";
		break;

	case Minesweeper::Playing:
		os << "Playing";
		break;

	case Minesweeper::Win:
		os << "Win";
		break;

	case Minesweeper::Lost:
		os << "Lost";
		break;

	case Minesweeper::Disconnect:
		os << "Disconnect";
		break;

	default:
		os << "Unknown";
		break;
	}
	return os;
}

int main()
{
	// Number of players that will play
	std::cout << "Enter number players: ";
	int maxPlayers;
	std::cin >> maxPlayers;

	std::cout << "Enter board x: ";
	int xSize;
	std::cin >> xSize;

	std::cout << "Enter board y: ";
	int ySize;
	std::cin >> ySize;

	std::cout << "Enter bomb density: ";
	float bombDensity;
	std::cin >> bombDensity;

	sf::TcpListener listener;
	if (listener.listen(port) != sf::Socket::Done)
	{
		std::cout << "An error occurred listening to port " << port << std::endl;
		std::cin.ignore(10000, '\n');
		return 0;
	}
	std::cout << "Connected to port " << port << std::endl << std::endl;

	// Initialize board
	const sf::Vector2i size = { xSize, ySize };
	const int nBombs = (int)((size.x * size.y) * bombDensity);

	Minesweeper ms(size, nBombs);
	ms.Discover(size / 2);

	// Get all bombs indexes
	int* bombs = new int[nBombs];
	int index = 0;
	for (int i = 0; i < ms.board.size(); i++)
	{
		if (ms.board[i].bomb)
		{
			bombs[index] = i;
			index++;
		}
	}

	// Create board data packet to send
	sf::Packet bloardData;
	bloardData.append(&maxPlayers, sizeof(maxPlayers));
	bloardData.append(&size, sizeof(size));
	bloardData.append(&nBombs, sizeof(nBombs));
	bloardData.append(bombs, sizeof(int) * nBombs);

	// Wait for all the players to join
	std::vector<Client*> clients;
	while (maxPlayers != 0 ? clients.size() < maxPlayers : true)
	{
		// Accept a new client
		Client* client = new Client;
		if (listener.accept(client->socket) != sf::Socket::Done)
		{
			std::cout << "An error occurred accepting the client" << std::endl;
			std::cin.ignore(10000, '\n');
			return 0;
		}
		std::cout << "Client connected: " << client->socket.getRemoteAddress() << ":" << client->socket.getRemotePort() << std::endl;

		// Send board data packet
		client->socket.send(bloardData);

		// Get the name
		char name[128];
		size_t size;
		client->socket.receive(name, sizeof(name), size);
		client->name = name;

		// Add client to the list of clients
		clients.push_back(client);

		std::cout << "Data sent to " << client->name << std::endl << std::endl;
	}

	// Send to all clients the list of players
	sf::Clock clock;
	for (Client* client : clients)
	{
		for (Client* c : clients)
		{
			client->socket.send(c->name.c_str(), c->name.size() + 1);

			// Wait for the message to be recieved (50ms)
			WaitFor(50ms);
		}
	}

	// Wait 5 secconds 
	std::cout << "Starting game!!" << std::endl;
	clock.restart();
	while (clock.getElapsedTime().asSeconds() < 5);

	// Send ready signal to all clients
	for (Client* client : clients)
	{
		const char msg[] = "Ready!";
		client->socket.send(msg, sizeof(msg));
		client->socket.setBlocking(false);
	}
	clock.restart();

	// Create the selector
	sf::SocketSelector selector;

	// Wait for a winner (or looser)
	int playersLeft = maxPlayers;
	while (true)
	{
		selector.clear();

		for (Client* client : clients)
			if (client->connected && !client->finished)
				selector.add(client->socket);

		selector.wait();

		float time = clock.getElapsedTime().asSeconds();

		std::cout << "Input recieved: " << std::endl;
		for (Client* client : clients)
		{
			if (client->connected && !client->finished && selector.isReady(client->socket))
			{
				sf::Packet playerUpdate;
				client->socket.receive(playerUpdate);

				for (Client* c : clients)
					if (c->connected)
						c->socket.send(playerUpdate);

				std::string inName;
				float inTime;
				int inState;
				playerUpdate >> inName >> inTime >> inState;
				client->state = (Minesweeper::State)inState;

				std::cout << client->name << ": " << client->state << std::endl;

				if (client->state != Minesweeper::Playing)
				{
					if (client->state == Minesweeper::Disconnect)
						client->connected = false;
					
					client->finished = true;
					playersLeft--;
				}
			}
		}

		std::cout << std::endl;
		
		if (playersLeft <= 0)
			std::cin.ignore(10000, '\n');
	}

	std::cin.ignore(10000, '\n');
}