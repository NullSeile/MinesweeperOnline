#include <iostream>
#include "..\Minesweeper\Minesweeper.h"
#include <SFML/Network.hpp>
#include <UITools/UITools.h>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

#define port 2002

using uint = unsigned int;

using namespace std::chrono_literals;

#define WaitFor(x) std::this_thread::sleep_for(x)


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

std::string TimeToStr(const float& time)
{
	int seconds = (int)time % 60;
	int minuts = (int)(time / 60.f);
	int fract = (int)(time * 100.f) % 100;

	std::stringstream ss;
	ss << std::setfill('0') << std::setw(2) << minuts << ":" << std::setw(2) << seconds << "." << std::setw(2) << fract;

	return ss.str();
}

std::string TimeToStr(const sf::Time& time)
{
	return TimeToStr(time.asSeconds());
}

struct PlayerData
{
	PlayerData(const std::string& name)
		: name(name)
	{
	}

	std::string name;
	Minesweeper::State state = Minesweeper::Playing;
	sf::Time time = sf::Time::Zero;
	float percent = 0;
};

int GetPlayerIndex(const std::string& name, const std::vector<PlayerData>& players)
{
	for (int i = 0; i < players.size(); i++)
		if (players[i].name == name)
			return i;

	return -1;
}

bool ComparePlayers(const PlayerData& a, const PlayerData& b)
{
	if (a.state == Minesweeper::Win)
	{
		if (b.state != Minesweeper::Win)
			return true;

		else
			return a.time < b.time;
	}

	if (a.state == Minesweeper::Disconnect)
	{
		if (b.state != Minesweeper::Disconnect)
			return false;

		else
			return a.percent > b.percent;
	}

	if (a.state == Minesweeper::Lost)
	{
		if (b.state != Minesweeper::Lost)
			return false;

		else
			return a.percent > b.percent;
	}

	return a.percent > b.percent;
}

int main()
{
	// Ip address of the server
	sf::IpAddress server;
	do
	{
		std::cout << "Address or name of the server: ";
		std::cin >> server;
	} while (server == sf::IpAddress::None);

	// Player username
	std::cout << "Username: ";
	std::string name;
	std::cin >> name;

	// Connect to the server
	sf::TcpSocket socket;
	if (socket.connect(server, port) != sf::Socket::Done)
	{
		std::cout << "An error occurred connecting to the server" << std::endl;
		std::cin.ignore(10000, '\n');
		return 0;
	}
	std::cout << "Connected to server " << server << std::endl;

	// Get board data packet
	sf::Packet boardPacket;
	socket.receive(boardPacket);

	// Reinterprete board pakage to extract the data
	struct Data
	{
		int nPlayers;
		sf::Vector2i size;
		int nBombs;
		int bombs[];
	};

	Data* boardData = (Data*)boardPacket.getData();
	sf::Vector2i size = boardData->size;
	
	std::vector<int> bombs(boardData->bombs, boardData->bombs + boardData->nBombs);

	std::cout << "(" << boardData->size.x << ", " << boardData->size.y << ") - " << boardData->nPlayers << std::endl;

	// Create the board
	Minesweeper ms(size, bombs);
	ms.Discover(size / 2);

	// Send username
	socket.send(name.c_str(), name.size() + 1);

	// Get players list
	//std::map<std::string, PlayerData> players;
	std::vector<PlayerData> players;
	for (int i = 0; i < boardData->nPlayers; i++)
	{
		char name[128];
		size_t nameSize;
		socket.receive(name, sizeof(name), nameSize);

		std::cout << std::string(name) << std::endl;

		players.emplace_back(std::string(name));
	}

	std::cout << "5" << std::endl;
	WaitFor(1s);
	std::cout << "4" << std::endl;
	WaitFor(1s);
	std::cout << "3" << std::endl;
	WaitFor(1s);
	std::cout << "2" << std::endl;
	WaitFor(1s);
	std::cout << "1" << std::endl;

	// Wait for server to start the game
	char serverReady[128];
	size_t msgSize;
	socket.receive(serverReady, sizeof(serverReady), msgSize);

	std::cout << std::string(serverReady) << std::endl;

	// Initialize the window
	float wSize = 600;
	float margin = 300;

	float scale = wSize / size.y;
	sf::RenderWindow window({ (uint)(size.x * scale + margin), (uint)(size.y * scale) }, "Minesweeper", sf::Style::Close);

	// Create the grid
	std::vector<sf::Vertex> grid;
	sf::Color gridColor = { 150, 150, 150 };
	for (float x = scale; x < size.x * scale; x += scale)
	{
		grid.push_back(sf::Vertex({ x, 0 }, gridColor));
		grid.push_back(sf::Vertex({ x, (float)size.y * scale }, gridColor));
	}
	for (float y = scale; y < size.y * scale; y += scale)
	{
		grid.push_back(sf::Vertex({ 0, y }, gridColor));
		grid.push_back(sf::Vertex({ (float)size.x * scale, y }, gridColor));
	}

	// Create the quads
	std::vector<sf::Vertex> quad;
	for (float y = 0; y < size.y; y++)
	{
		for (float x = 0; x < size.x; x++)
		{
			//sf::Color color = { sf::Uint8(255 * x / size.x), 0, sf::Uint8(255 * y / size.y) };
			sf::Color color = sf::Color::Black;
			quad.push_back(sf::Vertex(sf::Vector2f(x    , y    ) * scale, color));
			quad.push_back(sf::Vertex(sf::Vector2f(x    , y + 1) * scale, color));
			quad.push_back(sf::Vertex(sf::Vector2f(x + 1, y + 1) * scale, color));
			quad.push_back(sf::Vertex(sf::Vector2f(x + 1, y    ) * scale, color));
		}
	}

	// Create the font
	sf::Font font;
	font.loadFromFile("basis33.ttf");

	// "Global variables"
	sf::Time currentTime;

	int count = 0;
	for (const Minesweeper::Square& s : ms.board)
		if (s.discovered)
			count++;
	float percent = 100.f * (float)count / (float)(size.x * size.y - boardData->nBombs);

	for (PlayerData& p : players)
	{
		p.percent = percent;
	}

	// Create user interface
	ui::Widget panel;

	ui::Text timeText("time");
	timeText.setFont(font);
	timeText.setPosition(size.x * scale + 20, 10);
	timeText.setFillColor(sf::Color::White);
	timeText.setOutlineColor(sf::Color::Black);
	timeText.setOutlineThickness(1);
	timeText.setCharacterSize(40);

	timeText.SetUpdateFunction([&](ui::UIObject* obj)
	{
		auto self = dynamic_cast<ui::Text*>(obj);

		self->setString(TimeToStr(currentTime));
	});
	panel.AddObject(&timeText);


	ui::Text playersText("players");
	playersText.setFont(font);
	playersText.setPosition(size.x * scale + 10, 60);
	playersText.setFillColor(sf::Color::White);
	playersText.setOutlineColor(sf::Color::Black);
	playersText.setOutlineThickness(1);
	playersText.setCharacterSize(20);

	playersText.SetUpdateFunction([&](ui::UIObject* obj)
	{
		auto self = dynamic_cast<ui::Text*>(obj);

		std::stringstream ss;
		for (PlayerData& data : players)
		{
			ss << "· " << data.name << ": " << data.percent << "% "; 
			if (data.state != Minesweeper::Playing)
				ss << "(" << TimeToStr(data.time) << ")";

			ss << std::endl;
		}
		self->setString(ss.str());
	});
	panel.AddObject(&playersText);


	sf::Color backgrownd = sf::Color::Black;
	sf::Clock clock;

	Minesweeper::State prevState = ms.state;
	float prevPercent = percent;
	while (window.isOpen())
	{
		sf::Event e;
		while (window.pollEvent(e))
		{
			if (e.type == sf::Event::Closed)
				window.close();

			if (e.type == sf::Event::MouseButtonPressed)
			{
				sf::Vector2i pos = (sf::Vector2i)((sf::Vector2f)sf::Mouse::getPosition(window) / scale);
				if (pos.x < 0 || pos.x >= size.x || pos.y < 0 || pos.y >= size.y)
					break;
				
				if (e.key.code == sf::Mouse::Right)
				{
					ms.Mark(pos);
				}
				if (e.key.code == sf::Mouse::Left)
				{
					ms.Discover(pos);
				}
			}
		}

		window.clear(backgrownd);

		window.draw(&quad[0], quad.size(), sf::Quads);

		for (int y = 0; y < size.y; y++)
		{
			for (int x = 0; x < size.x; x++)
			{
				Minesweeper::Square& curr = ms.board[ms.Get(x, y)];

				sf::Color color = sf::Color::White;

				if (curr.marked)
				{
					if (ms.state == Minesweeper::Win || ms.state == Minesweeper::Lost)
						color = curr.bomb ? sf::Color::Green : sf::Color::Red;

					else
						color = sf::Color::Red;
				}
				else if (curr.bomb)
				{
					if (ms.state == Minesweeper::Lost)
						color = { 255, 220, 0 };
				}
				else if (curr.discovered)
				{
					color = sf::Color::Black;

					if (curr.neighbours > 0)
					{
						sf::Text text(std::to_string(ms.board[ms.Get(x, y)].neighbours), font);
						text.setCharacterSize((uint)scale);
						text.setFillColor(sf::Color::White);

						text.setPosition(x * scale + scale / 4.f, y * scale - scale / 4.f);

						window.draw(text);
					}
				}

				quad[ms.Get(x, y) * 4 + 0].color = color;
				quad[ms.Get(x, y) * 4 + 1].color = color;
				quad[ms.Get(x, y) * 4 + 2].color = color;
				quad[ms.Get(x, y) * 4 + 3].color = color;
			}
		}

		if (ms.state == Minesweeper::Playing)
			currentTime = clock.getElapsedTime();
		

		float count = 0;
		for (const Minesweeper::Square& s : ms.board)
			if (s.discovered)
				count++;
		percent = 100.f * count / (float)(size.x * size.y - boardData->nBombs);


		if (prevState != ms.state || prevPercent != percent)
		{
			std::cout << "Sent status update" << std::endl;

			socket.setBlocking(true);

			float time = clock.getElapsedTime().asSeconds();

			sf::Packet p;
			p << name << time << (int)ms.state << percent;

			socket.send(p);

			std::cout << name << ": " << prevState << " - " << ms.state << " / " << prevPercent << " - " << percent << std::endl;

			prevState = ms.state;
			prevPercent = percent;
		}

		socket.setBlocking(false);
		sf::Packet playerUpdate;
		sf::Socket::Status status = socket.receive(playerUpdate);
		if (status == sf::Socket::Done)
		{
			std::cout << "Recieved player update" << std::endl;

			std::string inName;
			float inTime;
			int inState;
			float inPercent;

			playerUpdate >> inName >> inTime >> inState >> inPercent;

			int index = GetPlayerIndex(inName, players);
			if (index == -1)
			{
				std::cout << "Player \"" << inName << "\" doesn't exist";
				return -1;
			}

			PlayerData& p = players[index];

			p.state = (Minesweeper::State)inState;
			p.time = sf::seconds(inTime);
			p.percent = inPercent;


			if (inState != Minesweeper::Playing && inName == name)
				currentTime = sf::seconds(inTime);

			std::sort(players.begin(), players.end(), ComparePlayers);

			std::cout << std::string(inName) << ": " << TimeToStr(sf::seconds(inTime)) << " - " << p.state << std::endl;
		}

		window.draw(&grid[0], grid.size(), sf::Lines);

		panel.Update(window);
		panel.Draw(window);

		window.display();
	}

	socket.setBlocking(true);

	ms.state = Minesweeper::Disconnect;
	socket.send(&ms.state, sizeof(Minesweeper::State));

	socket.disconnect();
}