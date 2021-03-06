#include "PassCurses.hpp"
#include "json.hpp"

namespace fs = std::filesystem;
using namespace PassCurses;

using JSON = nlohmann::json;

const int WIDTH     = 30;
const int HEIGHT    = 13;
const int BOX_SPACE = 11;


/*
 * Encrypts messages with XOR encryption
 */
inline std::string
PassCurses::encrypt(std::string message, const int &CYPHER_KEY) {
    for (auto &ch : message ) ch ^= CYPHER_KEY;

    return message;
}


/*
 * Decrypts XOR-encrypted messages
 */
inline std::string
PassCurses::decrypt(std::string message, const int &CYPHER_KEY) { return encrypt(std::move(message), CYPHER_KEY); }


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
    // Different colour schemes for different users
    if ((std::string(std::getenv("USER"))) == "ernie" ||
         std::string(std::getenv("USER")) == "user-admin") {
        init_color(COLOR_CYAN, 86, 143, 143); // Actually dark grey
        init_color(COLOR_WHITE, 1000, 1000, 1000); // Colour of text
    } else {
        init_color(COLOR_CYAN, 247, 247, 247);
        init_color(COLOR_WHITE, 940, 870, 686); // Colour of text
    }
    // Colour pairs for window and highlighting
    init_pair(1, COLOR_WHITE, COLOR_CYAN);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED,   COLOR_BLACK);
}


inline WINDOW*
PassCurses::initialize_ncurses_window() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    const auto START_Y = (rows / 2) - (HEIGHT);
    const auto START_X = (cols / 2) - (WIDTH / 2);
    WINDOW *password_win = newwin(HEIGHT, WIDTH, START_Y, START_X);
    wbkgd(password_win, COLOR_PAIR(1));
    wbkgd(stdscr, COLOR_PAIR(1));
    refresh();
    box(password_win, 0, 0);
    wrefresh(password_win);

    return password_win;
}


/*
 * Redraws window, computes new dimensions
 */
inline std::tuple<const int, const int>
PassCurses::resize_redraw() {
    clear();
    endwin();
    refresh();

    int resize_rows, resize_columns;
    getmaxyx(stdscr, resize_rows, resize_columns);
    const auto new_X = (resize_columns / 2) - (WIDTH / 2);
    const auto new_Y = (resize_rows / 2) - (HEIGHT);

    return {new_Y, new_X};
}


void
PassCurses::create_rc(const int &CYPHER_KEY) {
    int ch;
    std::cout << "passrc not present, create? [y]es/[n]o \n";
    ch = getchar();
    if (ch != 'y') std::exit(EXIT_FAILURE);

    std::ofstream outstream(HOME_DIRECTORY + "/.passcurses/passrc");
    if (!outstream.is_open()) {
        std::cout << "COULD NOT CREATE FILE!\n";
        std::exit(EXIT_FAILURE);
    } else {
        std::string password;
        std::cin.ignore();

        static struct termios old_term;
        tcgetattr(STDIN_FILENO, &old_term);
        struct termios new_term = old_term;
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


inline std::string
PassCurses::get_home_directory() {
    struct passwd *pwd = getpwuid(getuid());

    const char* home_directory = pwd->pw_dir;

    return std::string(home_directory);
}


inline void
PassCurses::create_data_directory(const std::string &home_directory) {
    int choice;
    std::cout << "Create new directory for data files? [y]es/[n]o \n";
    choice = getchar();
    std::cin.ignore();
    if (choice == 'y') {
        if (fs::create_directory(home_directory + "/.passcurses")) {
            std::cout << "Directory creation successful" << std::endl;
        } else {
            std::cout << "Could not create passcurses home directory!" << std::endl;
        }
    } else std::exit(EXIT_FAILURE);
}


/*
 * Getting master password from user, comparing to file
 */
std::string
PassCurses::read_master_password(const int &CYPHER_KEY) {
    std::ifstream instream(HOME_DIRECTORY + "/.passcurses/passrc");
    if (instream.fail()) {
        std::cerr << "CANNOT OPEN PASSRC" << std::endl;
    }

    std::string master_password;
    std::getline(instream, master_password);
    instream.close();

    const std::string final_master_password = decrypt(master_password, CYPHER_KEY);

    return final_master_password;
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
PassCurses::authenticate(const int &CYPHER_KEY) {
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
PassCurses::print_passwords(WINDOW *password_win, int highlight, JSON &j, const int &CYPHER_KEY, bool to_decrypt, bool is_copied) {

    auto x = 2, y = 1; // Positions for printed passwords

    // If highlight goes higher than box height, then "scrolling" is required
    auto scroll_down_amount = (highlight - BOX_SPACE);

    wclear(password_win);  // Clear the window for renewal

    int rows, columns;
    getmaxyx(stdscr, rows, columns);
    box(stdscr, 0, 0);
    refresh();
    box(password_win, 0, 0);

    const auto print_help_line   = ((rows/2) + static_cast<int>(HEIGHT*.05));
    const auto print_help_column = ((columns/2) - (WIDTH/2));
    mvprintw(print_help_line, print_help_column, "%s", "press 'h' to toggle help");
    mvwprintw(password_win, 0, x, "%s", "PASSWORDS");

    auto i = 0;
    for (auto& [key, value] : j.items()) {
        // Stop printing once box is "filled"
        if (i == BOX_SPACE) break;
        // Iterate through lines until "view" of passwords is correct
        // This creates the scrolling effect
        if (scroll_down_amount > 0) {
            scroll_down_amount--;
            highlight--;
            continue;
        }
        // Print highlighted line
        if (highlight == i+2) {
            wattron(password_win, A_STANDOUT);
            if ((to_decrypt) && (is_copied)) {
                mvwprintw(password_win, y, x, "%s copied!",
                          decrypt(key, CYPHER_KEY).c_str());
            } else if ((!to_decrypt) && (is_copied)) {
                mvwprintw(password_win, y, x, "%s copied!",
                          decrypt(key, CYPHER_KEY).c_str());
            } else if (to_decrypt) {
                wattron(password_win, COLOR_PAIR(2));
                mvwprintw(password_win, y, x, "%s: %s",
                          decrypt(key, CYPHER_KEY).c_str(),
                          decrypt(value.get<std::string>(), CYPHER_KEY).c_str());

                wattroff(password_win, COLOR_PAIR(2));
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
    std::ofstream outstream(HOME_DIRECTORY + "/.passcurses/testing.json");
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
bool
PassCurses::add_password(JSON &j, WINDOW *password_win, const int &CYPHER_KEY) {

    int rows, columns;
    getmaxyx(stdscr, rows, columns);
    const auto ROWS = (rows/2)-(HEIGHT+1);
    const auto COLS = (columns/2)-(WIDTH/2);

    std::ifstream instream;
    instream.open(HOME_DIRECTORY + "/.passcurses/testing.json");
    if (!instream.is_open()) {
        std::cerr << "FILE NOT FOUND!" << std::endl;
        return false;
    }

    // If there's already a file with some data in, read it in
    if (instream.peek() != std::ifstream::traits_type::eof()) instream >> j;

    instream.close();

    curs_set(1);
    char key[30];
    mvprintw(ROWS, COLS, "%s", "Enter key for new password: ");
    getstr(key);
    std::string empty_test(key);
    auto k_length = empty_test.length();

    if (k_length == 0) {
        move(ROWS, COLS);
        clrtoeol();
        curs_set(0);
        return false;
    }

    termios old_term;
    tcgetattr(STDIN_FILENO, &old_term);
    termios new_term = old_term;
    new_term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    char password[30];
    mvprintw(ROWS, COLS, "%s", "Enter your password:       ");
    move(ROWS, COLS+21);
    getstr(password);
    std::string empty_pass_test(password);
    auto p_length = empty_pass_test.length();

    if (p_length == 0) {
        move(ROWS, COLS);
        clrtoeol();
        curs_set(0);
        return false;
    }

    move(ROWS, COLS);
    clrtoeol();
    refresh();
    wrefresh(password_win);

    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);

    std::string final_key = encrypt(empty_test, CYPHER_KEY);
    std::string final_password = encrypt(empty_pass_test, CYPHER_KEY);
    j[final_key]= final_password; // setting the new/overridden value

    write_to_file(j);
    curs_set(0);

    return true;
}


/*
 * Create a JSON password file if none exists
 */
void
PassCurses::create_password_file(const int &CYPHER_KEY) {
    JSON j;
    std::ofstream outstream(get_home_directory() + "/.passcurses/testing.json");

    std::string key, value;
    std::cout << "Enter test key: ";
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
    char char_passw_len[10];
    int columns, rows;
    getmaxyx(stdscr, rows, columns);
    const auto ROWS = (rows/2)-(HEIGHT+1);
    const auto COLS = (columns/2)-(WIDTH/2);

    mvprintw(ROWS, COLS, "%s", "                              ");
    mvprintw(ROWS, COLS, "%s", "Enter length of new password: ");
    getstr(char_passw_len);
    std::string str_passw_len(char_passw_len);
    auto str_p_len = str_passw_len.length();

    if (str_p_len == 0) {
        move(ROWS, COLS);
        clrtoeol();
        return "";
    }

    auto passw_len = std::stoi(str_passw_len);
    refresh();
    move(ROWS, COLS);
    clrtoeol();
    wrefresh(password_win);
    refresh();

    // RNG logic
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> uni(48,122);

    char passw[passw_len+1]; // This will hold our randomly-generated characters
    char ch;
    for (auto i = 0; i <= passw_len; i++) {
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
bool
PassCurses::new_random_password(JSON &j, WINDOW *password_win, const int &CYPHER_KEY) {
    int columns, rows;
    getmaxyx(stdscr, rows, columns);
    const auto ROWS = (rows/2)-(HEIGHT+1);
    const auto COLS = (columns/2)-(WIDTH/2);

    std::ifstream instream;
    instream.open(HOME_DIRECTORY + "/.passcurses/testing.json");
    if (!instream.is_open()) {
        std::cerr << "FILE NOT FOUND!" << std::endl;
        std::exit(1);
    }

    if (instream.peek() != std::ifstream::traits_type::eof()) instream >> j;

    instream.close();
    char key[30];
    mvprintw(ROWS, COLS, "%s", "Enter key for your password: ");
    getstr(key);
    std::string empty_test(key);
    auto k_length = empty_test.length();

    if (k_length == 0) {
        move(ROWS, COLS);
        clrtoeol();
        curs_set(0);
        return false;
    }

    mvprintw(ROWS, COLS, "%s", "                              ");
    wrefresh(password_win);
    refresh();

    std::string passw = generate_password(password_win);
    if (passw.empty()) return false;

    std::string final_key = encrypt(key, CYPHER_KEY);
    std::string final_passw = encrypt(passw, CYPHER_KEY);
    j[final_key] = final_passw; // setting the new/overridden value

    return true;
}


JSON
PassCurses::open_password_file(const int &CYPHER_KEY) {
    JSON j;
    std::string home_directory = get_home_directory();
    std::ifstream instream(home_directory + "/.passcurses/testing.json");
    if (instream.fail()) {
        instream.close();
        int ch;
        std::cout << "No password JSON, create one? [y]es/[n]o \n";
        ch = getchar();
        if (ch == 'y') {
            create_password_file(CYPHER_KEY);
            std::ifstream new_instream(home_directory + "/.passcurses/testing.json");
            new_instream >> j;
            new_instream.close();
            return j;
        } else {
            instream.close();
            std::exit(EXIT_FAILURE);
        }
    }

    instream >> j;
    instream.close();
    return j;
}


/*
 * Copy currently highlighted password to clipboard
 */
void
inline PassCurses::copy_password_to_clipboard(JSON &j, const int &highlight, const int &CYPHER_KEY) {
    auto index = 0;
    std::string password, command,
                first_part = "echo -n ",
                second_part = " | xclip -selection clipboard";

    for (auto& [key, value] : j.items()) {
        if (++index != (highlight-1)) continue;
        password = decrypt(value.get<std::string>(), CYPHER_KEY);
        break;
    }

    command = first_part + password + second_part;
    std::system(command.c_str());
}


bool
inline PassCurses::print_help_message(bool help_printed) {
    static const std::vector<std::string> HELP_STRINGS {
            "'j' to scroll down",
            "'k' to scroll up",
            "'d' to decrypt password",
            "'c' to copy password to clipboard",
            "'a' to add new custom password",
            "'r' to generate new password",
            "'q' to quit",
            "'gg' to jump to the top",
            "'G' to jump to the bottom",
            "'M' to jump to the middle",
            "'D' to delete a password",
            "'/' to search for a key"
    };
    int cols, rows;
    getmaxyx(stdscr, rows, cols);
    auto starting_row = static_cast<int>(std::round(rows/2)+(HEIGHT*0.05));
    const auto starting_col = (cols/2)-(WIDTH/2);
    if (!help_printed) {
        for (auto & str : HELP_STRINGS) {
            mvprintw(++starting_row, starting_col, "%s", str.c_str());
        }
        help_printed = true;
    } else {
        clear();
        help_printed = false;
    }

    return help_printed;
}

bool
PassCurses::delete_password_entry(JSON &j, int highlight, const int &CYPHER_KEY) {
    int choice;
    int rows, columns;
    getmaxyx(stdscr, rows, columns);
    const auto ROWS = (rows/2)-(HEIGHT+1);
    const auto COLS = (columns/2)-(WIDTH/2);

    mvprintw(ROWS, COLS, "%s", "Delete password? [y]es/[n]o ");
    choice = mvgetch(ROWS, COLS+22);
    mvprintw(ROWS, COLS, "%s", "                            ");

    if (choice == 'n') return false;
    else {
        auto indx = 0;
        std::string deleted_key;
        for (auto& [key, value] : j.items()) {
            indx++;
            if (indx+1 < highlight) continue;
            mvprintw(ROWS, COLS, "%s", "Confirm deletion: [y]es/[n]o ");
            choice = getch();
            if (choice == 'n') {
                mvprintw(ROWS, COLS, "%s", "                                         ");
                return false;
            }
            deleted_key = key;
            j.erase(key);
            break;
        }
        mvprintw(ROWS, COLS, "%s", "                                         ");
        mvprintw(ROWS, COLS, "'%s' %s", decrypt(deleted_key, CYPHER_KEY).c_str(), "password deleted!");
        getch();
        mvprintw(ROWS, COLS, "%s", "                                         ");
        write_to_file(j);
    }

    return true;

}

int
PassCurses::search_for_password(JSON &j, int highlight, const int &CYPHER_KEY) {
    int rows, columns;
    getmaxyx(stdscr, rows, columns);
    const auto ROWS = (rows/2)-(HEIGHT+1);
    const auto COLS = (columns/2)-(WIDTH/2);

    char search_chars[30];
    mvprintw(ROWS, COLS, "%s", "Enter a key to search:       ");
    echo();
    move(ROWS, COLS+23);
    getstr(search_chars);
    noecho();
    std::string search_key(search_chars);
    mvprintw(ROWS, COLS, "%s", "                                     ");

    auto it = j.find(decrypt(search_key, CYPHER_KEY));
    if (it != j.end()) {
        highlight = std::distance(j.begin(), it) + 2;
    }

    return highlight;

}
