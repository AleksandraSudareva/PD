#include "game.h"

typedef Field<char>  Field_;


void Computations(int _rank) {
        int num_iter = 0;
    int done_iter = 0;
    int rank = _rank;
    int prev = 0;
    int next = 0;
    bool field_required = false;
    Field_* field = NULL;

    int neighbours[2];
    MPI_Recv(&neighbours, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    prev = neighbours[0];
    next = neighbours[1];

    int size_data[2];
    MPI_Recv(&size_data, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    int rows = size_data[0];
    int cols = size_data[1];

    field = new Field_(rows, cols);
    MPI_Recv(field->operator[](0), rows * cols, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    while (true) {
        int message_available = 0;

        do {
            MPI_Iprobe(0, 21, MPI_COMM_WORLD, &message_available, MPI_STATUS_IGNORE);

            if (message_available) {
                char command;
                MPI_Recv(&command, 1, MPI_CHAR, 0, 21, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                switch(command) {
                    case 'q': {
                        return;
                    }
                    case 'r': {
                        MPI_Recv(&num_iter, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        break;
                    }
                    case 's': {
                        field_required = true;
                        MPI_Send(&done_iter, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                        MPI_Recv(&num_iter, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        break;
                    }
                    default: {
                        std::cout << "UNKNOWN COMMAND " << command << " IN MAIN LOOP\n";
                        break;
                    }
                }
            }

            if (num_iter == done_iter && field_required) {
                field_required = false;
                MPI_Send(field->operator[](0), rows * cols, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }

        } while (num_iter == done_iter);

        std::vector<char> prev_field(cols);
        std::vector<char> next_field(cols);

        if (rank % 2 == 0) {
            MPI_Send(field->operator[](0), cols, MPI_CHAR, prev, 0, MPI_COMM_WORLD);
            MPI_Recv(&next_field[0], cols, MPI_CHAR, next, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            MPI_Send(field->operator[](rows - 1), cols, MPI_CHAR, next, 0, MPI_COMM_WORLD);
            MPI_Recv(&prev_field[0], cols, MPI_CHAR, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        } else {

            if (rank == 1 && prev % 2 == 1) {
                MPI_Send(field->operator[](0), cols, MPI_CHAR, prev, 0, MPI_COMM_WORLD);
                MPI_Recv(&next_field[0], cols, MPI_CHAR, next, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else {
                MPI_Recv(&next_field[0], cols, MPI_CHAR, next, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(field->operator[](0), cols, MPI_CHAR, prev, 0, MPI_COMM_WORLD);
            }

            MPI_Recv(&prev_field[0], cols, MPI_CHAR, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(field->operator[](rows - 1), cols, MPI_CHAR, next, 0, MPI_COMM_WORLD);
        }

        auto updated_field = new Field_(rows, cols);

        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {

                int alive_count = 0;

                for (int vshift = -1; vshift < 2; ++vshift) {
                    for (int hshift = -1; hshift < 2; ++hshift) {
                        int hind = (j + hshift + cols) % cols;

                        if (i == 0 && vshift == -1) {
                            if (prev_field[hind] == '1') {
                                ++alive_count;
                            }
                        } else if (i == rows - 1 && vshift == 1) {
                            if (next_field[hind] == '1') {
                                ++alive_count;
                            }
                        } else {
                            if (field->operator[](i + vshift)[hind] == '1') {
                                ++alive_count;
                            }
                        }
                    }
                }

                if (field->operator[](i)[j] == '1') {
                    --alive_count;
                }

                if (field->operator[](i)[j] == '1') {
                    updated_field->operator[](i)[j] = (alive_count == 2 || alive_count == 3 ? '1' : '0');
                } else {
                    updated_field->operator[](i)[j] = (alive_count == 3 ? '1' : '0');
                }
            }
        }

        delete field;
        field = updated_field;
        ++done_iter;
    }
}








int main(int argc, char **argv) {

    Game* game = NULL;

    int num_rows = 0;
    int num_cols = 0;
    std::string init_status;
    bool flag = true;

    MPI_Init(&argc, &argv);

    int size, rank, count;
    MPI_Status status;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        while(1) {
            if(flag == false) {
                break;
            }

            std::string query;
            std::cin >> query;

            if(query == "START") {
                std::cin >> init_status;

                if(init_status == "RANDOM") {
                    int num_rows, num_cols;
                    std::cin >> num_rows >> num_cols;
                    game = new Game(num_rows, num_cols);
                } else {
                    game = new Game(init_status);
                }
                continue;
            } else if(query == "RUN") {
                if (!game) {
                    std::cout << "START THE GAME FIRSTLY\n";
                    continue;
                }

                int iter_count;
                std::cin >> iter_count;
                game->Run(iter_count);

                continue;
            } else if (query == "STOP") {
                if (!game) {
                    std::cout << "START THE GAME FIRSTLY\n";
                    continue;
                }
                game->Stop();
                continue;
            } else if (query == "STATUS") {
                if (!game) {
                    std::cout << "START THE GAME FIRSTLY\n";
                    continue;
                }
                if (!game->RequestStatus()) {
                    std::cout << "STOP THE GAME FIRSTLY\n";
                }
                continue;
            } else if (query == "RESET") {
                continue;
            } else if (query == "QUIT") {
                QuitGame(game);
                flag = false;
                continue;
            } else {
                std::cout << "UNKNOWN COMMAND\n";
                continue;
            }
        }
    } else {
        while(1) {
            if (flag == false) {
                break;
            }

            char command;
            MPI_Recv(&command, 1, MPI_CHAR, 0, 21, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (command != 'f') {
                Computations(rank);
            }
        }
    }

    MPI_Finalize();
    return(0);
}

