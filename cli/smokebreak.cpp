#include <cstdlib>
#include <iostream>
#include <fstream>
#include <curl/curl.h>
#include <filesystem>
#include <regex>

using namespace std;

#define ENDPOINT "http://192.168.0.158:5000"

int runCommand(const string& command) {
    int status = system(command.c_str());

    return status;
}

string getRawPhoneNumber(string configLocation) {
    string rawPhoneNumber = "";

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

    return rawPhoneNumber;
}

size_t discardData(void *buffer, size_t size, size_t nmemb, void *userp)
{
   return size * nmemb;
}

string generateRequest(int exitStatus, string command, string phoneNumber) {
    string requestStr = 
        "{\"status\" : " + to_string(exitStatus) + 
        ", \"command\" : \"" + command + 
        "\", \"phone\" : \"" + phoneNumber + "\"}";

    return requestStr;
}

CURLcode postRequest(string requestString) {
    const char *request = requestString.c_str();
    int requestLength = requestString.length();

    CURL *curl = curl_easy_init();
    CURLcode res;

    curl_easy_setopt(curl, CURLOPT_URL, ENDPOINT);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, requestLength);
    curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, request);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_slist_append(NULL, "Content-Type: application/json; charset=utf-8"));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discardData);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return res;
}

int main(int argc, char *argv[]) {
    string command = "";
    for (int i = 1; i < argc; i++) {
        command += argv[i];
        command += " ";
    }

    char *homeDir = std::getenv("HOME");
    string configLocation = string(homeDir) + "/.sbconfig";

    string rawPhoneNumber = getRawPhoneNumber(configLocation);

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

    int exitStatus = runCommand(command);
    
    string request = generateRequest(exitStatus, command, phoneNumber);

    CURLcode res = postRequest(request);

    if (res != CURLE_OK) {
        cout << "Smokebreak experienced an unknown server error: " << res << endl;
    }

    return exitStatus;
}