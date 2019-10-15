#pragma once

#include <iostream>
#include <random>
#include <fstream>
#include <mpi.h>
#include <cstddef>
#include <vector>
#include <string.h>
#include <stdio.h>


template<typename T>
struct Field {
    Field(int _rows, int _cols):
            pool(_rows * _cols),
            array(_rows) {
        for (int i = 0; i < _rows; ++i) {
            array[i] = &pool[i * _cols];
        }
    };

    T* operator[](int i) {
        return array[i];
    }

    std::vector<T> pool;
    std::vector<T*> array;
};


class Game {
public:

    typedef Field<char> Field_;

    Game(int _rows, int _cols):
            rows{_rows},
            cols{_cols},
            field(rows, cols) {

        std::random_device rd;
        std::mt19937 gen(rd());
        std::bernoulli_distribution bern(0.5);

        for (int i = 0; i < _rows; ++i) {
            for (int j = 0; j < _cols; ++j) {
                field[i][j] = (bern(gen) ? '1' : '0');
            }
        }

        InitiateGame();
    }

    explicit Game(std::string& source):
            rows{CountRows(source)},
            cols{CountCols(source)},
            field(rows, cols) {

        std::ifstream input;
        input.open(source);

        std::string line;
        for (int i = 0; i < rows; ++i) {
            input >> line;
            for (int j = 0; j * 2 < line.size(); ++j) {
                field[i][j] = line[j * 2];
            }
        }

        InitiateGame();
    }

    bool RequestStatus();
    void Run(const int num_steps);
    void Stop();
    void Quit();

private:
    bool pause_status = true;
    int real_thread_count = 0;
    int rows;
    int cols;
    int iter_num = 0;
    Field_ field;

    void InitiateGame();
    void PrintField();
    int CountRows(std::string& source);
    int CountCols(std::string& source);
};

void Game::Run(const int num_steps) {
    iter_num += num_steps;

    char command = 'r';
    for (int i = 0; i < real_thread_count; ++i) {
        MPI_Send(&command, 1, MPI_CHAR, i + 1, 21, MPI_COMM_WORLD);
    }

    for (int i = 0; i < real_thread_count; ++i) {
        MPI_Send(&iter_num, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD);
    }

    pause_status = false;
}

bool Game::RequestStatus() {
    int iter_num_probe = iter_num;
    int status = true;

    Stop();

    if (iter_num != iter_num_probe) {
        int difference = iter_num - iter_num_probe;
        status = false;
        Run(difference);
    }

    if(!pause_status){
        return false;
    }

    std::cout << "Done " << iter_num << " iteration(s). Current field:\n";
    PrintField();
    return true;
}

void Game::PrintField() {
    for (int i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            std::cout << field[i][j];
        }
        std::cout << '\n';
    }
}

void Game::Stop() {

    char command = 's';
    for (size_t i = 0; i < real_thread_count; ++i) {
        MPI_Send(&command, 1, MPI_CHAR, i + 1, 21, MPI_COMM_WORLD);
    }

    iter_num = 0;

    for (int i = 0; i < real_thread_count; ++i) {
        int cur_iter;
        MPI_Recv(&cur_iter, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        iter_num = std::max(iter_num, cur_iter);
    }

    for (int i = 0; i < real_thread_count; ++i) {
        MPI_Send(&iter_num, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD);
    }

    int block_size = rows / real_thread_count;
    int last_start = (real_thread_count - 1) * block_size;

    for (int i = 0; i < real_thread_count - 1; ++i) {
        MPI_Recv(field[block_size * i], block_size * cols, MPI_CHAR, i + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    MPI_Recv(field[last_start], (rows - last_start) * cols, MPI_CHAR, real_thread_count, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    pause_status = true;
}

int Game::CountRows(std::string &source) {
    int _rows = 0;

    std::ifstream input;
    input.open(source);

    std::string line;
    while (input >> line) {
        ++_rows;
    }
    return _rows;
}

int Game::CountCols(std::string &source) {
    int _cols = 0;

    std::ifstream input;
    input.open(source);

    std::string line;
    input >> line;
    _cols = (line.size() + 1) / 2;

    return _cols;
}

void Game::Quit() {
    char command = 'q';
    for (size_t i = 0; i < real_thread_count; ++i) {
        MPI_Send(&command, 1, MPI_CHAR, i + 1, 21, MPI_COMM_WORLD);
    }
}

void Game::InitiateGame() {

    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    real_thread_count = std::min(size - 1, rows);
    int block_size = rows / real_thread_count;
    int last_start = (real_thread_count - 1) * block_size;

    char command = 'c';
    for (int i = 0; i < real_thread_count; ++i) {
        MPI_Send(&command, 1, MPI_CHAR, i + 1, 21, MPI_COMM_WORLD);
    }

    command = 'f';
    for (int i = real_thread_count; i < size - 1; ++i) {
        MPI_Send(&command, 1, MPI_CHAR, i + 1, 21, MPI_COMM_WORLD);
    }

    for (int i = 0; i < real_thread_count - 1; ++i) {
        int neighbors[2];
        neighbors[0] = i;
        neighbors[1] = i + 2;
        if (neighbors[0] == 0) {
            neighbors[0] = real_thread_count;
        }
        MPI_Send(neighbors, 2, MPI_INT, i + 1, 0, MPI_COMM_WORLD);

        int size_data[2];
        size_data[0] = block_size;
        size_data[1] = cols;
        MPI_Send(size_data, 2, MPI_INT, i + 1, 0, MPI_COMM_WORLD);
        MPI_Send(field[block_size * i], block_size * cols, MPI_CHAR, i + 1, 0, MPI_COMM_WORLD);
    }

    int size_data[2];
    size_data[0] = rows - last_start;
    size_data[1] = cols;

    int neighbors[2];
    neighbors[0] = real_thread_count - 1;
    neighbors[1] = 1;
    if (neighbors[0] == 0) {
        neighbors[0] = real_thread_count;
    }

    MPI_Send(neighbors, 2, MPI_INT, real_thread_count, 0, MPI_COMM_WORLD);
    MPI_Send(size_data, 2, MPI_INT, real_thread_count, 0, MPI_COMM_WORLD);
    MPI_Send(field[last_start], (rows - last_start) * cols, MPI_CHAR, real_thread_count, 0, MPI_COMM_WORLD);
}

void QuitGame(Game*& game) {
    if (!game) {
        return;
    }

    game->Quit();

    delete game;
    game = nullptr;
}
