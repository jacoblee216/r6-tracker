#include <iostream>
#include <curl/curl.h>
#include <fstream>
#include <string>
#include <iomanip>
#include <libpq-fe.h>
#include <nlohmann/json.hpp>
#include "player.h"
using namespace std;

/*
TO DO:
implement a way to check if player is already in the table
add a message to tell user searched player does not exist
implement a way to view all players in table
*/


// Callback function to store response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}
Player* getUser(PGconn* conn);
string processRanks(int rank);
void connectDb(Player* p, PGconn* conn);
int main() {
    const char* conninfo = "host=localhost port=5432 dbname=mydb user=postgres password=password";

    PGconn* conn = PQconnectdb(conninfo);

    if (PQstatus(conn) != CONNECTION_OK) {
        cout << "Connection failed: " << PQerrorMessage(conn) << endl;
        PQfinish(conn);
        return 0;
    }
    int option;
    while (true) {
        Player *p = getUser(conn); // assign pointer to a object
        if (p == nullptr) { // if nullptr then restart loop
            continue;
        }
        connectDb(p, conn);
        while (true) {
            cout << "Would you like to keep searching for players? (1. Yes 2. No)" << endl;
            cin >> option;
            if (cin.fail()) {
                cout << "Enter a valid input" << endl;
                cin.clear();
                cin.ignore();
                continue;
            }
            break;
        }
        if (option == 2) { // exit program
            break;
        }

        }
    return 0;
}
Player* getUser(PGconn* conn) {
    string name, platformType, platformFamily;
    cout << "What is your R6 username?" << endl;
    cin >> name;
    cout << "What is your platform type? (uplay, psn, xbl)" << endl;
    cin >> platformType;
    if (platformType == "uplay") {
        platformFamily = "pc";
    }
    else {
        platformFamily = "console";
    }
    string checkQuery = "select 1 from players where name = '" + name + "'"; // check if player is already in database
    PGresult *checkRes = PQexec(conn, checkQuery.c_str());

    if (PQntuples(checkRes) > 0) {
        cout << "Player already exists" << endl;
        return nullptr; // exit function and return nullptr
    }
    string url = "https://api.r6data.eu/api/stats?type=stats&nameOnPlatform=" + name + "&platformType=" + platformType + "&platform_families=" + platformFamily + "";
    CURL* curl;
    CURLcode res;
    string response;


    Player* p;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        // URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Headers
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "api-key: 0e2af6ea1851d4f460a38e78675a8471b032bdc2deea5261cdc60956da84d309b147e6fbb955871fbb40db33ecb080270fde07a0a08bdd057ddf6a5bbc86e260");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Response handling
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Perform request
        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            cerr << "Request failed: " << curl_easy_strerror(res) << endl;
            return nullptr; // api call didn't work, so return nullptr
        } else {
            ofstream outFile("response.json"); // print into a json file
            outFile << response;
            outFile.close(); 
        }
        using json = nlohmann::json;
        json data = json::parse(response); // parse the json file
        json rankedProfile;

        for (auto& board : data["platform_families_full_profiles"][0]["board_ids_full_profiles"]) {
            if (board["board_id"] == "ranked") {
                rankedProfile = board["full_profiles"][0]["profile"];
                break;
            }
        }
        if (rankedProfile.is_null() || rankedProfile.empty()) { // checks for empty json file
            cerr << "No ranked data found for this player" << endl;
            return nullptr; // if empty then there isn't a player
        }
        int rank = rankedProfile["rank"];
        int rank_points = rankedProfile["rank_points"];
        double kills = rankedProfile["kills"];
        double deaths = rankedProfile["deaths"];
        int wins = rankedProfile["wins"];
        int losses = rankedProfile["losses"];
        double kd = kills / deaths;

        string rankName = processRanks(rank);
        kd = round(kd * 100.0) / 100.0;
        p = new Player(name, rankName, kd, wins + losses);

        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return p;

}
void connectDb(Player* p, PGconn* conn) {


    cout << "Insert " << p->getName() << " into table?" << endl; // display player stats for user to confirm
    cout << "Rank: " + p->getRank() << endl;
    cout << "KD: " << p->getKd() << endl;
    cout << "Matches Played: " << p->getMatches() << endl;

    int option;
    while (true) {
        cout << "(1. Yes 2. No)" << endl;
        cin >> option;
        if (cin.fail()) {
            cout << "Enter a valid input" << endl;
            cin.clear();
            cin.ignore();
            continue;
        }
        break;
    }
    if (option == 2) {
        return; // exit function since user doesn't want to add player
    }

    string insertQuery = "insert into players(name, current_rank, kd, matches_played) values ('" + p->getName() + "','" + p->getRank() + "','" + to_string(p->getKd()) + "','" + to_string(p->getMatches()) + "')";
    PGresult *insertRes = PQexec(conn, insertQuery.c_str());

    if (PQresultStatus(insertRes) != PGRES_COMMAND_OK) {
        cout << "Query failed: " << PQerrorMessage(conn) << endl;
    }
    else {
        cout << "Query successful" << endl;
        cout << "Inserted into table: {Player-Name - " << p->getName() << "}" << endl;
    }
    PQclear(insertRes);

    PQfinish(conn);
}
string processRanks(int rank) { // function already done, don't touch
    string tier;
    string division;

    // Determine tier
    if (rank <= 5) tier = "Copper";
    else if (rank <= 10) tier = "Bronze";
    else if (rank <= 15) tier = "Silver";
    else if (rank <= 20) tier = "Gold";
    else if (rank <= 25) tier = "Platinum";
    else if (rank <= 30) tier = "Emerald";
    else if (rank <= 35) tier = "Diamond";
    else tier = "Champion";

    // Determine division (V to I)
    int mod = (rank - 1) % 5;

    switch (mod) {
        case 0: division = "V"; break;
        case 1: division = "IV"; break;
        case 2: division = "III"; break;
        case 3: division = "II"; break;
        case 4: division = "I"; break;
    }

    return tier + " " + division;
}