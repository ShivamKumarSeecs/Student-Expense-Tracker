#include <iostream>
#include <ctime>
#include <time.h>
#include <fstream>
#include <cstring>
#include <cstdlib> 
#include <iomanip>
#include <string>
#include <algorithm> 
#include <limits> 

using namespace std;

// ================= COLORS =================
// just defined some colors codes to make the console output look beter
const string RESET = "\033[0m";
const string RED = "\033[31m";
const string GREEN = "\033[32m";
const string YELLOW = "\033[33m";
const string CYAN = "\033[36m";
const string WHITE = "\033[37m";

// ================= STRUCTS =================

// LOANS
// structure to hold loan details, keeps track of who owes what
struct loanRecord {
    char date[11];
    char name[50];
    int amount;
    bool receivable;
    bool settled;
    char remarks[100];
};

// smaller struct for the index file so searching is faster
struct loanIndexRecord {
    int recordNum;
    char date[11];
    bool receivable;
};

// EXPENSES
// index record for expenses, stores week and year so reports are easy
struct expIndexRecord {
    int week;
    int year;
    char date[11];
    int recordNum;
};

// main expense data structure
struct expenseRecord {
    char date[11];
    int dayOfWeek;
    int food = 0, transport = 0, entertainment = 0, bills = 0, shopping = 0, rent = 0, misc = 0;
};

// INCOME
// same logic as expenses but for income tracking
struct incomeIndexRecord {
    int week;
    int year;
    char date[11];
    int recordNum;
};

struct incomeRecord {
    char date[11];
    int dayOfWeek;
    int salary = 0, allowance = 0, misc = 0;
};

// BUDGET
// stores the budget limits for a specific month and year
struct budgetRecord {
    int month;
    int year;
    int food = 0, transport = 0, entertainment = 0, bills = 0, shopping = 0, rent = 0, misc = 0;
    int totalLimit = 0;
};


// ================= PROTOTYPES =================

// Validation Helpers
int getValidInt(const char* prompt, bool allowZero = true);
void getValidDate(int& d, int& m, int& y);
void getValidName(char* buffer, int size);

tm getCurrentDate();
void normalizeDate(tm& date);
void initializeFiles();
void showMenu();
void handleReportsMenu(); // NEW: Unified Report Menu

void addRecordExpIndexFile(tm date);
void addRecordExpenseFile();

const char* getDayName(int dayNum);
void displayExpenseFile();

void addRecordIncIndexFile(tm date);
void addRecordIncomeFile();
void displayIncomeFile();
void modifyIncomeRecord(incomeRecord& record, bool& logAdded);

void weeklyReport(int d, int m, int y);
void monthlyReport(int m, int y);
void report(expenseRecord expSum, incomeRecord incSum, budgetRecord budget, int monthTotalExp, const char* title);
void drawBarGraph(string label, int value, int maxScale, int budgetLimit = 0);

void parseDate(const char* dateStr, tm& out);
int getCalendarWeekISO(tm date);

void modifyRecord(expenseRecord& record, bool& logAdded);
void manageLoans();
int countLoans(bool receivable);
void addNewLoan();
void editLoan();
int displayAllLoans(fstream& loanFile, fstream& indexFile);
void handleAmountEdit(loanRecord& record);
void completeLoan(loanRecord& record);

void processExpense(tm date, expenseRecord amountsToAdd);
void processIncome(tm date, incomeRecord amountsToAdd);

void recordLoanTransaction(bool receivable, int amount);

void setBudget();
budgetRecord getBudgetForMonth(int m, int y);
void checkBudgetWarnings(tm date);
int calculateMonthlyGrandTotal(int m, int y);


int main() {
    // clears screen and sets up files before showing the main menu
    system("cls");
    initializeFiles();
    showMenu();
    return 0;
}

// ================= INPUT VALIDATION HELPERS =================

// helper function to make sure user actually types a number
// prevents the program from crashing if they type letters
int getValidInt(const char* prompt, bool allowZero) {
    int val;
    while (true) {
        cout << prompt;
        if (cin >> val) {
            if (val < 0) {
                cout << RED << "   [!] Input cannot be negative. Try again.\n" << RESET;
            }
            else if (!allowZero && val == 0) {
                cout << RED << "   [!] Input cannot be zero. Try again.\n" << RESET;
            }
            else {
                // ignoring rest of line so next cin doesn't break
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                return val;
            }
        }
        else {
            cout << RED << "   [!] Invalid input. Please enter a number.\n" << RESET;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
}

// forces user to enter a valid date format DD MM YYYY
void getValidDate(int& d, int& m, int& y) {
    while (true) {
        cout << ">> Date (DD MM YYYY): ";
        if (cin >> d >> m >> y) {
            if (m < 1 || m > 12) cout << RED << "   [!] Invalid Month (1-12).\n" << RESET;
            else if (d < 1 || d > 31) cout << RED << "   [!] Invalid Day (1-31).\n" << RESET;
            else if (y < 2000 || y > 2100) cout << RED << "   [!] Please enter a realistic year.\n" << RESET;
            else {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                return;
            }
        }
        else {
            cout << RED << "   [!] Invalid format. Use spaces (e.g., 14 11 2024).\n" << RESET;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
}

// ================= UTILITYs & FILES =================

// checks if all the .dat files exist, if not it creates them
// writes a 0 integer at start of index files to init the count
void initializeFiles() {
    const char* files[] = { "LoanIndexFile.dat", "LoanFile.dat", "ExpIndexFile.dat", "ExpenseFile.dat", "IncIndexFile.dat", "IncomeFile.dat", "BudgetFile.dat" };
    int zero = 0;
    for (const char* filename : files) {
        fstream file;
        file.open(filename, ios::in | ios::binary);
        if (!file.is_open()) {
            file.open(filename, ios::out | ios::binary);
            if (strstr(filename, "Index")) file.write(reinterpret_cast<char*>(&zero), sizeof(zero));
        }
        file.close();
    }
}

// sets time to noon to avoid annoying daylight savings shift
void normalizeDate(tm& date) {
    date.tm_hour = 12; date.tm_min = 0; date.tm_sec = 0; date.tm_isdst = -1;
    mktime(&date);
}

// complex math to calcualte the ISO week number so reports are correct
int getCalendarWeekISO(tm date) {
    normalizeDate(date);
    time_t t = mktime(&date);
    tm thursday; localtime_s(&thursday, &t);
    int currentIsoDay = (thursday.tm_wday + 6) % 7;
    thursday.tm_mday += (3 - currentIsoDay);
    normalizeDate(thursday);
    return (thursday.tm_yday / 7) + 1;
}

// gets the current system time
tm getCurrentDate() {
    time_t currentTime = time(nullptr);
    tm localCurrentTime; localtime_s(&localCurrentTime, &currentTime);
    return localCurrentTime;
}

// simple lookup to get string name from day index
const char* getDayName(int dayNum) {
    const char* dayNames[] = { "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday" };
    if (dayNum < 0 || dayNum > 6) return "Unknown";
    return dayNames[dayNum];
}

// converts string date to tm struct
void parseDate(const char* dateStr, tm& out) {
    memset(&out, 0, sizeof(tm));
    int d, m, y;
    sscanf_s(dateStr, "%d-%d-%d", &d, &m, &y);
    out.tm_mday = d; out.tm_mon = m - 1; out.tm_year = y - 1900;
    normalizeDate(out);
}

// counts how many active loans exist in the file
int countLoans(bool receivable) {
    fstream indexFile("LoanIndexFile.dat", ios::in | ios::binary);
    if (!indexFile) return 0;
    int total = 0; indexFile.read(reinterpret_cast<char*>(&total), sizeof(total));
    loanIndexRecord idx; int count = 0;
    while (indexFile.read(reinterpret_cast<char*>(&idx), sizeof(idx))) {
        if (idx.receivable == receivable) count++;
    }
    indexFile.close(); return count;
}

// checks if string contains only letters and spaces
void getValidName(char* buffer, int size) {
    while (true) {
        cout << ">> Friend Name: ";
        cin.getline(buffer, size);

        // Check 1: Is it empty?
        if (strlen(buffer) == 0) {
            cout << RED << "   [!] Name cannot be empty.\n" << RESET;
            continue;
        }

        // Check 2: Does it contain numbers or special symbols?
        bool isValid = true;
        for (int i = 0; buffer[i] != '\0'; i++) {
            // If it's NOT a letter AND NOT a space, it's invalid
            if (!isalpha(buffer[i]) && !isspace(buffer[i])) {
                isValid = false;
                break;
            }
        }

        if (isValid) {
            break; // Input is good, exit loop
        }
        else {
            cout << RED << "   [!] Name should only contain letters (A-Z) and spaces.\n" << RESET;
        }
    }
}

// ================= BACKEND HANDLERS =================

// backend logic to write expense to file
// checks if record for today exists, if so updates it, else creates new
void processExpense(tm date, expenseRecord amountsToAdd) {
    normalizeDate(date);
    expenseRecord fileRecord = {};
    strftime(fileRecord.date, sizeof(fileRecord.date), "%d-%m-%Y", &date);
    fileRecord.dayOfWeek = date.tm_wday;

    expenseRecord tempCheck = {};
    fstream ExpenseFile("ExpenseFile.dat", ios::in | ios::out | ios::binary);

    if (ExpenseFile.is_open()) {
        ExpenseFile.seekg(0, ios::end);
        long fileSize = (long)ExpenseFile.tellg();
        bool found = false;

        // optimization: only check last record to avoid full scan
        if (fileSize > 0) {
            ExpenseFile.seekg(-int(sizeof(tempCheck)), ios::end);
            ExpenseFile.read(reinterpret_cast<char*>(&tempCheck), sizeof(tempCheck));
            if (strcmp(fileRecord.date, tempCheck.date) == 0) {
                found = true; fileRecord = tempCheck;
            }
        }

        // add new amounts to existing values
        fileRecord.food += amountsToAdd.food;
        fileRecord.transport += amountsToAdd.transport;
        fileRecord.entertainment += amountsToAdd.entertainment;
        fileRecord.bills += amountsToAdd.bills;
        fileRecord.shopping += amountsToAdd.shopping;
        fileRecord.rent += amountsToAdd.rent;
        fileRecord.misc += amountsToAdd.misc;

        if (found) {
            ExpenseFile.seekp(-int(sizeof(fileRecord)), ios::end);
            ExpenseFile.write(reinterpret_cast<char*>(&fileRecord), sizeof(fileRecord));
        }
        else {
            ExpenseFile.close();
            // update index file first if its a new record
            addRecordExpIndexFile(date);
            ExpenseFile.open("ExpenseFile.dat", ios::out | ios::app | ios::binary);
            ExpenseFile.write(reinterpret_cast<char*>(&fileRecord), sizeof(fileRecord));
        }
        ExpenseFile.close();
    }
}

// same backend logic but for income records
void processIncome(tm date, incomeRecord amountsToAdd) {
    normalizeDate(date);
    incomeRecord fileRecord = {};
    strftime(fileRecord.date, sizeof(fileRecord.date), "%d-%m-%Y", &date);
    fileRecord.dayOfWeek = date.tm_wday;

    incomeRecord tempCheck = {};
    fstream IncomeFile("IncomeFile.dat", ios::in | ios::out | ios::binary);

    if (IncomeFile.is_open()) {
        IncomeFile.seekg(0, ios::end);
        long fileSize = (long)IncomeFile.tellg();
        bool found = false;

        if (fileSize > 0) {
            IncomeFile.seekg(-int(sizeof(tempCheck)), ios::end);
            IncomeFile.read(reinterpret_cast<char*>(&tempCheck), sizeof(tempCheck));
            if (strcmp(fileRecord.date, tempCheck.date) == 0) {
                found = true; fileRecord = tempCheck;
            }
        }

        fileRecord.salary += amountsToAdd.salary;
        fileRecord.allowance += amountsToAdd.allowance;
        fileRecord.misc += amountsToAdd.misc;

        if (found) {
            IncomeFile.seekp(-int(sizeof(fileRecord)), ios::end);
            IncomeFile.write(reinterpret_cast<char*>(&fileRecord), sizeof(fileRecord));
        }
        else {
            IncomeFile.close();
            addRecordIncIndexFile(date);
            IncomeFile.open("IncomeFile.dat", ios::out | ios::app | ios::binary);
            IncomeFile.write(reinterpret_cast<char*>(&fileRecord), sizeof(fileRecord));
        }
        IncomeFile.close();
    }
}

// ================= MENUS =================

// main menu loop, handles navigation
void showMenu() {
    int choice = 0;
    while (true) {
        system("cls");
        cout << CYAN << "========================================" << endl;
        cout << "        STUDENT EXPENSE TRACKER          " << endl;
        cout << "========================================" << RESET << endl << endl;

        cout << YELLOW << "  [ RECORD ]" << RESET << endl;
        cout << "  1. Add Daily Expense" << endl;
        cout << "  2. Add Income / Pocket Money" << endl << endl;

        cout << YELLOW << "  [ VIEW ]" << RESET << endl;
        cout << "  3. View Expense Log" << endl;
        cout << "  4. View Income Log" << endl << endl;

        cout << YELLOW << "  [ ANALYSIS ]" << RESET << endl;
        cout << "  5. Reports (Graphs & Budgets)" << endl << endl;

        cout << YELLOW << "  [ SETTINGS ]" << RESET << endl;
        cout << "  6. Manage Loans" << endl;
        cout << "  7. Set Monthly Budget" << endl;
        cout << "  0. Exit" << endl << endl;

        cout << CYAN << "========================================" << RESET << endl;
        choice = getValidInt(">> Select Option: ");

        switch (choice) {
        case 1: addRecordExpenseFile(); break;
        case 2: addRecordIncomeFile(); break;
        case 3: displayExpenseFile(); break;
        case 4: displayIncomeFile(); break;
        case 5: handleReportsMenu(); break; // UNIFIED REPORT MENU
        case 6: manageLoans(); break;
        case 7: setBudget(); break;
        case 0: cout << GREEN << "\n>> Goodbye!\n" << RESET; return;
        default: 
            cout << RED << "\n   [!] Invalid Choice.\n" << RESET;
            cout << "   Press [ENTER] to try again...";
            cin.get();
        }
    }
}

// dedicated menu for generating weekly/monthly reports
void handleReportsMenu() {
    system("cls");
    cout << CYAN << "=== REPORTS MENU ===\n\n" << RESET;
    cout << "  1. Weekly Report\n";
    cout << "  2. Monthly Report\n";
    cout << "  0. Back\n\n";

    int type = getValidInt(">> Select Type: ");
    if (type == 0) return;

    // Determine Date
    tm now = getCurrentDate();
    int d = now.tm_mday, m = now.tm_mon + 1, y = now.tm_year + 1900;

    cout << "\n  1. Current Period (" << (type == 1 ? "This Week" : "This Month") << ")\n";
    cout << "  2. Custom Date\n\n";

    int dateChoice = getValidInt(">> Choice: ");
    if (dateChoice == 2) {
        if (type == 1) { // Weekly needs full date
            getValidDate(d, m, y);
        }
        else { // Monthly only needs MM YYYY
            m = getValidInt(">> Enter Month (1-12): ");
            y = getValidInt(">> Enter Year: ");
        }
    }

    if (type == 1) weeklyReport(d, m, y);
    else monthlyReport(m - 1, y); // Adjust for 0-index month

    cout << "\n>> Press [ENTER] to return...";
    cin.get();
}

// ================= REPORTING ENGINE =================

// draws ascii bars for graphs, also handles color coding for budget alerts
void drawBarGraph(string label, int value, int maxScale, int budgetLimit) {
    const int BAR_WIDTH = 40;
    int barLen = 0;
    if (maxScale > 0) barLen = (value * BAR_WIDTH) / maxScale;

    cout << left << setw(15) << label << " |";

    // COLOR LOGIC
    if (budgetLimit > 0 && value > budgetLimit) {
        // Calculate safe portion vs danger portion
        int safeLen = (budgetLimit * BAR_WIDTH) / maxScale;
        int dangerLen = barLen - safeLen;

        // Print Safe part in Green
        cout << GREEN;
        for (int i = 0; i < safeLen; i++) cout << "#";

        // Print Danger part in Red
        cout << RED;
        for (int i = 0; i < dangerLen; i++) cout << "#";
        cout << RESET;

        cout << " (" << value << ")" << RED << " [OVER]" << RESET;
    }
    else {
        // All Safe
        cout << GREEN;
        for (int i = 0; i < barLen; i++) cout << "#";
        cout << RESET;
        cout << " (" << value << ")";
    }
    cout << endl;
}

// main display functoin for reports, calls the graph drawing stuff
void report(expenseRecord expSum, incomeRecord incSum, budgetRecord budget, int monthTotalExp, const char* title) {
    system("cls");
    cout << CYAN << "\n========================================\n";
    cout << "       " << title << "      \n";
    cout << "========================================\n" << RESET;

    int totalInc = incSum.salary + incSum.allowance + incSum.misc;
    int totalExp = expSum.food + expSum.transport + expSum.entertainment + expSum.bills + expSum.shopping + expSum.rent + expSum.misc;

    if (totalInc == 0 && totalExp == 0) {
        cout << YELLOW << "\n  [!] No records found for this period.\n" << RESET;
        return;
    }

    int maxVal = max(totalInc, totalExp);
    if (maxVal == 0) maxVal = 1;

    // OVERVIEW
    cout << YELLOW << " >> OVERVIEW\n" << RESET;
    cout << " ----------------------------------------\n";
    drawBarGraph("Total Income", totalInc, maxVal);
    drawBarGraph("Total Expense", totalExp, maxVal, budget.totalLimit);

    int net = totalInc - totalExp;
    cout << " ----------------------------------------\n";
    cout << " NET SAVINGS   : " << (net >= 0 ? GREEN : RED) << net << " PKR " << RESET << "\n\n";

    // EXPENSE BREAKDOWN
    if (totalExp > 0) {
        cout << YELLOW << " >> EXPENSE BREAKDOWN\n" << RESET;
        cout << " ----------------------------------------\n";

        int maxCat = max({ expSum.food, expSum.transport, expSum.entertainment, expSum.bills, expSum.shopping, expSum.rent, expSum.misc });
        if (maxCat == 0) maxCat = 1;

        drawBarGraph("Food", expSum.food, maxCat, budget.food);
        drawBarGraph("Transport", expSum.transport, maxCat, budget.transport);
        drawBarGraph("Fun/Outing", expSum.entertainment, maxCat, budget.entertainment);
        drawBarGraph("Bills", expSum.bills, maxCat, budget.bills);
        drawBarGraph("Shopping", expSum.shopping, maxCat, budget.shopping);
        drawBarGraph("Rent", expSum.rent, maxCat, budget.rent);
        drawBarGraph("Misc", expSum.misc, maxCat, budget.misc);
        cout << endl;
    }

    // BUDGET STATUS
    if (budget.totalLimit > 0) {
        cout << YELLOW << " >> MONTHLY BUDGET HEALTH\n" << RESET;
        cout << " ----------------------------------------\n";
        cout << left << setw(20) << " Limit:" << budget.totalLimit << "\n";

        int remaining = budget.totalLimit - monthTotalExp;

        if (remaining < 0)
            cout << left << setw(20) << " STATUS:" << RED << "[OVER SPENT BY " << abs(remaining) << "]" << RESET << "\n";
        else {
            cout << left << setw(20) << " Remaining:" << GREEN << remaining << RESET << "\n";
            // Progress Bar
            cout << " [";
            int pct = (monthTotalExp * 20) / budget.totalLimit;
            if (pct > 20) pct = 20;
            cout << (remaining < 0 ? RED : GREEN);
            for (int i = 0; i < pct; i++) cout << "#";
            cout << RESET;
            for (int i = pct; i < 20; i++) cout << ".";
            cout << "] " << (monthTotalExp * 100 / budget.totalLimit) << "%\n";
        }
    }
    cout << CYAN << "========================================\n" << RESET;
}

// ================= FILE OPS & RECORDING =================

// adds a new record to the expense index file
// stores iso week and year so we can filter reports later
void addRecordExpIndexFile(tm date) {
    expIndexRecord record;
    int totalRecords;
    record.week = getCalendarWeekISO(date);
    record.year = date.tm_year + 1900;
    strftime(record.date, sizeof(record.date), "%d-%m-%Y", &date);

    fstream indexFile("ExpIndexFile.dat", ios::in | ios::out | ios::binary);
    if (indexFile.is_open()) {
        indexFile.read(reinterpret_cast<char*>(&totalRecords), sizeof(totalRecords));
        record.recordNum = totalRecords;
        totalRecords++;
        indexFile.seekp(0, ios::beg);
        indexFile.write(reinterpret_cast<char*>(&totalRecords), sizeof(totalRecords));
        indexFile.seekp(0, ios::end);
        indexFile.write(reinterpret_cast<char*>(&record), sizeof(record));
        indexFile.close();
    }
}

// UI wrapper that gets user input then calls backend process
void addRecordExpenseFile() {
    tm date = getCurrentDate();
    expenseRecord inputRecord = {};
    strftime(inputRecord.date, sizeof(inputRecord.date), "%d-%m-%Y", &date);
    bool logAdded = false;
    modifyRecord(inputRecord, logAdded);

    if (logAdded) {
        processExpense(date, inputRecord);
        cout << GREEN << "\n>> Expense Saved Successfully.\n" << RESET;
        checkBudgetWarnings(date);
    }
    cout << ">> Press [ENTER] to return..."; cin.get();
}

// iterates through expense file and prints everything
void displayExpenseFile() {
    system("cls");
    expenseRecord record;
    fstream ExpenseFile("ExpenseFile.dat", ios::in | ios::binary);
    if (ExpenseFile.is_open()) {
        cout << CYAN << "=== MY EXPENSES ===\n\n" << RESET;
        cout << left << setw(12) << "Date" << setw(12) << "Day" << setw(8) << "Food" << setw(12) << "Transport" << setw(10) << "Fun" << setw(8) << "Bills" << setw(10) << "Shop" << setw(8) << "Rent" << setw(8) << "Misc" << endl;
        cout << string(94, '-') << endl;
        while (ExpenseFile.read(reinterpret_cast<char*>(&record), sizeof(record))) {
            cout << left << setw(12) << record.date << setw(12) << getDayName(record.dayOfWeek)
                << setw(8) << record.food << setw(12) << record.transport
                << setw(10) << record.entertainment << setw(8) << record.bills
                << setw(10) << record.shopping << setw(8) << record.rent
                << setw(8) << record.misc << endl;
        }
        ExpenseFile.close();
    }
    else cout << YELLOW << "No records found.\n" << RESET;
    cout << "\n>> Press [ENTER] to return..."; cin.get();
}

// interactive menu to select expense category and amount
void modifyRecord(expenseRecord& record, bool& logAdded) {
    int option = -1, amount;
    while (option != 8) {
        system("cls");
        cout << CYAN << "=== ADD EXPENSE (PKR) ===\n" << RESET;
        cout << "Date: " << record.date << "\n\n";
        cout << "  [1] Food \n  [2] Transport\n  [3] Fun / Outings\n  [4] Bills\n  [5] Shopping\n  [6] Rent\n  [7] Misc\n  [8] Save & Exit\n\n";
        option = getValidInt(">> Select Category: ");
        if (option >= 1 && option <= 7) amount = getValidInt(">> Enter Amount: ");

        switch (option) {
        case 1: record.food += amount; logAdded = true; break;
        case 2: record.transport += amount; logAdded = true; break;
        case 3: record.entertainment += amount; logAdded = true; break;
        case 4: record.bills += amount; logAdded = true; break;
        case 5: record.shopping += amount; logAdded = true; break;
        case 6: record.rent += amount; logAdded = true; break;
        case 7: record.misc += amount; logAdded = true; break;
        case 8: break;
        default: cout << RED << "Invalid.\n" << RESET; cin.get();
        }
    }
}

// writes income metadata to index file
void addRecordIncIndexFile(tm date) {
    incomeIndexRecord record;
    int totalRecords;
    record.week = getCalendarWeekISO(date);
    record.year = date.tm_year + 1900;
    strftime(record.date, sizeof(record.date), "%d-%m-%Y", &date);
    fstream indexFile("IncIndexFile.dat", ios::in | ios::out | ios::binary);
    if (indexFile.is_open()) {
        indexFile.read(reinterpret_cast<char*>(&totalRecords), sizeof(totalRecords));
        record.recordNum = totalRecords;
        totalRecords++;
        indexFile.seekp(0, ios::beg);
        indexFile.write(reinterpret_cast<char*>(&totalRecords), sizeof(totalRecords));
        indexFile.seekp(0, ios::end);
        indexFile.write(reinterpret_cast<char*>(&record), sizeof(record));
        indexFile.close();
    }
}

// UI wrapper to get income input
void addRecordIncomeFile() {
    tm date = getCurrentDate();
    incomeRecord inputRecord = {};
    bool logAdded = false;
    strftime(inputRecord.date, sizeof(inputRecord.date), "%d-%m-%Y", &date);
    modifyIncomeRecord(inputRecord, logAdded);
    if (logAdded) {
        processIncome(date, inputRecord);
        cout << GREEN << "\n>> Income Saved Successfully.\n" << RESET;
    }
    cout << ">> Press [ENTER] to return..."; cin.get();
}

// interactive menu for income categories
void modifyIncomeRecord(incomeRecord& record, bool& logAdded) {
    int option = -1, amount;
    while (option != 4) {
        system("cls");
        cout << CYAN << "=== ADD INCOME (PKR) ===\n" << RESET;
        cout << "  [1] Salary / Stipend\n  [2] Pocket Money\n  [3] Other\n  [4] Save & Exit\n\n";
        option = getValidInt(">> Choice: ");
        if (option >= 1 && option <= 3) amount = getValidInt(">> Amount: ");
        switch (option) {
        case 1: record.salary += amount; logAdded = true; break;
        case 2: record.allowance += amount; logAdded = true; break;
        case 3: record.misc += amount; logAdded = true; break;
        case 4: break;
        }
    }
}

// shows all income history from file
void displayIncomeFile() {
    system("cls");
    incomeRecord record;
    fstream IncomeFile("IncomeFile.dat", ios::in | ios::binary);
    if (IncomeFile.is_open()) {
        cout << CYAN << "=== MY INCOME ===\n\n" << RESET;
        cout << left << setw(12) << "Date" << setw(12) << "Day" << setw(10) << "Salary" << setw(12) << "PocketMon" << setw(10) << "Misc" << endl;
        cout << string(60, '-') << endl;
        while (IncomeFile.read(reinterpret_cast<char*>(&record), sizeof(record))) {
            cout << setw(12) << left << record.date << setw(12) << getDayName(record.dayOfWeek)
                << setw(10) << record.salary << setw(12) << record.allowance << setw(10) << record.misc << endl;
        }
        IncomeFile.close();
    }
    else cout << YELLOW << "No records found.\n" << RESET;
    cout << "\n>> Press [ENTER] to return..."; cin.get();
}

// gathers data for the selected week and calls report generator
void weeklyReport(int d, int m, int y) {
    tm queryDate = {}; queryDate.tm_mday = d; queryDate.tm_mon = m - 1; queryDate.tm_year = y - 1900;
    int targetWeek = getCalendarWeekISO(queryDate);
    int targetYear = queryDate.tm_year + 1900;

    cout << "\n>> Processing Data for Week #" << targetWeek << ", " << targetYear << "...\n";

    expenseRecord expSum = {};
    fstream indexFile("ExpIndexFile.dat", ios::in | ios::binary);
    fstream dataFile("ExpenseFile.dat", ios::in | ios::binary);
    if (indexFile && dataFile) {
        int total; indexFile.read(reinterpret_cast<char*>(&total), sizeof(total));
        expIndexRecord idx; expenseRecord rec;
        for (int i = 0; i < total; i++) {
            indexFile.read(reinterpret_cast<char*>(&idx), sizeof(idx));
            if (idx.week == targetWeek && idx.year == targetYear) {
                dataFile.seekg(idx.recordNum * sizeof(rec));
                dataFile.read(reinterpret_cast<char*>(&rec), sizeof(rec));
                expSum.food += rec.food; expSum.transport += rec.transport; expSum.entertainment += rec.entertainment;
                expSum.bills += rec.bills; expSum.shopping += rec.shopping; expSum.rent += rec.rent; expSum.misc += rec.misc;
            }
        }
    }
    indexFile.close(); dataFile.close();

    incomeRecord incSum = {};
    fstream incIndex("IncIndexFile.dat", ios::in | ios::binary);
    fstream incData("IncomeFile.dat", ios::in | ios::binary);
    if (incIndex && incData) {
        int total; incIndex.read(reinterpret_cast<char*>(&total), sizeof(total));
        incomeIndexRecord idx; incomeRecord rec;
        for (int i = 0; i < total; i++) {
            incIndex.read(reinterpret_cast<char*>(&idx), sizeof(idx));
            if (idx.week == targetWeek && idx.year == targetYear) {
                incData.seekg(idx.recordNum * sizeof(rec));
                incData.read(reinterpret_cast<char*>(&rec), sizeof(rec));
                incSum.salary += rec.salary; incSum.allowance += rec.allowance; incSum.misc += rec.misc;
            }
        }
    }
    incIndex.close(); incData.close();

    budgetRecord budget = getBudgetForMonth(m - 1, y);
    int monthTotal = calculateMonthlyGrandTotal(m - 1, y);
    report(expSum, incSum, budget, monthTotal, "WEEKLY REPORT");
}

// gathers data for month and calls report generator
void monthlyReport(int m, int y) {
    cout << "\n>> Processing Data for Month " << (m + 1) << "-" << y << "...\n";
    expenseRecord expSum = {};
    fstream expFile("ExpenseFile.dat", ios::in | ios::binary);
    if (expFile) {
        expenseRecord rec; tm t = {};
        while (expFile.read(reinterpret_cast<char*>(&rec), sizeof(rec))) {
            parseDate(rec.date, t);
            if (t.tm_mon == m && (t.tm_year + 1900) == y) {
                expSum.food += rec.food; expSum.transport += rec.transport; expSum.entertainment += rec.entertainment;
                expSum.bills += rec.bills; expSum.shopping += rec.shopping; expSum.rent += rec.rent; expSum.misc += rec.misc;
            }
        }
    }
    expFile.close();

    incomeRecord incSum = {};
    fstream incFile("IncomeFile.dat", ios::in | ios::binary);
    if (incFile) {
        incomeRecord rec; tm t = {};
        while (incFile.read(reinterpret_cast<char*>(&rec), sizeof(rec))) {
            parseDate(rec.date, t);
            if (t.tm_mon == m && (t.tm_year + 1900) == y) {
                incSum.salary += rec.salary; incSum.allowance += rec.allowance; incSum.misc += rec.misc;
            }
        }
    }
    incFile.close();

    budgetRecord budget = getBudgetForMonth(m, y);
    int monthTotal = calculateMonthlyGrandTotal(m, y);
    report(expSum, incSum, budget, monthTotal, "MONTHLY REPORT");
}

// submenu for managing user loans
void manageLoans() {
    int choice;
    do {
        system("cls");
        cout << CYAN << "===== LOAN MANAGER =====\n\n" << RESET;
        cout << "  Friends Owe Me : " << countLoans(true) << endl;
        cout << "  I Owe Others   : " << countLoans(false) << endl << endl;
        cout << "  1. Add New Loan\n  2. View / Edit Loans\n  0. Main Menu\n\n";
        choice = getValidInt(">> Choice: ");
        if (choice == 1) addNewLoan();
        else if (choice == 2) editLoan();
        else if (choice != 0) {
            cout << RED << "   [!] Invalid option.\n" << RESET;
            cout << "   Press [ENTER] to try again...";
            cin.get(); // Pauses so they can read the error
        }
    } while (choice != 0);
}

// logic to add a new loan entry to file
void addNewLoan() {
    system("cls");
    cout << CYAN << "=== ADD NEW LOAN ===\n\n" << RESET;
    loanRecord newLoan{};
    tm date = getCurrentDate();
    int choice;

    while (true) {
        cout << "  1. Today\n  2. Other Date\n  0. Back\n";
        choice = getValidInt(">> Choice: ");

        if (choice == 0) return;
        if (choice == 1 || choice == 2) break; // Valid input, exit loop

        cout << RED << "   [!] Invalid option. Please select 1 or 2.\n\n" << RESET;
    }

    if (choice == 2) {
        int d, m, y; getValidDate(d, m, y);
        date.tm_mday = d; date.tm_mon = m - 1; date.tm_year = y - 1900;
    }
    strftime(newLoan.date, sizeof(newLoan.date), "%d-%m-%Y", &date);

    while (true) {
        cout << "\n  1. Receivable (Friend owes me)\n  2. Payable (I owe friend)\n";
        choice = getValidInt(">> Choice: ");

        if (choice == 1 || choice == 2) break; // Valid input, exit loop

        cout << RED << "   [!] Invalid option. Please select 1 or 2.\n" << RESET;
    }
    newLoan.receivable = (choice == 1);
    newLoan.settled = false;

    getValidName(newLoan.name, 50);
    newLoan.amount = getValidInt(">> Amount (PKR): ", false);
    cout << ">> Note: "; cin.getline(newLoan.remarks, 100);

    fstream loanFile("LoanFile.dat", ios::app | ios::binary);
    loanFile.write(reinterpret_cast<char*>(&newLoan), sizeof(newLoan));
    loanFile.close();

    fstream indexFile("LoanIndexFile.dat", ios::in | ios::out | ios::binary);
    int total; indexFile.read(reinterpret_cast<char*>(&total), sizeof(total));
    total++; indexFile.seekp(0); indexFile.write(reinterpret_cast<char*>(&total), sizeof(total));
    loanIndexRecord idx; idx.recordNum = total - 1; strcpy_s(idx.date, sizeof(idx.date), newLoan.date); idx.receivable = newLoan.receivable;
    indexFile.seekp(0, ios::end); indexFile.write(reinterpret_cast<char*>(&idx), sizeof(idx));
    indexFile.close();
    cout << GREEN << "\n>> Saved.\n" << RESET; 
    cout << ">> Press [ENTER] to return...";
    cin.get();
}

// lists all loans on screen so user can pick one
int displayAllLoans(fstream& loanFile, fstream& indexFile) {
    system("cls");
    cout << CYAN << "=== LOAN LIST ===\n\n" << RESET;
    int total; indexFile.seekg(0); indexFile.read(reinterpret_cast<char*>(&total), sizeof(total));
    if (total == 0) { cout << YELLOW << ">> No records.\n" << RESET; return 0; }
    loanIndexRecord idx; loanRecord rec;
    cout << left << setw(4) << "No" << setw(12) << "Date" << setw(18) << "Friend" << setw(10) << "PKR" << setw(12) << "Type" << setw(10) << "Status" << "Note\n";
    cout << string(80, '-') << endl;
    for (int i = 0; i < total; i++) {
        indexFile.seekg(sizeof(int) + i * sizeof(loanIndexRecord));
        indexFile.read(reinterpret_cast<char*>(&idx), sizeof(idx));
        loanFile.seekg(idx.recordNum * sizeof(loanRecord));
        loanFile.read(reinterpret_cast<char*>(&rec), sizeof(rec));
        cout << setw(4) << i + 1 << setw(12) << rec.date << setw(18) << rec.name
            << setw(10) << rec.amount << setw(12) << (rec.receivable ? "Receivable" : "Payable")
            << setw(10) << (rec.settled ? "Cleared" : "Pending") << rec.remarks << endl;
    }
    return total;
}

// menu for editing specific loan details
void editLoan() {
    fstream loanFile("LoanFile.dat", ios::in | ios::out | ios::binary);
    fstream indexFile("LoanIndexFile.dat", ios::in | ios::binary);
    if (!loanFile || !indexFile) return;
    int total = displayAllLoans(loanFile, indexFile);
    if (total == 0) { 
        cin.get();
        cout << ">> Press [ENTER] to return...";
        return;
    }
    int choice;

    // loop until u enter a valid number between 1 and total
    while (true) {
        choice = getValidInt("\n>> Select Loan No. (0 to Back): ");

        if (choice == 0) return; // Exit

        // Is choice within range?
        if (choice > 0 && choice <= total) {
            break; // Valid input, break the loop
        }

        // Error message
        
        cout << RED;
        if (total == 1) cout << "   [!] Invalid selection. Please choose between 1 or exit";
        else cout << "   [!] Invalid selection. Please choose between 1 and " << total;
        cout <<  ".\n" << RESET;
    }
    choice--;

    loanIndexRecord idx; loanRecord rec;
    indexFile.seekg(sizeof(int) + choice * sizeof(idx)); indexFile.read(reinterpret_cast<char*>(&idx), sizeof(idx));
    loanFile.seekg(idx.recordNum * sizeof(rec)); loanFile.read(reinterpret_cast<char*>(&rec), sizeof(rec));

    int opt = -1;
    while (opt != 0) {
        system("cls");
        cout << CYAN << "=== EDIT LOAN ===\n" << RESET;
        cout << "Friend: " << rec.name << " | Amount: " << rec.amount << "\n\n";
        cout << "  1. Change Amount\n  2. Change Name\n  3. Change Note\n  4. Mark Cleared\n  0. Save\n";
        opt = getValidInt(">> Choice: ");
        if (opt == 1) handleAmountEdit(rec);
        else if (opt == 2) { cout << ">> Name: "; cin.getline(rec.name, 50); }
        else if (opt == 3) { cout << ">> Note: "; cin.getline(rec.remarks, 100); }
        else if (opt == 4) completeLoan(rec);
    }
    loanFile.seekp(idx.recordNum * sizeof(rec)); loanFile.write(reinterpret_cast<char*>(&rec), sizeof(rec));
    loanFile.close(); indexFile.close();

    cout << GREEN << "\n>> Changes Saved.\n" << RESET; 
    cout << ">> Press [ENTER] to return...";           
    cin.get();
}

// handles logic if user changes loan amount, can record diff as transaction
void handleAmountEdit(loanRecord& record) {
    int newAmount = getValidInt(">> New Amount: ", false);
    int diff = newAmount - record.amount;
    if (diff == 0) return;
    char ans; cout << ">> Record diff (" << diff << ") as transaction? (y/n): "; cin >> ans;
    if (ans == 'y' || ans == 'Y') {
        int absAmount = abs(diff);
        bool isIncome = (record.receivable && diff < 0) || (!record.receivable && diff > 0);
        recordLoanTransaction(isIncome, absAmount);
    }
    record.amount = newAmount; cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// marks loan as settled and records transaction if needed
void completeLoan(loanRecord& record) {
    if (record.amount == 0) { record.settled = true; return; }
    char ans; cout << ">> Add settlement to transactions? (y/n): "; cin >> ans;
    if (ans == 'y' || ans == 'Y') recordLoanTransaction(record.receivable, record.amount);
    record.amount = 0; record.settled = true; cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// auto-generates expense or income when loan changes
void recordLoanTransaction(bool receivable, int amount) {
    tm date = getCurrentDate();
    if (receivable) {
        incomeRecord inc = {}; inc.misc = amount; processIncome(date, inc); cout << GREEN << ">> Recorded as Income.\n" << RESET;
    }
    else {
        expenseRecord exp = {}; exp.misc = amount; processExpense(date, exp); cout << RED << ">> Recorded as Expense.\n" << RESET;
    }
}

// interface for user to set their budget limits
void setBudget() {
    system("cls");
    cout << CYAN << "=== MONTHLY BUDGET SETTINGS ===\n\n" << RESET;
    tm date = getCurrentDate();
    budgetRecord rec = getBudgetForMonth(date.tm_mon, date.tm_year + 1900);
    rec.month = date.tm_mon; rec.year = date.tm_year + 1900;

    cout << ">> Setting Budget for: " << (rec.month + 1) << "-" << rec.year << endl;
    cout << "--------------------------------\n";

    cout << "  1. Set Total Budget Limit\n";
    cout << "  2. Set Category-wise Limits\n";
    cout << "  0. Back\n\n";

    int choice = getValidInt(">> Choice: ");

    if (choice == 0) return; 

    if (choice == 1) {
        rec.totalLimit = getValidInt(">> Enter Monthly Limit: ");

        // Reset categories so Total Limit takes precedence
        rec.food = 0; rec.transport = 0; rec.entertainment = 0;
        rec.bills = 0; rec.shopping = 0; rec.rent = 0; rec.misc = 0;

        cout << GREEN << "\n>> Global Limit set to " << rec.totalLimit << ".\n" << RESET;
    }
    else if (choice == 2) {
        cout << "\n>> Enter Category Limits:\n";
        rec.food = getValidInt("   [1] Food:      ");
        rec.transport = getValidInt("   [2] Transport: ");
        rec.entertainment = getValidInt("   [3] Outings:   ");
        rec.bills = getValidInt("   [4] Bills:     ");
        rec.shopping = getValidInt("   [5] Shopping:  ");
        rec.rent = getValidInt("   [6] Rent:      ");
        rec.misc = getValidInt("   [7] Misc:      ");

        rec.totalLimit = rec.food + rec.transport + rec.entertainment + rec.bills + rec.shopping + rec.rent + rec.misc;
        cout << "\n>> Effective Total Budget: " << rec.totalLimit << endl;
    }
    else {
        cout << RED << "\n   [!] Invalid Choice.\n" << RESET;
        cout << "   Press [ENTER] to return...";
        cin.get();
        return;
    }

    fstream file("BudgetFile.dat", ios::app | ios::binary);
    file.write(reinterpret_cast<char*>(&rec), sizeof(rec));
    file.close();
    cout << GREEN << "\n>> Budget Saved!\n" << RESET; 
    cout << ">> Press [ENTER] to return...";
    cin.get();
}

// checks if current spending is over budget and warns user
void checkBudgetWarnings(tm date) {
    budgetRecord budget = getBudgetForMonth(date.tm_mon, date.tm_year + 1900);
    if (budget.totalLimit == 0) return;
    int currentTotal = calculateMonthlyGrandTotal(date.tm_mon, date.tm_year + 1900);
    cout << "\n----------------------------------------\n";
    if (currentTotal > budget.totalLimit) cout << RED << " [CRITICAL] Monthly Budget EXCEEDED! (" << currentTotal << "/" << budget.totalLimit << ")\n" << RESET;
    else if (currentTotal > (budget.totalLimit * 0.8)) cout << YELLOW << " [WARNING]  You have used " << (currentTotal * 100 / budget.totalLimit) << "% of your monthly budget.\n" << RESET;
    else cout << GREEN << " [INFO]     Monthly Budget Left: " << (budget.totalLimit - currentTotal) << "\n" << RESET;
    cout << "------------------------------------edi----\n";
}

// iterates through file to sum up expenses for the month
int calculateMonthlyGrandTotal(int m, int y) {
    fstream file("ExpenseFile.dat", ios::in | ios::binary);
    if (!file) return 0;
    expenseRecord rec; int total = 0; tm t = {};
    while (file.read(reinterpret_cast<char*>(&rec), sizeof(rec))) {
        parseDate(rec.date, t);
        if (t.tm_mon == m && (t.tm_year + 1900) == y) total += (rec.food + rec.transport + rec.entertainment + rec.bills + rec.shopping + rec.rent + rec.misc);
    }
    file.close(); return total;
}

// tries to find budget in file, or carries over from prev month
budgetRecord getBudgetForMonth(int m, int y) {
    budgetRecord rec; budgetRecord foundRec = {}; bool found = false;
    fstream file("BudgetFile.dat", ios::in | ios::binary);
    if (!file) return foundRec;
    while (file.read(reinterpret_cast<char*>(&rec), sizeof(rec))) {
        if (rec.month == m && rec.year == y) { foundRec = rec; found = true; }
    }
    file.close();
    if (found) return foundRec;

    int prevM = (m == 0) ? 11 : m - 1; int prevY = (m == 0) ? y - 1 : y;
    file.open("BudgetFile.dat", ios::in | ios::binary);
    while (file.read(reinterpret_cast<char*>(&rec), sizeof(rec))) {
        if (rec.month == prevM && rec.year == prevY) { foundRec = rec; found = true; }
    }
    file.close();
    if (found) {
        foundRec.month = m; foundRec.year = y;
        file.open("BudgetFile.dat", ios::app | ios::binary);
        file.write(reinterpret_cast<char*>(&foundRec), sizeof(foundRec));
        file.close();
        return foundRec;
    }
    return {};
}