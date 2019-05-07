#pragma once // Only include this header once, in lieu of header guards
#include <filesystem>
#include <iostream>
#include <ncurses.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <fstream>
#include <unistd.h>
#include <iomanip>
#include <random>
#include <thread>
#include <tuple>
#include "json.hpp"


extern const int WIDTH;
extern const int HEIGHT;
extern const int BOX_SPACE;

extern const std::vector<std::string> HELP_STRINGS;


namespace PassCurses {

    /*
     * Encrypts messages with XOR encryption
     */
    inline std::string
    encrypt(std::string message, const int &CYPHER_KEY);


    /*
     * Decrypts XOR-encrypted messages
     */
    inline std::string
    decrypt(std::string message, const int &CYPHER_KEY);


    /*
     * Respond to window resize by redrawing
     */
    inline std::tuple<const int, const int>
    resize_redraw();


    /*
     * Creates a passrc if one doesn't already exist
     */
    void
    create_rc(const int &CYPHER_KEY);

    inline std::string
    get_home_directory();


    /*
     * Creates directory for data files
     */
    inline void
    create_data_directory(const std::string &home_directory);


    inline void
    initialize_ncurses();


    inline WINDOW*
    initialize_ncurses_window();


    /*
     * Getting master password from user, comparing to file
     */
    std::string
    read_master_password(const int &CYPHER_KEY);


    /*
     * Getting the cypher key from the user
     */
    int
    set_key();


    /*
     * Authenticating the user through password check
     */
    bool
    authenticate(const int &CYPHER_KEY);


    /*
     * Prints the passwords into position in ncurses box
     */
    void
    print_passwords(WINDOW *password_win, int highlight, nlohmann::json &j, const int &CYPHER_KEY, bool to_decrypt, bool is_copied);


    /*
     * Writes edited nlohmann::json to file
     */
    void
    write_to_file(nlohmann::json &j);


    /*
     * Add a user-defined password to the nlohmann::json file
     */
    bool
    add_password(nlohmann::json &j, WINDOW *password_win, const int &CYPHER_KEY);


    /*
     * Create a nlohmann::json password file if none exists
     */
    void
    create_password_file(const int &CYPHER_KEY);


    /*
     * Generates a random password
     */
    std::string
    generate_password(WINDOW *password_win);


    /*
     * Generates new random password, applies it to password JSON, calls generate_password()
     */
    bool
    new_random_password(nlohmann::json &j, WINDOW *password_win, const int &CYPHER_KEY);

    /*
     * Opens password file, if it exists
     */
    nlohmann::json
    open_password_file(const int &CYPHER_KEY);

    void
    inline copy_password_to_clipboard(nlohmann::json &j, const int &highlight, const int &CYPHER_KEY);

    bool
    inline print_help_message(bool help_printed);


    /*
     * Delete a password entry in the JSON file
     */
    bool
    delete_password_entry(nlohmann::json &j, int highlight, const int &CYPHER_KEY);

    /*
     * Search for a password entry
     */
    int
    search_for_password(nlohmann::json &j, int highlight, const int &CYPHER_KEY);
}
