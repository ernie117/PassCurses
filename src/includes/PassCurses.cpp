#include "PassCurses.hpp"
#include "json.hpp"

namespace fs = std::filesystem;
using JSON = nlohmann::json;
using namespace PassCurses;
const std::string CURRENT_PATH = fs::current_path();
const int WIDTH = 30;
const int HEIGHT = 13;
const int BOX_SPACE = 11;


/*
 * Encrypts messages with XOR encryption
 */
inline std::string PassCurses::encrypt(std::string message, int CYPHER_KEY) {
    for (std::string::size_type i = 0; i < message.size(); i++) message[i] ^= CYPHER_KEY;

    return message;
}


/*
 * Decrypts XOR-encrypted messages
 */
inline std::string PassCurses::decrypt(std::string message, int CYPHER_KEY) { return encrypt(message, CYPHER_KEY); }


/*
 * Sets up ncurses, clearing screen, turn off echoing, initialize colours, hide cursor
 */
inline void
PassCurses::initialize_ncurses() {
    initscr();
    clear();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, true);
    start_color();
}


/*
 * Called when window resizes, computes new dimensions
 */
inline std::tuple<int, int>
PassCurses::resize_redraw() {
    clear();
    endwin();
    refresh();
    int resize_rows, resize_columns;
    getmaxyx(stdscr, resize_rows, resize_columns);
    int newx = (resize_columns / 2) - (WIDTH / 2);
    int newy = (resize_rows / 2) - (HEIGHT);

    return {newy, newx};
}


void PassCurses::create_rc(int CYPHER_KEY) {
    char ch;
    std::cout << "passrc not present, create? y/n \n";
    ch = getchar();
    if (ch != 'y') std::exit(EXIT_FAILURE);

    std::ofstream outstream(CURRENT_PATH + "/data/passrc");
    if (outstream.fail()) {
        std::cout << "COULD NOT CREATE FILE!\n";
        std::exit(EXIT_FAILURE);
    } else {
        std::string password;
        std::cin.ignore();

        termios old_term;
        tcgetattr(STDIN_FILENO, &old_term);
        termios new_term = old_term;
        new_term.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

        std::cout << "Enter master password to write to file: ";
        std::getline(std::cin, password);
        std::cout << "\r" << std::string(50, ' ') << "\r";
        tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
        outstream << encrypt(password, CYPHER_KEY);
        outstream.close();
        std::cout << "passrc created\n";
    }
}




/*
 * Getting master password from user, comparing to file
 */
std::string
PassCurses::read_master_password(int CYPHER_KEY) {
    std::ifstream instream(CURRENT_PATH + "/data/passrc");
    if (instream.fail()) {
        std::cerr << "CANNOT OPEN PASSRC" << std::endl;
    }

    std::string master_password;
    std::getline(instream, master_password);

    return decrypt(master_password, CYPHER_KEY);
}


/*
 * Getting the cypher key from the user
 */
int
PassCurses::set_key() {
    // Setting terminal to hide input
    termios old_term;
    tcgetattr(STDIN_FILENO, &old_term);
    termios new_term = old_term;
    new_term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    std::string temp;
    do {
        std::cout << "Enter your KEY: ";
        std::getline(std::cin, temp);
        std::cout << "\r" << std::string(20, ' ') << "\r";
    } while (temp.empty());

    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    const int CYPHER_KEY = std::stoi(temp);
    return CYPHER_KEY;
}


/*
 * Authenticating the user through password check
 */
bool
PassCurses::authenticate(int CYPHER_KEY) {
    termios old_term;
    tcgetattr(STDIN_FILENO, &old_term);
    termios new_term = old_term;
    new_term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    std::string input;

    // 'q' to exit, loop continues until password is
    // correct, or the user opts to quit
    std::string PASSWORD = read_master_password(CYPHER_KEY);
    bool authenticated = false;
    std::cout << "Enter master password: ";
    while (true) {
        std::getline(std::cin, input);
        if (input == PASSWORD) {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
            std::cout << "\r" << std::string(22, ' ') << "\r";
            std::cout.flush();
            authenticated = true;
            break;
        }
        if (input == "q") {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
            std::cout << "\r" << std::string(20, ' ') << "\r";
            std::cout.flush();
            break;
        } else std::cout << "\rTry again: "
                         << std::string(22, ' ')
                         << std::string(22, '\b');
    }

    return authenticated;
}


/*
 * Prints the passwords into position in ncurses box
 */
void
PassCurses::print_passwords(WINDOW *password_win, int highlight, JSON &j, int CYPHER_KEY, bool to_decrypt, bool is_copied) {

    int x = 2, y = 1; // Positions for printed text

    // If highlight goes higher than box height, then "scrolling" is required
    int scroll_down_amount = (highlight - BOX_SPACE);

    wclear(password_win);  // Clear the window for renewal

    int rows, columns;
    getmaxyx(stdscr, rows, columns);
    box(password_win, 0, 0);

    mvprintw((rows/2)+(HEIGHT*0.05), (columns/2)-(WIDTH/2), "%s", "press 'h' to toggle help");
    mvwprintw(password_win, 0, x, "%s", "PASSWORDS");
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED,   COLOR_BLACK);

    int i = 0;
    for (auto& [key, value] : j.items()) {
        if (i == BOX_SPACE) break; // Stop printing once box is "filled"
        if (scroll_down_amount > 0) {
            scroll_down_amount--;
            highlight--;
            continue;
        }
        // Print highlighted line
        if (highlight == i+2) {
            wattron(password_win, A_STANDOUT);
            if (to_decrypt) {
                wattron(password_win, COLOR_PAIR(2));
                mvwprintw(password_win, y, x, "%s: %s",
                          decrypt(key, CYPHER_KEY).c_str(),
                          decrypt(value.get<std::string>(), CYPHER_KEY).c_str());

                wattroff(password_win, COLOR_PAIR(2));
            } else if (is_copied) {
                mvwprintw(password_win, y, x, "%s copied!",
                          decrypt(key, CYPHER_KEY).c_str());
            } else {
                wattron(password_win, COLOR_PAIR(3));
                mvwprintw(password_win, y, x, "%s: %s",
                          decrypt(key, CYPHER_KEY).c_str(),
                          value.get<std::string>().c_str());

                wattroff(password_win, COLOR_PAIR(3));
            }
            wattroff(password_win, A_STANDOUT);
        }
        // Print non-highlighted line
        else mvwprintw(password_win, y, x, "%s: %s",
                       key.c_str(),
                       value.get<std::string>().c_str());

        i++;
        y++;
    }
    wrefresh(password_win);
    refresh();
}


/*
 * Writes edited JSON to file
 */
void
PassCurses::write_to_file(JSON &j) {
    std::ofstream outstream(CURRENT_PATH + "/data/testing.json");
    if (!outstream.is_open()) {
        std::cerr << "CAN'T WRITE TO FILE!" << std::endl;
        return;
    }

    outstream << std::setw(4) << j << std::endl;
    outstream.close();
}


/*
 * Add a user-defined password to the JSON file
 */
void PassCurses::add_password(JSON &j, WINDOW *password_win, int CYPHER_KEY) {

    int rows, columns;
    getmaxyx(stdscr, rows, columns);
    std::ifstream instream;
    instream.open(CURRENT_PATH + "/data/testing.json");
    if (!instream.is_open()) {
        std::cerr << "FILE NOT FOUND!" << std::endl;
        return;
    }

    // If there's already a file with some data in, read it in
    if (instream.peek() != std::ifstream::traits_type::eof()) instream >> j;

    instream.close();

    curs_set(1);
    char key[30];
    mvprintw((rows/2)-(HEIGHT+1), (columns/2)-(WIDTH/2), "%s", "Enter key for new password: ");
    wrefresh(password_win);
    refresh();
    scanw("%s", &key);
    std::string empty_test;
    if (empty_test.empty()) {
        mvprintw((rows/2)-(HEIGHT+1), (columns/2)-(WIDTH/2), "%s", "                              ");
        curs_set(0);
        return;
    }

    termios old_term;
    tcgetattr(STDIN_FILENO, &old_term);
    termios new_term = old_term;
    new_term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    char password[30];
    mvprintw((rows/2)-(HEIGHT+1), (columns/2)-(WIDTH/2), "%s", "                              ");
    mvprintw((rows/2)-(HEIGHT+1), (columns/2)-(WIDTH/2), "%s", "Enter your password: ");
    wrefresh(password_win);
    refresh();
    scanw("%s", &password);
    mvprintw((rows/2)-(HEIGHT+1), (columns/2)-(WIDTH/2), "%s", "                     ");
    refresh();
    wrefresh(password_win);

    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);

    std::string final_key = encrypt(std::string(key), CYPHER_KEY);
    std::string final_password = encrypt(std::string(password), CYPHER_KEY);
    j[final_key]= final_password; // setting the new/overridden value

    write_to_file(j);
    curs_set(0);
}


/*
 * Create a JSON password file if none exists
 */
void PassCurses::create_password_file(int CYPHER_KEY) {
    JSON j;
    std::ofstream outstream(CURRENT_PATH + "/data/testing.json");

    std::string key, value;
    std::cout << "Enter test key: ";
    std::cin.ignore();
    std::getline(std::cin, key);
    std::cout << "Enter test password: ";
    std::getline(std::cin, value);

    key   = encrypt(key, CYPHER_KEY);
    value = encrypt(value, CYPHER_KEY);

    j[key] = value;

    outstream << std::setw(4) << j << std::endl;
    outstream.close();

}


/*
 * Generates a random password
 */
std::string
PassCurses::generate_password(WINDOW *password_win) {
    int passw_len;
    int columns, rows;
    getmaxyx(stdscr, rows, columns);

    mvprintw((rows/2)-(HEIGHT+1), (columns/2)-(WIDTH/2), "%s", "                             ");
    mvprintw((rows/2)-(HEIGHT+1), (columns/2)-(WIDTH/2), "%s", "Enter length of new password: ");
    scanw("%d", &passw_len);
    refresh();
    mvprintw((rows/2)-(HEIGHT+1), (columns/2)-(WIDTH/2), "%s", "                              ");
    wrefresh(password_win);
    refresh();

    // RNG logic for random number generation
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> uni(48,122);

    char passw[passw_len+1]; // This will hold our randomly-generated characters
    char ch;
    for (int i = 0; i <= passw_len; i++) {
        ch = uni(rng);
        if ((ch > 90 && ch < 97) || (ch > 57 && ch < 65)) {
            i--;
            continue;
        }
        passw[i] = ch;
    }

    passw[passw_len+1] = '\0';

    return std::string(passw);
}


/*
 * Generates new random password, applies it to JSON file
 */
void
PassCurses::new_random_password(JSON &j, WINDOW *password_win, int CYPHER_KEY) {
    int columns, rows;
    getmaxyx(stdscr, rows, columns);

    std::ifstream instream;
    instream.open(CURRENT_PATH + "/data/testing.json");
    if (!instream.is_open()) {
        std::cerr << "FILE NOT FOUND!" << std::endl;
        std::exit(1);
    }

    if (instream.peek() != std::ifstream::traits_type::eof()) instream >> j;

    instream.close();
    char key[30];
    mvprintw((rows/2)-(HEIGHT+1), (columns/2)-(WIDTH/2), "%s", "Enter key for your password: ");
    scanw("%s", &key);
    std::string empty_test;
    if (empty_test.empty()) {
        mvprintw((rows/2)-(HEIGHT+1), (columns/2)-(WIDTH/2), "%s", "                              ");
        curs_set(0);
        return;
    }

    mvprintw((rows/2)-(HEIGHT+1), (columns/2)-(WIDTH/2), "%s", "                              \r");
    wrefresh(password_win);
    refresh();

    std::string passw = generate_password(password_win);

    std::string final_key = encrypt(std::string(key), CYPHER_KEY);
    std::string final_passw = encrypt(std::string(passw), CYPHER_KEY);
    j[final_key] = final_passw; // setting the new/overridden value

}

JSON
PassCurses::open_password_file(int CYPHER_KEY) {
    JSON j;
    std::ifstream instream(CURRENT_PATH + "/data/testing.json");
    if (instream.fail()) {
        instream.close();
        char ch;
        std::cout << "No password JSON, create one? y/n \n";
        ch = getchar();
        if (ch == 'y') {
            create_password_file(CYPHER_KEY);
        } else {
            instream.close();
            std::exit(EXIT_FAILURE);
        }
    }

    instream >> j;
    return j;
}

void
inline PassCurses::copy_password_to_clipboard(JSON &j, int highlight, int CYPHER_KEY) {
    int indx = 0;
    for (auto& [key, value] : j.items()) {
        indx++;
        if (indx+1 < highlight) continue;
        const char* first_part  = "echo -n ";
        const char* second_part = " | xclip -selection clipboard";
        char password[20];
        strcpy(password, decrypt(value.get<std::string>(), CYPHER_KEY).c_str());
        char command[100];
        strcpy(command, first_part);
        strcat(command, password);
        strcat(command, second_part);
        system(command);
        break;
    }
}

bool
inline PassCurses::print_help_message(bool help_printed) {
    int columns, rows;
    getmaxyx(stdscr, rows, columns);
    if (!help_printed) {
        mvprintw(0, columns, "%s", "Press:");
        mvprintw((rows/2)+(HEIGHT*0.05)+1, (columns/2)-(WIDTH/2), "%s", "'j' to scroll down");
        mvprintw((rows/2)+(HEIGHT*0.05)+2, (columns/2)-(WIDTH/2), "%s", "'k' to scroll up");
        mvprintw((rows/2)+(HEIGHT*0.05)+3, (columns/2)-(WIDTH/2), "%s", "'d' to decrypt password");
        mvprintw((rows/2)+(HEIGHT*0.05)+4, (columns/2)-(WIDTH/2), "%s", "'c' to copy password to clipboard");
        mvprintw((rows/2)+(HEIGHT*0.05)+5, (columns/2)-(WIDTH/2), "%s", "'a' to add new custom password");
        mvprintw((rows/2)+(HEIGHT*0.05)+6, (columns/2)-(WIDTH/2), "%s", "'r' to generate new password");
        mvprintw((rows/2)+(HEIGHT*0.05)+7, (columns/2)-(WIDTH/2), "%s", "'q' to quit");
        help_printed = true;
    } else {
        mvprintw((rows/2)+(HEIGHT*0.05)+1, (columns/2)-(WIDTH/2), "%s", "                  ");
        mvprintw((rows/2)+(HEIGHT*0.05)+2, (columns/2)-(WIDTH/2), "%s", "                  ");
        mvprintw((rows/2)+(HEIGHT*0.05)+3, (columns/2)-(WIDTH/2), "%s", "                       ");
        mvprintw((rows/2)+(HEIGHT*0.05)+4, (columns/2)-(WIDTH/2), "%s", "                                 ");
        mvprintw((rows/2)+(HEIGHT*0.05)+5, (columns/2)-(WIDTH/2), "%s", "                              ");
        mvprintw((rows/2)+(HEIGHT*0.05)+6, (columns/2)-(WIDTH/2), "%s", "                            ");
        mvprintw((rows/2)+(HEIGHT*0.05)+7, (columns/2)-(WIDTH/2), "%s", "           ");
        help_printed = false;
    }

    return help_printed;
}
