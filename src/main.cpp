// TODO Implement password deletion
#include "includes/PassCurses.hpp"
#include "includes/PassCurses.cpp"
#include "includes/json.hpp"


int main()
{
    const int CYPHER_KEY = set_key();

    if (!fs::exists(HOME_DIRECTORY + "/.passcurses")) create_data_directory(HOME_DIRECTORY);
    if (!fs::exists(HOME_DIRECTORY + "/.passcurses/passrc")) create_rc(CYPHER_KEY);
    if (!fs::exists(HOME_DIRECTORY + "/.passcurses/testing.json")) create_password_file(CYPHER_KEY);

    if (!authenticate(CYPHER_KEY)) return 0;

    JSON j = open_password_file(CYPHER_KEY);

    initialize_ncurses();
    init_color(COLOR_CYAN, 86, 143, 143); // Actually dark grey
    init_pair(1, COLOR_WHITE, COLOR_CYAN);

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

    int j_compare  = j.size();
    auto choice    = 0;  // char is too small to hold curses KEY values
    auto highlight = 1;  // which password to highlight
    auto decrypted = false;  // tracking whether a password has been decrypted
    auto deleted   = false;  // tracking whether a password has been deleted
    auto helped    = false;  // tracking whether help has been printed
    auto added     = false;  // tracking whether passwords were actually added

    print_passwords(password_win, highlight, j, CYPHER_KEY, false, false);
    for (;;) {
        choice = getch();
        switch(choice) {
            case KEY_RESIZE: {
                const auto [newY, newX] = resize_redraw();
                password_win = newwin(HEIGHT, WIDTH, newY, newX);
                wbkgdset(password_win, COLOR_PAIR(1));
                print_passwords(password_win, highlight, j, CYPHER_KEY, false, false);
                break;
            }
            case KEY_DOWN:
            case 106:
                if (highlight == j_compare+1) highlight = 2;
                else ++highlight;
                if (decrypted) {
                    print_passwords(password_win, highlight, j, CYPHER_KEY, false, false);
                    decrypted = false;
                } else {
                    print_passwords(password_win, highlight, j, CYPHER_KEY, false, false);
                }
                break;
            case KEY_UP:
            case 107:
                if (highlight == 1) highlight = j_compare+1;
                else --highlight;
                if (decrypted) {
                    print_passwords(password_win, highlight, j, CYPHER_KEY, false, false);
                    decrypted = false;
                } else {
                    print_passwords(password_win, highlight, j, CYPHER_KEY, false, false);
                }
                break;
            case 'D':
                deleted = delete_password_entry(j, highlight, CYPHER_KEY);
                if (deleted) j_compare--;
                print_passwords(password_win, highlight, j, CYPHER_KEY, false, false);
                break;
            case 'd':
                if (!decrypted) {
                    print_passwords(password_win, highlight, j, CYPHER_KEY, true, false);
                    decrypted = true;
                } else {
                    print_passwords(password_win, highlight, j, CYPHER_KEY, false, false);
                    decrypted = false;
                }
                break;
            case 'c':
                copy_password_to_clipboard(j, highlight, CYPHER_KEY);
                print_passwords(password_win, highlight, j, CYPHER_KEY, false, true);
                break;
            case 'a':
                added = add_password(j, password_win, CYPHER_KEY);
                if (added) j_compare++; // So scrolling knows to go all the way to the bottom of passwords
                print_passwords(password_win, highlight, j, CYPHER_KEY, false, false);
                break;
            case 'r':
                added = new_random_password(j, password_win, CYPHER_KEY);
                if (added) j_compare++;
                write_to_file(j);
                print_passwords(password_win, highlight, j, CYPHER_KEY, false, false);
                break;
            case 'h':
                helped = print_help_message(helped);
                break;
            default:
                print_passwords(password_win, highlight, j, CYPHER_KEY, false, false);
        }
        wrefresh(password_win);
        refresh();
        if (choice == 'q') break;
    }

    clrtoeol();
    endwin();

    return 0;
}
