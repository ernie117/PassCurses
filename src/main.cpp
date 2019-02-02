// TODO Implement password deletion
#include "includes/PassCurses.hpp"
#include "includes/PassCurses.cpp"
#include "includes/json.hpp"


int main()
{
    const int CYPHER_KEY = set_key();
    if (!fs::exists(CURRENT_PATH + "/data/passrc")) create_rc(CYPHER_KEY);

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
    refresh();
    box(password_win, 0, 0);
    wrefresh(password_win);

    auto choice    = 0;
    auto highlight = 1;
    int j_compare  = j.size();
    auto decrypted = false;
    auto helped    = false;
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
                add_password(j, password_win, CYPHER_KEY);
                j_compare++; // So scrolling knows to go all the way to the bottom of passwords
                print_passwords(password_win, highlight, j, CYPHER_KEY, false, false);
                break;
            case 'r':
                new_random_password(j, password_win, CYPHER_KEY);
                j_compare++; // So scrolling knows to go all the way to the bottom of passwords
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
