#include <cstdlib>
#include <iostream>
#include <fstream>
#include <curl/curl.h>
#include <filesystem>
#include <regex>

using namespace std;

int runCommand(const string& command) {
    int status = system(command.c_str());

    return status;
}

int main(int argc, char *argv[]) {
    string command = "";
    for (int i = 1; i < argc; i++) {
        command += argv[i];
        command += " ";
    }

    string rawPhoneNumber = "";

    char *homeDir = std::getenv("HOME");
    string configLocation = string(homeDir) + "/.tawconfig";

    if (filesystem::exists(configLocation)) {
        std::ifstream configFile(configLocation);
        configFile >> rawPhoneNumber;
        configFile.close();

    } else {
        cout << "Config file does not exist (" << configLocation << ")" << endl;
        cout << "Please enter your phone number to create it now: ";
        cin >> rawPhoneNumber;
        std::ofstream configFile(configLocation);
        configFile << rawPhoneNumber;
        configFile.close();
    }

    string phoneNumber = "";

    string phoneNumberRegex = "^\\s*(\\+\\d{1,2}\\s?)?\\(?\\d{3}\\)?[\\s.-]?\\d{3}[\\s.-]?\\d{4}\\s*$";

    string phoneNumberUSRegex = "^\\s*(\\+0?1\\s?)?\\(?\\d{3}\\)?[\\s.-]?\\d{3}[\\s.-]?\\d{4}\\s*$";

    if (std::regex_match(rawPhoneNumber, std::regex(phoneNumberRegex))) {
        if (std::regex_match(rawPhoneNumber, std::regex(phoneNumberUSRegex))) {
            phoneNumber = rawPhoneNumber;

            // remove all non-numeric characters
            phoneNumber.erase(std::remove_if(phoneNumber.begin(), phoneNumber.end(), [](char c) { return !std::isdigit(c); }), phoneNumber.end());
        } else {
            cout << "Non-US phone number in config (" << configLocation << "): " << rawPhoneNumber << "\nOnly US phone numbers are supported" << endl;
            return 1;
        }
    } else {
        cout << "Invalid phone number in config (" << configLocation << "): " << rawPhoneNumber << endl;
        return 1;
    };

    int status = runCommand(command);

    string requestStr = 
        "{\"status\" : " + to_string(status) + 
        ", \"command\" : \"" + command + 
        "\", \"phone\" : \"" + phoneNumber + "\"}";
    const char* request = requestStr.c_str();

    CURL *curl;
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.158:5000");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, requestStr.length());
        curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, request);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_slist_append(NULL, "Content-Type: application/json; charset=utf-8"));
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    return status;
}