#include <string>
using namespace std;
class Player {
    public:
    Player() : rank(""), kd(0), matches_played(0) {}
    Player(string n, string s, double d, int i) : name(n), rank(s), kd(d), matches_played(i) {}
    Player(const Player &other) {
        this->rank = other.rank;
        this->kd = other.kd;
        this->matches_played = other.matches_played;
    }
    string getName() {
        return name;
    }
    string getRank() {
        return rank;
    }
    double getKd() {
        return kd;
    }
    int getMatches() {
        return matches_played;
    }
    private:
    string name = "None";
    string rank;
    double kd;
    int matches_played;

};