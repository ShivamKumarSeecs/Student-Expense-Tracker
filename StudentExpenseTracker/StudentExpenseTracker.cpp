#include <iostream>
#include <ctime>
#include <time.h>
#include <fstream>
#include <cstring>
#include <cstdlib> // for atoi
#include <iomanip>
#include <vector>

using namespace std;

// using 4 files:
/*ExpIndexFile.dat stores metadata*/
/*ExpenseFile stores data*/
/*IncIndexFile.dat stores metadata*/
/*IncomeFile stores data*/

// LOANS
struct loanRecord {
    char date[11];
    char name[50];
    int amount;
    bool receivable;
    bool settled;
    char remarks[100];
};

struct loanIndexRecord {
    int recordNum;
    char date[11];
    bool receivable;
};

// EXPENSES
struct expIndexRecord {
    int week;       // Stores Calendar Week (1-53)
    char date[11];
    int recordNum;
};

struct expenseRecord {
    char date[11];
    int dayOfWeek;
    int food = 0, transport = 0, entertainment = 0, bills = 0, shopping = 0, rent = 0, misc = 0;
};

// INCOME
struct incomeIndexRecord {
    int week;       // Stores Calendar Week (1-53)
    char date[11];
    int recordNum;
};

struct incomeRecord {
    char date[11];
    int dayOfWeek;
    int salary = 0, allowance = 0, misc = 0;
};


// ================= PROTOTYPES =================

tm getCurrentDate();
void normalizeDate(tm& date); // Helper to fix date logic
void initializeFiles();
void showMenu();

void addRecordExpIndexFile(tm date);
void addRecordExpenseFile();

const char* getDayName(int dayNum);
void displayExpenseFile();
void displayExpRecord(expenseRecord record);

void addRecordIncIndexFile(tm date);
void addRecordIncomeFile();
void displayIncomeFile();
void modifyIncomeRecord(incomeRecord& record, bool& logAdded);

void weeklyReport(int d, int m, int y);
void report(int startRec, int endRec);

void parseDate(const char* dateStr, tm& out);
int getCalendarWeek(tm date); // Pass by value to allow internal normalization

void modifyRecord(expenseRecord& record, bool& logAdded);
void manageLoans();
int countLoans(bool recievable);
void addNewLoan();
void editLoan();
int displayAllLoans(fstream& loanFile, fstream& indexFile);
void recordLoanTransaction(bool receivable, int amount, const char* dateStr);
void handleAmountEdit(loanRecord& record);
void completeLoan(loanRecord& record);


int main() {
    initializeFiles();
    showMenu();
    return 0;
}

// ================= UTILS & FILES =================

void initializeFiles() {
    const char* files[] = { "LoanIndexFile.dat", "LoanFile.dat", "ExpIndexFile.dat", "ExpenseFile.dat", "IncIndexFile.dat", "IncomeFile.dat" };
    int zero = 0;

    for (const char* filename : files) {
        fstream file;
        file.open(filename, ios::in | ios::binary);
        if (!file.is_open()) {
            file.open(filename, ios::out | ios::binary);
            // Write 0 total records for index files
            if (strstr(filename, "Index")) {
                file.write(reinterpret_cast<char*>(&zero), sizeof(zero));
            }
        }
        file.close();
    }
}

// Ensures the date struct is consistent (calculates day of week/year correctly)
void normalizeDate(tm& date) {
    date.tm_hour = 12; // Set to noon to avoid DST boundary issues
    date.tm_min = 0;
    date.tm_sec = 0;
    date.tm_isdst = -1; // Auto-detect DST
    mktime(&date);      // This calculates tm_yday and tm_wday based on D/M/Y
}

// Returns the ISO-like week number (1-53)
// Logic: Shifts the date to the Thursday of that week, then calculates the week number.
// This ensures that Mon-Sun are always grouped together, even across month boundaries.
int getCalendarWeek(tm date) {
    normalizeDate(date);

    // Create a time_t from the date to manipulate it safely
    time_t t = mktime(&date);

    // We need a safe local copy to modify
    tm thursday;
    localtime_s(&thursday, &t);

    // Calculate shift needed to get to Thursday
    // (tm_wday + 6) % 7 converts Sunday(0) to 6, Monday(1) to 0, etc.
    // This creates a Monday-based index.
    int currentIsoDay = (thursday.tm_wday + 6) % 7;

    // Shift the day to Thursday (3 is the index for Thursday in 0-6 range)
    thursday.tm_mday += (3 - currentIsoDay);

    // Recalculate yday for the specific Thursday of this week
    normalizeDate(thursday);

    // Return standard week number
    return (thursday.tm_yday / 7) + 1;
}

tm getCurrentDate() {
    time_t currentTime = time(nullptr);
    tm localCurrentTime;
    localtime_s(&localCurrentTime, &currentTime);
    return localCurrentTime;
}

const char* getDayName(int dayNum) {
    const char* dayNames[] = { "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday" };
    return dayNames[dayNum];
}

void parseDate(const char* dateStr, tm& out) {
    memset(&out, 0, sizeof(tm));
    int d, m, y;
    sscanf_s(dateStr, "%d-%d-%d", &d, &m, &y);
    out.tm_mday = d;
    out.tm_mon = m - 1;
    out.tm_year = y - 1900;
    normalizeDate(out); // Ensure yday is calculated immediately
}

// Counts active loans based on type (Receivable vs Payable)
int countLoans(bool recievable) {
    fstream indexFile("LoanIndexFile.dat", ios::in | ios::binary);
    if (!indexFile) return 0;

    int total = 0;
    indexFile.read(reinterpret_cast<char*>(&total), sizeof(total));

    loanIndexRecord idx;
    int count = 0;

    while (indexFile.read(reinterpret_cast<char*>(&idx), sizeof(idx))) {
        if (idx.receivable == recievable)
            count++;
    }

    indexFile.close();
    return count;
}

// ================= MENU =================

void showMenu() {
    int choice = 0;

    while (true) {
        system("cls");
        cout << "========================================" << endl;
        cout << "        STUDENT BUDGET TRACKER          " << endl;
        cout << "========================================" << endl << endl;

        cout << "  [1] Add Daily Expense" << endl;
        cout << "  [2] Add Income / Pocket Money" << endl;
        cout << "  [3] View My Expenses" << endl;
        cout << "  [4] View My Income" << endl;
        cout << "  [5] Weekly Spending Report" << endl;
        cout << "  [6] Manage Loans" << endl;
        cout << "  [7] Exit" << endl << endl;
        cout << "========================================" << endl;
        cout << ">> Select Option (1-7): ";
        cin >> choice;

        switch (choice) {
        case 1: addRecordExpenseFile(); break;
        case 2: addRecordIncomeFile(); break;
        case 3: displayExpenseFile(); break;
        case 4: displayIncomeFile(); break;
        case 5: {
            system("cls");
            cout << "=== WEEKLY SPENDING CHECK ===\n\n";
            int d, m, y;
            cout << ">> Enter ANY date within the week you want to check.\n";
            cout << ">> Date (DD MM YYYY): ";
            cin >> d >> m >> y;
            weeklyReport(d, m, y);
            cout << "\n>> Press [ENTER] to return to Main Menu...";
            cin.ignore(); cin.get();
            break;
        }
        case 6: manageLoans(); break;
        case 7: cout << "\n>> Saving data... Goodbye!\n"; return;
        default:
            cout << "\n   [!] Invalid Choice. Press [ENTER]...";
            cin.ignore(); cin.get();
        }
    }
}

// ================= EXPENSE FUNCTIONS =================

void addRecordExpIndexFile(tm date) {
    expIndexRecord record;
    int totalRecords; // Use a separate variable for the count

    record.week = getCalendarWeek(date);
    strftime(record.date, sizeof(record.date), "%d-%m-%Y", &date);

    fstream indexFile("ExpIndexFile.dat", ios::in | ios::out | ios::binary);
    if (indexFile.is_open()) {
        // Read current total
        indexFile.read(reinterpret_cast<char*>(&totalRecords), sizeof(totalRecords));

        // CRITICAL FIX: The ID of the new record is the current total (0-indexed)
        record.recordNum = totalRecords;

        // Increment total for the header
        totalRecords++;

        // Write new total to header
        indexFile.seekp(0, ios::beg);
        indexFile.write(reinterpret_cast<char*>(&totalRecords), sizeof(totalRecords));

        // Write new record to end
        indexFile.seekp(0, ios::end);
        indexFile.write(reinterpret_cast<char*>(&record), sizeof(record));
        indexFile.close();
    }
}

void addRecordExpenseFile() {
    tm date = getCurrentDate();
    normalizeDate(date); // Ensure date is valid and dayOfWeek is set

    expenseRecord record;
    strftime(record.date, sizeof(record.date), "%d-%m-%Y", &date);

    expenseRecord checkRecord = {};
    fstream ExpenseFile;

    // Check last record
    ExpenseFile.open("ExpenseFile.dat", ios::in | ios::out | ios::binary);
    if (ExpenseFile.is_open()) {
        ExpenseFile.seekg(0, ios::end);
        if (ExpenseFile.tellg() > 0) {
            ExpenseFile.seekg(-int(sizeof(checkRecord)), ios::end);
            ExpenseFile.read(reinterpret_cast<char*>(&checkRecord), sizeof(checkRecord));
        }

        if (strcmp(record.date, checkRecord.date) == 0) {
            record = checkRecord;
            system("cls");
            cout << ">> RECORD FOUND FOR TODAY [" << record.date << "]\n";
            displayExpRecord(record);
            cout << "\n>> Adding to today's record...\n";
            cout << ">> Press [ENTER] to continue...";
            cin.ignore(); cin.get();
            ExpenseFile.seekp(-int(sizeof(record)), ios::end); // Overwrite mode
        }
        else {
            record.dayOfWeek = date.tm_wday;
            addRecordExpIndexFile(date); // Create new index
            ExpenseFile.seekp(0, ios::end); // Append mode
        }

        bool logAdded = false;
        modifyRecord(record, logAdded);

        if (logAdded) {
            ExpenseFile.write(reinterpret_cast<char*>(&record), sizeof(record));
        }
        ExpenseFile.close();
    }
}

void displayExpenseFile() {
    system("cls");
    expenseRecord record;
    fstream ExpenseFile("ExpenseFile.dat", ios::in | ios::binary);

    if (ExpenseFile.is_open()) {
        cout << "=== MY EXPENSES ===\n\n";
        cout << left << setw(12) << "Date" << setw(10) << "Day"
            << setw(8) << "Food" << setw(12) << "Transport"
            << setw(10) << "Fun" << setw(8) << "Bills"
            << setw(10) << "Shopping" << setw(8) << "Rent"
            << setw(8) << "Misc" << endl;
        cout << string(92, '-') << endl;

        while (ExpenseFile.read(reinterpret_cast<char*>(&record), sizeof(record))) {
            cout << left << setw(12) << record.date << setw(10) << record.dayOfWeek
                << setw(8) << record.food << setw(12) << record.transport
                << setw(10) << record.entertainment << setw(8) << record.bills
                << setw(10) << record.shopping << setw(8) << record.rent
                << setw(8) << record.misc << endl;
        }
        ExpenseFile.close();
    }
    else {
        cout << "No records found.\n";
    }
    cout << "\n>> Press [ENTER] to return...";
    cin.ignore(); cin.get();
}

void modifyRecord(expenseRecord& record, bool& logAdded) {
    int option = -1, amount;
    while (option != 8) {
        system("cls");
        cout << "=== ADD EXPENSE (PKR) ===\n";
        cout << "Date: " << record.date << "\n\n";
        cout << "  [1] Food \n  [2] Transport\n  [3] Fun / Outings\n  [4] Bills\n";
        cout << "  [5] Shopping\n  [6] Rent\n  [7] Misc\n  [8] Save & Exit\n\n";
        cout << ">> Select Category: ";
        cin >> option;

        if (option >= 1 && option <= 7) {
            cout << ">> Enter Amount (PKR): "; cin >> amount;
        }

        switch (option) {
        case 1: record.food += amount; logAdded = true; break;
        case 2: record.transport += amount; logAdded = true; break;
        case 3: record.entertainment += amount; logAdded = true; break;
        case 4: record.bills += amount; logAdded = true; break;
        case 5: record.shopping += amount; logAdded = true; break;
        case 6: record.rent += amount; logAdded = true; break;
        case 7: record.misc += amount; logAdded = true; break;
        case 8: break;
        default: cout << "Invalid. Press Enter."; cin.ignore(); cin.get();
        }
    }
}

void displayExpRecord(expenseRecord record) {
    cout << "--------------------------------\n";
    cout << "  Food: " << record.food << " | Transport: " << record.transport << " | Fun: " << record.entertainment << "\n";
    cout << "  Bills: " << record.bills << " | Shop: " << record.shopping << " | Rent: " << record.rent << " | Misc: " << record.misc << "\n";
    cout << "--------------------------------\n";
}

// ================= INCOME FUNCTIONS =================

void addRecordIncIndexFile(tm date) {
    incomeIndexRecord record;
    int totalRecords; // Use a separate variable

    record.week = getCalendarWeek(date);
    strftime(record.date, sizeof(record.date), "%d-%m-%Y", &date);

    fstream indexFile("IncIndexFile.dat", ios::in | ios::out | ios::binary);
    if (indexFile.is_open()) {
        indexFile.read(reinterpret_cast<char*>(&totalRecords), sizeof(totalRecords));

        // CRITICAL FIX: ID = current total
        record.recordNum = totalRecords;

        totalRecords++;

        indexFile.seekp(0, ios::beg);
        indexFile.write(reinterpret_cast<char*>(&totalRecords), sizeof(totalRecords));

        indexFile.seekp(0, ios::end);
        indexFile.write(reinterpret_cast<char*>(&record), sizeof(record));
        indexFile.close();
    }
}

void addRecordIncomeFile() {
    tm date = getCurrentDate();
    normalizeDate(date);
    incomeRecord record = {};
    strftime(record.date, sizeof(record.date), "%d-%m-%Y", &date);

    incomeRecord checkRecord = {};
    fstream IncomeFile;
    IncomeFile.open("IncomeFile.dat", ios::in | ios::out | ios::binary);

    if (IncomeFile.is_open()) {
        IncomeFile.seekg(0, ios::end);
        if (IncomeFile.tellg() > 0) {
            IncomeFile.seekg(-int(sizeof(checkRecord)), ios::end);
            IncomeFile.read(reinterpret_cast<char*>(&checkRecord), sizeof(checkRecord));
        }

        if (strcmp(record.date, checkRecord.date) == 0) {
            record = checkRecord;
            system("cls");
            cout << ">> INCOME RECORD FOUND FOR TODAY. Modifying...\n";
            IncomeFile.seekp(-int(sizeof(record)), ios::end);
        }
        else {
            record.dayOfWeek = date.tm_wday;
            addRecordIncIndexFile(date);
            IncomeFile.seekp(0, ios::end);
        }

        bool logAdded = false;
        modifyIncomeRecord(record, logAdded);

        if (logAdded) {
            IncomeFile.write(reinterpret_cast<char*>(&record), sizeof(record));
            cout << "\n>> Income Added Successfully.\n";
        }
        IncomeFile.close();
    }
    cout << ">> Press [ENTER] to return...";
    cin.ignore(); cin.get();
}

void modifyIncomeRecord(incomeRecord& record, bool& logAdded) {
    int option = -1, amount;
    while (option != 4) {
        system("cls");
        cout << "=== ADD INCOME (PKR) ===\n";
        cout << "  [1] Salary / Stipend\n  [2] Pocket Money\n  [3] Other\n  [4] Save & Exit\n\n";
        cout << ">> Choice: "; cin >> option;

        if (option >= 1 && option <= 3) {
            cout << ">> Amount: "; cin >> amount;
        }

        switch (option) {
        case 1: record.salary += amount; logAdded = true; break;
        case 2: record.allowance += amount; logAdded = true; break;
        case 3: record.misc += amount; logAdded = true; break;
        case 4: break;
        }
    }
}

void displayIncomeFile() {
    system("cls");
    incomeRecord record;
    fstream IncomeFile("IncomeFile.dat", ios::in | ios::binary);
    if (IncomeFile.is_open()) {
        cout << "=== MY INCOME ===\n\n";
        cout << left << setw(12) << "Date" << setw(10) << "Salary" << setw(12) << "PocketMon" << setw(10) << "Misc" << endl;
        cout << string(45, '-') << endl;
        while (IncomeFile.read(reinterpret_cast<char*>(&record), sizeof(record))) {
            cout << setw(12) << left << record.date << setw(10) << record.salary
                << setw(12) << record.allowance << setw(10) << record.misc << endl;
        }
        IncomeFile.close();
    }
    cout << "\n>> Press [ENTER] to return...";
    cin.ignore(); cin.get();
}

// ================= REPORTING =================

void weeklyReport(int d, int m, int y) {
    tm queryDate = {};
    queryDate.tm_mday = d;
    queryDate.tm_mon = m - 1;
    queryDate.tm_year = y - 1900;

    // Calculate week based on NORMALIZED date
    // This now strictly uses the ISO logic (Monday-Sunday grouping)
    int targetWeek = getCalendarWeek(queryDate);

    cout << "\n>> Generating Report for Calendar Week #" << targetWeek << "...\n";

    fstream indexFile("ExpIndexFile.dat", ios::in | ios::binary);
    if (!indexFile) { cout << "Error opening index file\n"; return; }

    int totalRecords;
    indexFile.read(reinterpret_cast<char*>(&totalRecords), sizeof(totalRecords));

    expIndexRecord idx;
    int startRec = -1, endRec = -1;

    for (int i = 0; i < totalRecords; i++) {
        indexFile.read(reinterpret_cast<char*>(&idx), sizeof(idx));
        if (idx.week == targetWeek) {
            if (startRec == -1) startRec = idx.recordNum;
            endRec = idx.recordNum;
        }
    }
    indexFile.close();

    if (startRec == -1) {
        cout << "\n>> No spending records found for Week #" << targetWeek << ".\n";
        return;
    }
    report(startRec, endRec);
}

void report(int startRec, int endRec) {
    expenseRecord record;
    int totalFood = 0, totalTransport = 0, totalEntertainment = 0, totalBills = 0, totalShopping = 0, totalRent = 0, totalMisc = 0;

    fstream expenseFile("ExpenseFile.dat", ios::in | ios::binary);
    if (expenseFile.is_open()) {
        expenseFile.seekg(startRec * sizeof(expenseRecord), ios::beg);
        for (int i = startRec; i <= endRec; i++) {
            if (expenseFile.read(reinterpret_cast<char*>(&record), sizeof(record))) {
                totalFood += record.food;
                totalTransport += record.transport;
                totalEntertainment += record.entertainment;
                totalBills += record.bills;
                totalShopping += record.shopping;
                totalRent += record.rent;
                totalMisc += record.misc;
            }
        }
        expenseFile.close();
    }

    cout << "\n====================================\n";
    cout << "       WEEKLY SPENDING SUMMARY      \n";
    cout << "====================================\n";
    cout << left << setw(20) << "Category" << "Amount (PKR)" << endl;
    cout << string(36, '-') << endl;
    cout << left << setw(20) << "Food/Chai" << totalFood << "\n";
    cout << left << setw(20) << "Transport" << totalTransport << "\n";
    cout << left << setw(20) << "Outings" << totalEntertainment << "\n";
    cout << left << setw(20) << "Bills" << totalBills << "\n";
    cout << left << setw(20) << "Shopping" << totalShopping << "\n";
    cout << left << setw(20) << "Rent" << totalRent << "\n";
    cout << left << setw(20) << "Misc" << totalMisc << "\n";
    cout << string(36, '-') << endl;
    cout << left << setw(20) << "TOTAL SPENT" << (totalFood + totalTransport + totalEntertainment + totalBills + totalShopping + totalRent + totalMisc) << "\n";
    cout << "====================================\n";
}

// ================= LOANS =================

void manageLoans() {
    int choice;
    do {
        system("cls");
        cout << "===== LOAN MANAGER =====\n\n";
        cout << "  Friends Owe Me : " << countLoans(true) << endl;
        cout << "  I Owe Others   : " << countLoans(false) << endl << endl;
        cout << "  [1] Add New Loan\n  [2] View / Edit Loans\n  [0] Main Menu\n\n>> Choice: ";
        cin >> choice;
        if (choice == 1) addNewLoan();
        else if (choice == 2) editLoan();
    } while (choice != 0);
}

void addNewLoan() {
    system("cls");
    cout << "=== ADD NEW LOAN ===\n\n";
    loanRecord newLoan{};
    tm date = getCurrentDate();

    int choice;
    cout << "  [1] Today\n  [2] Other Date\n  [0] Back\n>> Choice: "; cin >> choice;
    if (choice == 0) return;
    if (choice == 2) {
        cout << ">> Date (DD MM YYYY): "; cin >> date.tm_mday >> date.tm_mon >> date.tm_year;
        date.tm_mon--; date.tm_year -= 1900;
    }
    strftime(newLoan.date, sizeof(newLoan.date), "%d-%m-%Y", &date);

    cout << "\n  [1] Receivable\n  [2] Payable\n>> Choice: "; cin >> choice;
    newLoan.receivable = (choice == 1);
    newLoan.settled = false;

    cin.ignore();
    cout << ">> Friend Name: "; cin.getline(newLoan.name, 50);
    cout << ">> Amount (PKR): "; cin >> newLoan.amount; cin.ignore();
    cout << ">> Note: "; cin.getline(newLoan.remarks, 100);

    fstream loanFile("LoanFile.dat", ios::app | ios::binary);
    loanFile.write(reinterpret_cast<char*>(&newLoan), sizeof(newLoan));
    loanFile.close();

    fstream indexFile("LoanIndexFile.dat", ios::in | ios::out | ios::binary);
    int total;
    indexFile.read(reinterpret_cast<char*>(&total), sizeof(total));
    total++;
    indexFile.seekp(0);
    indexFile.write(reinterpret_cast<char*>(&total), sizeof(total));

    loanIndexRecord idx;
    idx.recordNum = total - 1;
    strcpy_s(idx.date, sizeof(idx.date), newLoan.date);
    idx.receivable = newLoan.receivable;
    indexFile.seekp(0, ios::end);
    indexFile.write(reinterpret_cast<char*>(&idx), sizeof(idx));
    indexFile.close();

    cout << "\n>> Saved.\n"; cin.get();
}

int displayAllLoans(fstream& loanFile, fstream& indexFile) {
    system("cls");
    cout << "=== LOAN LIST ===\n\n";
    int total;
    indexFile.seekg(0);
    indexFile.read(reinterpret_cast<char*>(&total), sizeof(total));
    if (total == 0) { cout << ">> No records.\n"; return 0; }

    loanIndexRecord idx; loanRecord rec;
    cout << left << setw(4) << "No" << setw(12) << "Date" << setw(18) << "Friend"
        << setw(10) << "PKR" << setw(12) << "Type" << setw(10) << "Status" << "Note\n";
    cout << string(80, '-') << endl;

    for (int i = 0; i < total; i++) {
        indexFile.seekg(sizeof(int) + i * sizeof(loanIndexRecord));
        indexFile.read(reinterpret_cast<char*>(&idx), sizeof(idx));
        loanFile.seekg(idx.recordNum * sizeof(loanRecord));
        loanFile.read(reinterpret_cast<char*>(&rec), sizeof(rec));
        cout << setw(4) << i + 1 << setw(12) << rec.date << setw(18) << rec.name
            << setw(10) << rec.amount << setw(12) << (rec.receivable ? "Recievable" : "Payable")
            << setw(10) << (rec.settled ? "Cleared" : "Pending") << rec.remarks << endl;
    }
    return total;
}

void editLoan() {
    fstream loanFile("LoanFile.dat", ios::in | ios::out | ios::binary);
    fstream indexFile("LoanIndexFile.dat", ios::in | ios::binary);
    if (!loanFile || !indexFile) return;

    int total = displayAllLoans(loanFile, indexFile);
    if (total == 0) { cin.ignore(); cin.get(); return; }

    int choice;
    cout << "\n>> Select Loan No. (0 to Back): "; cin >> choice;
    if (choice == 0) return;
    choice--;

    loanIndexRecord idx; loanRecord rec;
    indexFile.seekg(sizeof(int) + choice * sizeof(idx));
    indexFile.read(reinterpret_cast<char*>(&idx), sizeof(idx));
    loanFile.seekg(idx.recordNum * sizeof(rec));
    loanFile.read(reinterpret_cast<char*>(&rec), sizeof(rec));

    int opt = -1;
    while (opt != 0) {
        system("cls");
        cout << "=== EDIT LOAN ===\n";
        cout << "Friend: " << rec.name << " | Amount: " << rec.amount << "\n\n";
        cout << "  [1] Change Amount\n  [2] Change Name\n  [3] Change Note\n  [4] Mark Cleared\n  [0] Save\n>> Choice: ";
        cin >> opt; cin.ignore();

        if (opt == 1) handleAmountEdit(rec);
        else if (opt == 2) { cout << ">> Name: "; cin.getline(rec.name, 50); }
        else if (opt == 3) { cout << ">> Note: "; cin.getline(rec.remarks, 100); }
        else if (opt == 4) completeLoan(rec);
    }

    loanFile.seekp(idx.recordNum * sizeof(rec));
    loanFile.write(reinterpret_cast<char*>(&rec), sizeof(rec));
    loanFile.close(); indexFile.close();
}

void handleAmountEdit(loanRecord& record) {
    int newAmount; cout << ">> New Amount: "; cin >> newAmount;
    int diff = newAmount - record.amount;
    if (diff == 0) return;
    char ans; cout << ">> Record diff (" << diff << ") as transaction? (y/n): "; cin >> ans;
    if (ans == 'y' || ans == 'Y') {
        int absAmount = abs(diff);
        bool isIncome = (record.receivable && diff < 0) || (!record.receivable && diff > 0);
        recordLoanTransaction(isIncome, absAmount, record.date);
    }
    record.amount = newAmount;
}

void completeLoan(loanRecord& record) {
    if (record.amount == 0) { record.settled = true; return; }
    char ans; cout << ">> Add settlement to transactions? (y/n): "; cin >> ans;
    if (ans == 'y' || ans == 'Y') recordLoanTransaction(record.receivable, record.amount, record.date);
    record.amount = 0; record.settled = true;
}

void recordLoanTransaction(bool receivable, int amount, const char* dateStr) {
    tm t{}; parseDate(dateStr, t);
    if (receivable) {
        incomeRecord inc{}; strftime(inc.date, sizeof(inc.date), "%d-%m-%Y", &t); inc.misc = amount;
        fstream("IncomeFile.dat", ios::app | ios::binary).write(reinterpret_cast<char*>(&inc), sizeof(inc));
    }
    else {
        expenseRecord exp{}; strftime(exp.date, sizeof(exp.date), "%d-%m-%Y", &t); exp.misc = amount;
        fstream("ExpenseFile.dat", ios::app | ios::binary).write(reinterpret_cast<char*>(&exp), sizeof(exp));
    }
}