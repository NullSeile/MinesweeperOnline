#include "Minesweeper.h"

#include <random>

int RandRange(int min, int max)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(min, max);

	return dist(gen);
}



Minesweeper::Minesweeper(const sf::Vector2i& size, uint bombs)
	: state(State::Start)
	, size(size)
	, bombs(bombs)
{
	board.resize(size.x * size.y);
	
	InitBoard();
}

Minesweeper::Minesweeper(const sf::Vector2i& size, const std::vector<int>& bombs)
	: state(State::Start)
	, size(size)
	, bombs(bombs.size())
{
	board.resize(size.x * size.y);

	for (const int& index : bombs)
	{
		board[index].bomb = true;
	}

	InitBoard(false, false);
}


void Minesweeper::ForEachNeighbour(const sf::Vector2i& pos, const std::function<void(int i, int j)>& func)
{
	for (int i = pos.x - 1; i <= pos.x + 1; i++)
	{
		for (int j = pos.y - 1; j <= pos.y + 1; j++)
		{
			if (i < 0 || i >= size.x || j < 0 || j >= size.y)
				continue;

			func(i, j);
		}
	}
}


void Minesweeper::InitBoard(bool initBombs, bool clear)
{
	if (clear)
	{
		board.clear();
		board.resize(size.x * size.y);
	}

	if (initBombs)
	{
		// Initialize the board
		for (uint i = 0; i < bombs; i++)
		{
			uint index = RandRange(0, (size.x * size.y) - 1);

			while (board[index].bomb)
			{
				index = RandRange(0, (size.x * size.y) - 1);
			}
			board[index].bomb = true;
		}
	}

	for (int x = 0; x < size.x; x++)
	{
		for (int y = 0; y < size.y; y++)
		{
			if (!board[Get(x, y)].bomb)
			{
				// Check every neighbour
				ForEachNeighbour({ x, y }, [&](int i, int j)
				{
					if (board[Get(i, j)].bomb)
						board[Get(x, y)].neighbours++;
				});
			}
		}
	}
}


void Minesweeper::Discover(const sf::Vector2i& pos)
{
	if (state == State::Win || state == State::Lost)
		return;

	if (state == State::Start)
	{
		while (board[Get(pos)].bomb || board[Get(pos)].neighbours != 0)
		{
			InitBoard();
		}
		state = State::Playing;
	}

	std::vector<sf::Vector2i> tracked;
	if (!board[Get(pos)].marked)
	{
		if (board[Get(pos)].bomb)
		{
			state = State::Lost;
			return;
		}
		else
		{
			board[Get(pos)].discovered = true;

			if (board[Get(pos)].neighbours == 0)
				tracked.push_back(pos);
		}

		while (tracked.size() != 0)
		{
			std::vector<sf::Vector2i> newTracked;
			for (const sf::Vector2i& p : tracked)
			{
				// Check every neighbour
				ForEachNeighbour(p, [&](int i, int j)
				{
					if (!board[Get(i, j)].bomb && !board[Get(i, j)].discovered && !board[Get(i, j)].marked)
					{
						board[Get(i, j)].discovered = true;

						if (!board[Get(i, j)].bomb && board[Get(i, j)].neighbours == 0)
							newTracked.push_back({ i, j });
					}
				});

			}
			tracked = newTracked;
		}
	}
}


void Minesweeper::Mark(const sf::Vector2i& pos)
{
	if (state != State::Playing || state == State::Lost || board[Get(pos)].discovered)
		return;

	board[Get(pos)].marked = board[Get(pos)].marked == 0 ? 1 : 0;

	uint trueCount = 0;
	uint count = 0;
	for (int i = 0; i < size.x * size.y; i++)
	{
		if (board[i].marked)
		{
			count++;
			if (board[i].bomb)
				trueCount++;
		}
	}
	//std::cout << "bombs = " << bombs << ", count = " << count << ", trueCount = " << trueCount << std::endl;

	if (trueCount == bombs && count == bombs)
	{
		//for (int i = 0; i < size.x * size.y; i++)
		for (Square& s : board)
			if (!s.bomb)
				s.discovered = true;

		state = State::Win;
	}
}


unsigned int Minesweeper::Get(uint x, uint y)
{
	return x + size.x * y;
}

unsigned int Minesweeper::Get(const sf::Vector2i& pos)
{
	return pos.x + size.x * pos.y;
}
