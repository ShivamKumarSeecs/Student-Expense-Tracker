#include <iostream>
#include <ctime>
#include <fstream>
#include <cstring>
#include <cstdlib> // for atoi

// using two files:
/*IndexFile.dat is a binary file that stores the total number of records, the date, week number
and record number (starting from 0) of each record for random access*/
/*ExpenseFIle is a binary file that stores the actual expense reccords*/


//structure for the index binary file records
struct indexRecord {
    int week;
    char date[11];
    int recordNum;
};

//structure for the Expenses file records
struct expenseRecord {
    char date[11];
    int dayOfWeek;

    //expense categories
    int food = 0, transport = 0, entertainment = 0, bills = 0, shopping = 0, rent = 0, misc = 0;
    //categories' amount set to 0 by default
};


//function that returns current date in tm structure data type (see definition of function)
std::tm getCurrentDate();

//function to add new record to the index file (used by addRecordExpenseFile)
void addRecordIndexFile(std::tm date);

//function to add new record to expense file
void addRecordExpenseFile();

//function to get the week number of any given date
int getWeekNum(std::tm date);

//function to get Name of Day when displaying a record (used by displayRecord)
const char* getDayName(int dayNum);

//function to display the entire index file
void displayIndexFile();

//function to display the entire expense file
void displayExpenseFile();

//function to display a given record
void displayRecord(expenseRecord record);

//function for generating weekly report
void weeklyReport(int weekNum);

void report(int startRec, int endRec);

/*function to modify the contents of any given record.logAdded is used to ensure the record is modified
before being written to file*/
void modifyRecord(expenseRecord& record, bool& logAdded);

int main() {
    /*//to intialize an empty IndexFile (use only when manually delete files for some reason)
    int total = -1;
    std::fstream indexFile;
    indexFile.open("IndexFile.dat", std::ios::out | std::ios::binary);
    indexFile.write(reinterpret_cast<char*>(&total), sizeof(total));
    indexFile.close();*/
    displayExpenseFile();
    displayIndexFile();
    return 0;
}

std::tm getCurrentDate() {
    // stroing current time stamp in current time (not human readable)
    std::time_t currentTime = std::time(NULL);

    //converting the timestamp to tm structure that stores month,year,date etc.
    // use portable localtime and copy the result
    std::tm localCurrentTime = *std::localtime(&currentTime);

    return localCurrentTime;
}

int getWeekNum(std::tm date) {
    char weekNum[3]; //length 3 as terminating character also stored

    //getting the week number
    std::strftime(weekNum, sizeof(weekNum), "%W", &date);

    return std::atoi(weekNum); // atoi converts to integer
}

const char* getDayName(int dayNum) {
    const char* dayNames[] = { "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday" };
    return dayNames[dayNum];

}

void addRecordIndexFile(std::tm date) {
    indexRecord record;

    record.week = getWeekNum(date);
    //formating the date to standard dd//mm//yy format and storing it in date part of the record structure
    std::strftime(record.date, sizeof(record.date), "%d-%m-%Y", &date);

    //opening the index file
    std::fstream indexFile;
    indexFile.open("IndexFile.dat", std::ios::in | std::ios::out | std::ios::binary);
    if (indexFile.is_open()) {

        /*reading the total number of records into recordNum and incrementing it to get the record number of
        new record*/
        indexFile.read(reinterpret_cast<char*>(&record.recordNum), sizeof(record.recordNum));
        record.recordNum += 1;

        //writing the new number of records back to the start of the index file (overwriting the old total records
        indexFile.seekp(0, std::ios::beg);
        indexFile.write(reinterpret_cast<char*>(&record.recordNum), sizeof(record.recordNum));

        //writing the new index record into the index file
        indexFile.seekp(0, std::ios::end);
        indexFile.write(reinterpret_cast<char*>(&record), sizeof(record));
        indexFile.close();
    }

    else
        std::cout << "Error\n";
}

void displayIndexFile() {
    int totalRecords;
    indexRecord record;
    std::fstream indexFile;


    indexFile.open("IndexFile.dat", std::ios::in | std::ios::binary);
    if (indexFile.is_open()) {
        //reading the total number records in the file stored which is the first 4 bytes
        indexFile.read(reinterpret_cast<char*>(&totalRecords), sizeof(totalRecords));

        //displaying total number of records in the file
        std::cout << "Total Records: " << totalRecords << std::endl;

        //displaying all the records in the file
        while (indexFile.read(reinterpret_cast<char*>(&record), sizeof(record))) {
            std::cout << "Date: " << record.date << " Week: " << record.week << " Record Number: " << record.recordNum << std::endl;
        }
        indexFile.close();
    }
    else std::cout << "Error\n";
}

void displayExpenseFile() {
    expenseRecord record;
    std::fstream ExpenseFile;

    ExpenseFile.open("ExpenseFile.dat", std::ios::in | std::ios::binary);

    if (ExpenseFile.is_open()) {

        //displaying all the records in the file
        while (ExpenseFile.read(reinterpret_cast<char*>(&record), sizeof(record))) {
            std::cout << "Date: " << record.date << ", "
                << "Day (0-6): " << record.dayOfWeek << ", "
                << "Food: " << record.food << ", "
                << "Transport: " << record.transport << ", "
                << "Entertainment: " << record.entertainment << ", "
                << "Bills: " << record.bills << ", "
                << "Shopping: " << record.shopping << ", "
                << "Rent: " << record.rent << ", "
                << "Misc: " << record.misc
                << std::endl;
        }
        ExpenseFile.close();
    }

    else
        std::cout << "Error\n";
}

void displayRecord(expenseRecord record) {
    char dayName[11];

    std::cout << "Date: " << record.date << ", "
        << getDayName(record.dayOfWeek) << ", "
        << "Food: " << record.food << ", "
        << "Transport: " << record.transport << ", "
        << "Entertainment: " << record.entertainment << ", "
        << "Bills: " << record.bills << ", "
        << "Shopping: " << record.shopping << ", "
        << "Rent: " << record.rent << ", "
        << "Misc: " << record.misc
        << std::endl;
}

void modifyRecord(expenseRecord& record, bool& logAdded) {
    int option = -1, amount;
    //displaying and performing functions of menu
    while (option != 8) {
        std::cout << "Select Option by Entering Option Number\n1)Food\n2)Transport\n3)Entertainment\n4)Bills\n5)Shopping\n6)Rent\n7)Miscellaneous\n8)Exit Menu" << std::endl;
        std::cin >> option;
        if (option != 8) {
            std::cout << "Enter Amount: ";
            std::cin >> amount;
        }

        switch (option) {
        case 1:
            logAdded = true;
            record.food += amount;
            break;
        case 2:
            logAdded = true;
            record.transport += amount;
            break;
        case 3:
            logAdded = true;
            record.entertainment += amount;
            break;
        case 4:
            logAdded = true;
            record.bills += amount;
            break;
        case 5:
            logAdded = true;
            record.shopping += amount;
            break;
        case 6:
            logAdded = true;
            record.rent += amount;
            break;
        case 7:
            logAdded = true;
            record.misc += amount;
            break;
        case 8:
            break;
        default:
            std::cout << "Enter Valid Option\n";
        }
    }

}

void addRecordExpenseFile() {

    //getting the current date
    std::tm date = getCurrentDate();

    expenseRecord record;

    //storing formatted date in record
    std::strftime(record.date, sizeof(record.date), "%d-%m-%Y", &date);

    expenseRecord checkRecord = {};

    std::fstream ExpenseFile;
    //opening and closing in append mode to ensure file created if does not exist
    ExpenseFile.open("ExpenseFile.dat", std::ios::app | std::ios::binary);
    ExpenseFile.close();
    //checking if record for current date is already added, then we'll just modify it

    ExpenseFile.open("ExpenseFile.dat", std::ios::in | std::ios::out | std::ios::binary);

    if (ExpenseFile.is_open()) {
        ExpenseFile.seekg(0, std::ios::end);// to put read pointer at end to see file size
        ExpenseFile.seekp(0, std::ios::end);//to put write pointer at end for append(filemode app givig logical error)

        if (ExpenseFile.tellg() > 0) { //if read pointer is now at the end of file which is not at 0
            ExpenseFile.seekg(-int(sizeof(checkRecord)), std::ios::end);
            ExpenseFile.read(reinterpret_cast<char*>(&checkRecord), sizeof(checkRecord));

        }

        if (strcmp(record.date, checkRecord.date) == 0) { //comparing the two date arrays
            record = checkRecord;
            std::cout << "Today's Record:\n";
            displayRecord(record);
            std::cout << "**Adding New Entry to Today's Record**\n";

            //bringing back the write pointer in file to just before the existing record to overwrite it
            ExpenseFile.seekp(-int(sizeof(record)), std::ios::end);


        }
        else {
            record.dayOfWeek = date.tm_wday;
            addRecordIndexFile(date);
        }
        //if record does not exist, we add the dayOfWeek and create it's index

        bool logAdded = false;
        modifyRecord(record, logAdded);

        //if any values are entered to the new record (logAdded) so the record is written to file
        if (logAdded) {

            if (ExpenseFile.is_open()) {

                ExpenseFile.write(reinterpret_cast<char*>(&record), sizeof(record));


            }
            else
                std::cout << "Error\n";
        }
        ExpenseFile.close();
    }
    else
        std::cout << "Error\n";

}

void weeklyReport(int weekNum) {
    int startRecNum = -1, endRecNum;
    indexRecord record;
    std::fstream indexFile;
    indexFile.open("IndexFile.dat", std::ios::in | std::ios::binary);

    if (indexFile.is_open()) {
        indexFile.seekg(4, std::ios::beg); //to go in front of TotalRecords
        while (indexFile.read(reinterpret_cast<char*>(&record), sizeof(record))) {
            if (record.week == weekNum && startRecNum == -1)
                startRecNum = record.recordNum;
            else if (record.week > weekNum)
                break;
            endRecNum = record.recordNum;
        }
        indexFile.close();


    }
    else
        std::cout << "Error\n";
}

void report(int startRec, int endRec) {
    int Total = 0, foodTot = 0, transTot = 0, entTot = 0, billTot = 0, shopTot = 0, rentTot = 0, miscTot = 0;
    int largestRec = -1, readLength = sizeof(expenseRecord);
    expenseRecord record;
    std::fstream expenseFile;
    expenseFile.open("ExpenseFile.dat", std::ios::in | std::ios::binary);
    if (expenseFile.is_open()) {
        expenseFile.seekg((startRec * readLength), std::ios::beg);
        while (startRec <= endRec) {
            expenseFile.read(reinterpret_cast<char*>(&record), sizeof(record));
            startRec += 1;

        }
        expenseFile.close();
    }
    else std::cout << "Error\n";

}
