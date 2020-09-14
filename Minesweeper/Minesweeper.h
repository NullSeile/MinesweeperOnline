#pragma once

#include <iostream>
#include <vector>
#include <functional>

#include <SFML/System/Vector2.hpp>


class Minesweeper
{
public:
	using uint = unsigned int;

	class Square
	{
	public:
		uint neighbours = 0;
		bool marked = false;
		bool discovered = false;
		bool bomb = false;
	};

	enum State : int
	{
		Start,
		Playing,
		Win,
		Lost,
		Disconnect
	};

	State state;
	sf::Vector2i size;
	uint bombs;
	std::vector<Square> board;

	void ForEachNeighbour(const sf::Vector2i& pos, const std::function<void(int i, int j)>& func);

	void InitBoard(bool initBombs = true, bool clear = true);

public:
	Minesweeper(const sf::Vector2i& size, uint bombs);
	Minesweeper(const sf::Vector2i& size, const std::vector<int>& bombs);

	void Discover(const sf::Vector2i& pos);

	void Mark(const sf::Vector2i& pos);

	uint Get(uint x, uint y);

	uint Get(const sf::Vector2i& pos);
};

/*std::ostream& operator<<(std::ostream& os, const Minesweeper::State& state)
{
	switch (state)
	{
	case Minesweeper::Start:
		os << "Minesweeper::Start";
		break;

	case Minesweeper::Playing:
		os << "Minesweeper::Playing";
		break;

	case Minesweeper::Win:
		os << "Minesweeper::Win";
		break;

	case Minesweeper::Lost:
		os << "Minesweeper::Lost";
		break;

	case Minesweeper::Disconnect:
		os << "Minesweeper::Disconnect";
		break;
	}
	return os;
}*/