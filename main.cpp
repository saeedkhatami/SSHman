#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <cstdio>
#include <chrono>
#include <thread>

const std::string DATABASE_FILE = "./db.txt";

struct UserData {
    std::string expirationDate;
    bool active;
};

void clearScreen() {
    system("clear");
}

std::map<std::string, UserData> loadDatabase() {
    std::ifstream file(DATABASE_FILE);
    std::map<std::string, UserData> database;

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string username;
            std::string expirationDate;
            bool active;

            if (iss >> username >> expirationDate >> active) {
                database[username] = {expirationDate, active};
            }
        }

        file.close();
    }

    std::string listUsersCommand = "awk -F: '/bash/{print $1}' /etc/passwd";
    FILE* pipe = popen(listUsersCommand.c_str(), "r");

    if (pipe) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            buffer[strcspn(buffer, "\n")] = '\0';

            if (database.find(buffer) == database.end()) {
                database[buffer] = {"", true};
            }
        }

        pclose(pipe);
    }

    return database;
}

void saveDatabase(const std::map<std::string, UserData>& data) {
    std::ofstream file(DATABASE_FILE);

    if (file.is_open()) {
        for (const auto& entry : data) {
            const std::string& username = entry.first;
            const UserData& userData = entry.second;
            file << username << " " << userData.expirationDate << " " << userData.active << "\n";
        }

        file.close();
    } else {
        std::cerr << "Unable to open database file for writing." << std::endl;
    }
}

void createUser(const std::string& username, int daysActive) {
    std::time_t now = std::time(nullptr);
    std::tm* expirationDate = std::localtime(&now);
    expirationDate->tm_mday += daysActive;

    std::time_t expirationTime = std::mktime(expirationDate);
    expirationDate = std::localtime(&expirationTime);

    char buffer[11];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", expirationDate);

    system(("sudo useradd " + username).c_str());
    system(("sudo passwd " + username).c_str());

    std::map<std::string, UserData> database = loadDatabase();
    database[username] = {buffer, true};
    saveDatabase(database);
}

void deleteUser(const std::string& username) {
    std::map<std::string, UserData> database = loadDatabase();
    auto it = database.find(username);

    if (it != database.end()) {// force kill and delete user
        system(("sudo pkill -u " + username).c_str());

        std::this_thread::sleep_for(std::chrono::seconds(2));

        system(("sudo userdel " + username).c_str());

        database.erase(it);
        std::cout << "User " << username << " deleted." << std::endl;

        saveDatabase(database);
    } else {
        std::cerr << "User " << username << " not found in the database. Please enter a valid username." << std::endl;
    }
}

void listUsers() {
    std::map<std::string, UserData> database = loadDatabase();

    std::cout << "Users:" << std::endl;
    for (const auto& entry : database) {
        const std::string& username = entry.first;
        const UserData& data = entry.second;

        std::string status = data.active ? "Active" : "Inactive";
        std::cout << username << " - Status: " << status << ", Expires on: " << data.expirationDate << std::endl;
    }
}

void editUser(const std::string& username) {
    std::map<std::string, UserData> database = loadDatabase();
    auto it = database.find(username);

    if (it != database.end()) {
        std::cout << "Current expiration date for user " << username << ": " << it->second.expirationDate << std::endl;

        std::cout << "Enter new expiration date (YYYY-MM-DD or number of days) or 'none' to keep it unchanged: ";
        std::string newExpirationInput;
        std::cin >> newExpirationInput;

        if (newExpirationInput != "none") {
            std::string usermodCommand = "sudo usermod --expiredate " + newExpirationInput + " " + username;
            if (system(usermodCommand.c_str()) == 0) {
                std::cout << "Expiration date for user " << username << " updated to: " << newExpirationInput << std::endl;
                it->second.expirationDate = newExpirationInput;
            } else {
                std::cerr << "Failed to update expiration date for user " << username << ". Please check your input." << std::endl;
            }
        }

        std::cout << "Do you want to deactivate the user? (y/n): ";
        char choice;
        std::cin >> choice;

        if (choice == 'y' || choice == 'Y') {
            it->second.active = false;
            std::cout << "User " << username << " deactivated." << std::endl;
        }

        saveDatabase(database);
    } else {
        std::cerr << "User " << username << " not found in the database. Please enter a valid username." << std::endl;
    }
}

int main() {
    clearScreen();
    while (true) {
                std::cout << "\n1. Create User\n2. List Users\n3. Delete User\n4. Edit User\n0. Exit" << std::endl;
        std::cout << "Enter your choice (1/2/3/4/0): ";

        int choice;
        std::cin >> choice;

        switch (choice) {
            case 1: {
		        clearScreen();
                std::string username;
                int daysActive;
                std::cout << "Enter username: ";
                std::cin >> username;
                std::cout << "Enter days of activation: ";
                std::cin >> daysActive;
                createUser(username, daysActive);
                break;
            }
            case 2:
                clearScreen();
                listUsers();
                break;
            case 3: {
		        clearScreen();
                std::string username;
		        listUsers();
                std::cout << "Enter username to delete: ";
                std::cin >> username;
                deleteUser(username);
                break;
            }
            case 4: {
		        clearScreen();
                std::string username;
		        listUsers();
                std::cout << "Enter username to edit: ";
                std::cin >> username;
                editUser(username);
                break;
	        }
	        case 0:
		        return 0;
            default:
                std::cerr << "Invalid choice. Please enter 1, 2, 3, 4 or 0." << std::endl;
        }
    }

    return 0;
}

