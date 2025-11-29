#include <iostream>
#include <ctime>
#include <string>
#include <cstdlib>
#include <iomanip>
using namespace std;

string currUser;

int userLogin(string &userName){
    cout << "Enter your display name: ";
    cin >> userName;
    return 0;
}

int greetingInterface() {
    int selectedOption;
    cout << "Welcome,  " << currUser << "." << endl
         << endl
         << "How may I help you?" << endl
         << "1: Record a transation" << setw(7)
         << "2: Set new Budget" << endl
         << "3: Weekly Report" << setw(7)
         << "4: Monthly Report" << endl;
    cout << "Select an option (1-4): ";
    cin >> selectedOption;

    return 0;
}

int main() {
    userLogin(currUser);
    system("cls");
    greetingInterface();
    return 0;
}