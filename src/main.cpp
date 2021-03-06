#include "includes/PassCurses.hpp"
#include "includes/PassCurses.cpp"
#include "includes/json.hpp"


int main()
{
    static const int CYPHER_KEY = set_key();

    static const std::string HOME_DIRECTORY = get_home_directory();

    if (!fs::exists(HOME_DIRECTORY + "/.passcurses")) create_data_directory(HOME_DIRECTORY);
    if (!fs::exists(HOME_DIRECTORY + "/.passcurses/passrc")) create_rc(CYPHER_KEY);
    if (!fs::exists(HOME_DIRECTORY + "/.passcurses/testing.json")) create_password_file(CYPHER_KEY);

    if (!authenticate(CYPHER_KEY)) return 0;

    JSON j = open_password_file(CYPHER_KEY);

    initialize_ncurses();
    WINDOW *password_win = initialize_ncurses_window();

    auto j_compare = j.size();
    auto choice    = 0;      // char is too small to hold curses KEY values
    auto decrypted = false;  // tracking whether a password has been decrypted
    auto is_copied = false;  // tracking whether a password has been copied
    auto helped    = false;  // tracking whether help has been printed
    auto highlight = 1;      // which password to highlight

    print_passwords(password_win, highlight, j, CYPHER_KEY, decrypted, is_copied);
    for (;;) {
        is_copied = false;
        choice = getch();
        switch(choice) {
            case KEY_RESIZE: {
                const auto [newY, newX] = resize_redraw();
                password_win = newwin(HEIGHT, WIDTH, newY, newX);
                wbkgdset(password_win, COLOR_PAIR(1));
                break;
            }
            case KEY_DOWN:
            case 106:
                if (highlight == j_compare+1) highlight = 2;
                else ++highlight;
                if (decrypted) decrypted = false;
                break;
            case KEY_UP:
            case 107:
                if (highlight == 1) highlight = j_compare+1;
                else --highlight;
                if (decrypted) decrypted = false;
                break;
            // Vim-like binding to jump to the top
            case 'g':
                choice = getch();
                if (choice == 'g') highlight = 2;
                break;
            // Jump to the bottom
            case 'G':
                highlight = j_compare+1;
                break;
            // Jump to the middle
            case 'M':
                highlight = (j_compare / 2) + 2;
                break;
            // Delete a password
            case 'D':
                if (delete_password_entry(j, highlight, CYPHER_KEY)) j_compare--;
                break;
            // Decrypt/encrypt a password
            case 'd':
                decrypted = !decrypted;
                break;
            // Copy a password
            case 'c':
                copy_password_to_clipboard(j, highlight, CYPHER_KEY);
                is_copied = true;
                break;
            // Add a password
            case 'a':
                // Adding a password necessarily increases the number of passwords
                // so the 'size' tracking variable needs to be incremented
                if (add_password(j, password_win, CYPHER_KEY)) j_compare++;
                break;
            // Generate a random password
            case 'r':
                if (new_random_password(j, password_win, CYPHER_KEY)) j_compare++;
                write_to_file(j);
                break;
            // Search for a password key
            case '/':
                highlight = search_for_password(j, highlight, CYPHER_KEY);
                break;
            // Show the help lines
            case 'h':
                helped = print_help_message(helped);
                break;
            default:
                print_passwords(password_win, highlight, j, CYPHER_KEY, decrypted, is_copied);
        }
        print_passwords(password_win, highlight, j, CYPHER_KEY, decrypted, is_copied);
        wrefresh(password_win);
        refresh();
        if (choice == 'q') break;
    }

    clear();
    endwin();

    return 0;
}
