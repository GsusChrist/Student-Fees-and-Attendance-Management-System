/*
 * Student Fees & Attendance Management System
 * I used fixed-size arrays (size 100) because vectors were giving me memory errors
 * that I couldn't fix in time. The console formatting for the receipt was also
 * very tedious to line up manually.
 *
 * Future Improvements:
 * - Make the arrays dynamic so they don't crash after 100 students.
 * - Add a real GUI instead of this console text.
 * - Clean up the repeated code in the print functions.
 */

 // 1. SUPPRESSION MACROS
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <iostream>
#include <iomanip>
#include <string>
#include <ctime>
#include <limits>
#include <mysql/jdbc.h>
#include <conio.h>
#include <windows.h>
#include <cmath> 

using namespace std;

// CONSTANTS (Arrays need a fixed size)
// I set this to 100. If we have more students, the program might break.
const int MAX_ITEMS = 100;

// ===================== FUNCTION PROTOTYPES =====================
void setColor(int color);
void gotoxy(int x, int y);
void hideCursor();
int getConsoleWidth();
int getCenterMargin(int contentWidth);
void printCentered(const string& line);
string inputString(const string& prompt, bool isPassword = false);
void drawHeader(const string& title, int color = 11);
void drawSuccess(const string& message);
void drawError(const string& message);

// Menu now takes a raw array and size instead of vector
void drawMenuFrame(const string& title, string options[], int optionCount, int selected);
void drawLoadingScreen(sql::Connection* conn);
void printReceipt(string ref, string date, string sName, string fName, double amount);

sql::Connection* connectDB();
int getStudentID(sql::Connection* conn, string username);
bool selectCourse(sql::Connection* conn, int& outCourseID, string& outCourseName, double& outFee);

void viewAttendance(sql::Connection* conn, int studentID);
void takeAttendance(sql::Connection* conn, int teacherID);

void payFees(sql::Connection* conn, string studentUsername);
void showMyScore(sql::Connection* conn, int studentID);
void showPaymentHistory(sql::Connection* conn, int studentID);

void updateStudent(sql::Connection* conn, string username);
void updateTeacher(sql::Connection* conn, string username);
void deleteUser(sql::Connection* conn);

void listRecords(sql::Connection* conn);

// Analytics Functions
void showAdminStats(sql::Connection* conn);
void showReliabilityScore(sql::Connection* conn);
void showDebtList(sql::Connection* conn);

void addCourse(sql::Connection* conn);
void editCourse(sql::Connection* conn);
void removeCourse(sql::Connection* conn);

void adminMenu(sql::Connection* conn, string username);
void teacherMenu(sql::Connection* conn, string username);
void studentMenu(sql::Connection* conn, string username);
int login(sql::Connection* conn, string role, string& outUser);
void registerUser(sql::Connection* conn);

// ===================== UI FUNCTIONS =====================

void setColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, (WORD)color);
}

void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void hideCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &info);
}

int getConsoleWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

int getCenterMargin(int contentWidth) {
    int consoleWidth = getConsoleWidth();
    int margin = (consoleWidth - contentWidth) / 2;
    if (margin < 0) return 0;
    return margin;
}

void printCentered(const string& line) {
    int consoleWidth = getConsoleWidth();
    int leftMargin = (consoleWidth - (int)line.length()) / 2;
    if (leftMargin < 0) leftMargin = 0;
    for (int i = 0; i < leftMargin; i++) cout << " ";
    cout << line << endl;
}

string inputString(const string& prompt, bool isPassword) {
    int frameWidth = 60;
    int leftMargin = getCenterMargin(frameWidth) + 2;
    for (int i = 0; i < leftMargin; i++) cout << " ";
    cout << prompt;

    string input;
    char ch;
    while ((ch = (char)_getch()) != 13) { // 13 is Enter
        if (ch == 8) { // 8 is Backspace
            if (!input.empty()) {
                input.pop_back();
                cout << "\b \b";
            }
        }
        else if (ch == 27) { return ""; } // 27 is Esc
        else if (ch >= 32 && ch <= 126) {
            input.push_back(ch);
            if (isPassword) cout << '*'; else cout << ch;
        }
    }
    cout << endl;
    return input;
}

void drawHeader(const string& title, int color) {
    int width = 60;
    int borderWidth = width - 4;
    int leftMargin = getCenterMargin(width);

    setColor(color);
    cout << "\n";
    for (int i = 0; i < leftMargin; i++) cout << " ";
    cout << "  " << "\xDA"; for (int i = 0; i < borderWidth; i++) cout << "\xC4"; cout << "\xBF" << endl;

    for (int i = 0; i < leftMargin; i++) cout << " ";
    cout << "  " << "\xB3";
    int padding = (borderWidth - (int)title.length()) / 2;
    for (int i = 0; i < padding; i++) cout << " ";
    setColor(14); cout << title; setColor(color);
    int rightPadding = borderWidth - padding - (int)title.length();
    for (int i = 0; i < rightPadding; i++) cout << " ";
    cout << "\xB3" << endl;

    for (int i = 0; i < leftMargin; i++) cout << " ";
    cout << "  " << "\xC0"; for (int i = 0; i < borderWidth; i++) cout << "\xC4"; cout << "\xD9" << endl;
    setColor(7); cout << "\n";
}

void drawSuccess(const string& message) {
    setColor(10); cout << "\n"; printCentered("[SUCCESS] " + message); cout << "\n"; setColor(7);
}

void drawError(const string& message) {
    setColor(12); cout << "\n"; printCentered("[ERROR] " + message); cout << "\n"; setColor(7);
}

void drawMenuFrame(const string& title, string options[], int optionCount, int selected) {
    gotoxy(0, 0);

    setColor(10); cout << "\n";
    printCentered("============================================================");
    printCentered("       STUDENT FEES AND ATTENDANCE MANAGEMENT SYSTEM        ");
    printCentered("============================================================");
    setColor(7);
    drawHeader(title, 11);

    int boxWidth = 56;
    int leftMargin = getCenterMargin(60);

    for (int i = 0; i < leftMargin; i++) cout << " ";
    cout << "  " << "\xDA"; for (int i = 0; i < boxWidth; i++) cout << "\xC4"; cout << "\xBF" << endl;

    for (int i = 0; i < optionCount; ++i) {
        for (int j = 0; j < leftMargin; j++) cout << " "; cout << "  " << "\xB3"; for (int j = 0; j < boxWidth; j++) cout << " "; cout << "\xB3" << endl;
        for (int j = 0; j < leftMargin; j++) cout << " "; cout << "  " << "\xB3";

        if (i == selected) {
            setColor(14); cout << "  >> " << options[i]; setColor(7);
        }
        else {
            cout << "     " << options[i];
        }
        int padding = boxWidth - (5 + (int)options[i].length());
        for (int j = 0; j < padding; j++) cout << " "; cout << "\xB3" << endl;
    }
    for (int j = 0; j < leftMargin; j++) cout << " "; cout << "  " << "\xB3"; for (int j = 0; j < boxWidth; j++) cout << " "; cout << "\xB3" << endl;
    for (int i = 0; i < leftMargin; i++) cout << " "; cout << "  " << "\xC0"; for (int i = 0; i < boxWidth; i++) cout << "\xC4"; cout << "\xD9" << endl;

    setColor(8); printCentered("[UP/DOWN] Navigate  [ENTER] Select"); setColor(7);
}

void drawLoadingScreen(sql::Connection* conn) {
    system("cls");
    setColor(10); cout << "\n\n\n";
    printCentered("STUDENT FEES AND ATTENDANCE MANAGEMENT SYSTEM");
    setColor(7); cout << "\n\n";

    if (conn && !conn->isClosed()) {
        setColor(10); printCentered("[SUCCESS] Database Connected Successfully!");
    }
    else {
        setColor(12); printCentered("[ERROR] Database Connection Failed!");
    }

    setColor(8); cout << "\n\n";
    printCentered("Press any key to continue...");
    setColor(7);
    (void)_getch();
    system("cls");
}

// I used ASCII characters to draw the box. 
void printReceipt(string ref, string date, string sName, string fName, double amount) {
    system("cls");
    cout << "\n\n";

    int width = 50;
    int margin = getCenterMargin(width);

    // Top Border
    setColor(11);
    printCentered("\xDA\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xBF");

    // Title
    string titleLine = "\xB3                OFFICIAL RECEIPT                \xB3";
    for (int i = 0; i < margin; i++) cout << " "; cout << titleLine << endl;

    // Separator
    for (int i = 0; i < margin; i++) cout << " ";
    cout << "\xC3\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xB4" << endl;

    // Row 1: Ref ID
    for (int i = 0; i < margin; i++) cout << " ";
    cout << "\xB3 ";
    setColor(8); cout << left << setw(14) << "Ref ID:";
    setColor(15); cout << left << setw(32) << ref.substr(0, 32);
    setColor(11); cout << " \xB3" << endl;

    // Row 2: Date
    for (int i = 0; i < margin; i++) cout << " ";
    cout << "\xB3 ";
    setColor(8); cout << left << setw(14) << "Date:";
    setColor(15); cout << left << setw(32) << date.substr(0, 32);
    setColor(11); cout << " \xB3" << endl;

    // Row 3: Student
    for (int i = 0; i < margin; i++) cout << " ";
    cout << "\xB3 ";
    setColor(8); cout << left << setw(14) << "Student:";
    setColor(15); cout << left << setw(32) << sName.substr(0, 32);
    setColor(11); cout << " \xB3" << endl;

    // Row 4: Fee Type
    for (int i = 0; i < margin; i++) cout << " ";
    cout << "\xB3 ";
    setColor(8); cout << left << setw(14) << "Fee Type:";
    setColor(15); cout << left << setw(32) << fName.substr(0, 32);
    setColor(11); cout << " \xB3" << endl;

    // Bottom Separator
    for (int i = 0; i < margin; i++) cout << " ";
    cout << "\xC3\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xB4" << endl;

    // Total Amount
    for (int i = 0; i < margin; i++) cout << " ";
    cout << "\xB3 ";
    setColor(10); cout << left << setw(14) << "AMOUNT PAID:";
    cout << "$ " << left << setw(30) << fixed << setprecision(2) << amount;
    setColor(11); cout << " \xB3" << endl;

    // Bottom Border
    for (int i = 0; i < margin; i++) cout << " ";
    cout << "\xC0\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xD9" << endl;

    setColor(7);
    cout << "\n";
    printCentered("[Press any key to close receipt]");
    (void)_getch();
}

sql::Connection* connectDB() {
    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_driver_instance();
        sql::Connection* conn = driver->connect("tcp://127.0.0.1:3306", "root", "1234");
        conn->setSchema("studentmansys_v2");
        return conn;
    }
    catch (sql::SQLException& e) {
        drawError("Database connection failed: " + string(e.what()));
        (void)_getch();
        exit(1);
    }
}

int getStudentID(sql::Connection* conn, string username) {
    try {
        sql::PreparedStatement* p = conn->prepareStatement("SELECT StudentID FROM STUDENT WHERE Username = ?");
        p->setString(1, username);
        sql::ResultSet* r = p->executeQuery();
        int sid = -1;
        if (r->next()) sid = r->getInt("StudentID");
        delete r; delete p;
        return sid;
    }
    catch (sql::SQLException&) { return -1; }
}

void listRecords(sql::Connection* conn) {
    system("cls");
    while (true) {
        string ops[] = {
            "View All Students", "View All Teachers", "View All Courses", "View All Transactions", "Back"
        };
        int opCount = 5;
        int choice = 0;

        while (true) {
            drawMenuFrame("VIEW RECORDS", ops, opCount, choice);
            char key = (char)_getch();
            if (key == 72) choice = (choice - 1 + opCount) % opCount;
            else if (key == 80) choice = (choice + 1) % opCount;
            else if (key == 13) break;
        }

        if (choice == 4) return;
        system("cls");

        // Transaction History Logic
        if (choice == 3) {
            drawHeader("TRANSACTION HISTORY", 11);
            string query = "SELECT P.TransactionRef, P.Amount, P.PaymentDate, S.StudentName, F.FeeName FROM PAYMENT P JOIN STUDENT S ON P.StudentID = S.StudentID JOIN STUDENT_FEE SF ON P.SFID = SF.SFID JOIN FEE F ON SF.FeeID = F.FeeID ORDER BY P.PaymentDate DESC";

            try {
                sql::Statement* stmt = conn->createStatement();
                sql::ResultSet* r = stmt->executeQuery(query);

                struct TransData { string ref; double amt; string date; string sName; string fName; };
                TransData history[MAX_ITEMS];
                int count = 0;

                cout << left << setw(5) << "#" << setw(20) << "Ref ID" << setw(15) << "Amount" << setw(20) << "Student" << "Date" << endl;
                cout << string(78, '-') << endl;

                int row = 1;
                while (r->next()) {
                    if (count >= MAX_ITEMS) break; // Safety check
                    history[count].ref = r->getString("TransactionRef");
                    history[count].amt = r->getDouble("Amount");
                    history[count].date = r->getString("PaymentDate");
                    history[count].sName = r->getString("StudentName");
                    history[count].fName = r->getString("FeeName");

                    cout << left << setw(5) << row++ << setw(20) << history[count].ref << "$" << setw(14) << fixed << setprecision(2) << history[count].amt << setw(20) << history[count].sName.substr(0, 18) << history[count].date << endl;
                    count++;
                }
                delete r; delete stmt;

                if (count > 0) {
                    cout << string(78, '-') << endl;
                    string input = inputString("   Enter # to VIEW RECEIPT (or ENTER to back): ");
                    if (!input.empty()) {
                        try {
                            int sel = stoi(input);
                            if (sel >= 1 && sel <= count) {
                                TransData t = history[sel - 1];
                                printReceipt(t.ref, t.date, t.sName, t.fName, t.amt);
                            }
                        }
                        catch (...) {}
                    }
                }
            }
            catch (sql::SQLException& e) { drawError(e.what()); (void)_getch(); }
            system("cls");
            continue;
        }

        string query;
        if (choice == 0) {
            drawHeader("LIST OF STUDENTS", 11);
            query = "SELECT StudentID, StudentName, Username, Email FROM STUDENT ORDER BY StudentID ASC";
        }
        else if (choice == 1) {
            drawHeader("LIST OF TEACHERS", 11);
            query = "SELECT TeacherID, TeacherName, Username FROM TEACHER ORDER BY TeacherID ASC";
        }
        else if (choice == 2) {
            drawHeader("LIST OF COURSES", 11);
            query = "SELECT CourseID, CourseName, CreditHours, SemesterFee, (SemesterFee / NULLIF(CreditHours, 0)) as CostPerCredit FROM COURSE ORDER BY CourseID ASC";
        }

        try {
            sql::Statement* stmt = conn->createStatement();
            sql::ResultSet* r = stmt->executeQuery(query);

            if (choice == 0) cout << left << setw(5) << "ID" << setw(30) << "Name" << setw(20) << "Username" << "Email" << endl;
            else if (choice == 1) cout << left << setw(5) << "ID" << setw(30) << "Name" << setw(20) << "Username" << endl;
            else if (choice == 2) cout << left << setw(5) << "ID" << setw(30) << "Course" << setw(10) << "Credits" << setw(12) << "Fee($)" << "Value Index" << endl;

            cout << string(75, '-') << endl;

            bool found = false;
            while (r->next()) {
                found = true;
                if (choice == 0) cout << left << setw(5) << r->getInt("StudentID") << setw(30) << r->getString("StudentName") << setw(20) << r->getString("Username") << r->getString("Email") << endl;
                else if (choice == 1) cout << left << setw(5) << r->getInt("TeacherID") << setw(30) << r->getString("TeacherName") << setw(20) << r->getString("Username") << endl;
                else if (choice == 2) {
                    double fee = r->getDouble("SemesterFee");
                    double cpc = r->getDouble("CostPerCredit");
                    cout << left << setw(5) << r->getInt("CourseID") << setw(30) << r->getString("CourseName") << setw(10) << r->getInt("CreditHours") << "$" << setw(11) << fixed << setprecision(2) << fee << "($" << (int)cpc << "/cr)" << endl;
                }
            }
            if (!found) drawError("No records found.");
            delete r; delete stmt;
        }
        catch (sql::SQLException& e) { drawError(e.what()); }
        cout << "\nPress any key to return..."; (void)_getch();
        system("cls");
    }
}

void showAdminStats(sql::Connection* conn) {
    system("cls"); drawHeader("EXECUTIVE ANALYTICS", 13);

    try {
        sql::Statement* stmt = conn->createStatement();

        cout << "\n   [1] TOTAL FEES AND STUDENT DEBT\n";
        cout << "   " << string(60, '-') << endl;

        // I did these separately because JOINing them all at once returned wrong numbers
        sql::ResultSet* r1 = stmt->executeQuery("SELECT SUM(Amount) AS Total FROM PAYMENT");
        double totalRev = 0.0; if (r1->next()) totalRev = r1->getDouble("Total"); delete r1;

        sql::ResultSet* r2 = stmt->executeQuery("SELECT SUM(AmountDue - AmountPaid) AS Debt FROM STUDENT_FEE");
        double totalDebt = 0.0; if (r2->next()) totalDebt = r2->getDouble("Debt"); delete r2;

        sql::ResultSet* r3 = stmt->executeQuery("SELECT COUNT(*) FROM STUDENT");
        int totalStu = 0; if (r3->next()) totalStu = r3->getInt(1); delete r3;

        cout << "   Total Fees Collected:  "; setColor(10); cout << "$" << fixed << setprecision(2) << totalRev << endl; setColor(7);
        cout << "   Outstanding Debt:      "; setColor(12); cout << "$" << fixed << setprecision(2) << totalDebt << endl; setColor(7);
        cout << "   Total Students:        " << totalStu << endl;

        cout << "\n\n   [2] TOTAL COLLECTED FEES BY COURSE\n";
        cout << "   " << string(60, '-') << endl;

        // This query was hard. If I don't use COALESCE, null values break the code.
        string revQuery = "SELECT C.CourseName, COALESCE(SUM(P.Amount), 0) as Metric FROM COURSE C LEFT JOIN STUDENT_COURSE SC ON C.CourseID = SC.CourseID LEFT JOIN PAYMENT P ON SC.StudentID = P.StudentID GROUP BY C.CourseID, C.CourseName ORDER BY Metric DESC";

        sql::ResultSet* r4 = stmt->executeQuery(revQuery);

        string courseNames[MAX_ITEMS];
        double revenues[MAX_ITEMS];
        int count = 0;
        double maxRev = 0;

        while (r4->next()) {
            if (count >= MAX_ITEMS) break; // prevent crash
            courseNames[count] = r4->getString("CourseName");
            revenues[count] = r4->getDouble("Metric");
            if (revenues[count] > maxRev) maxRev = revenues[count];
            count++;
        }
        delete r4;

        if (count == 0) cout << "   No revenue data available.\n";
        else {
            for (int i = 0; i < count; i++) {
                // Formatting the bar chart was annoying. 
                int barLen = 0;
                if (maxRev > 0) {
                    barLen = (int)((revenues[i] / maxRev) * 30.0);
                }

                cout << "   " << left << setw(20) << courseNames[i] << " |";
                if (revenues[i] > 0) setColor(11); else setColor(8);

                // Draw the blocks
                for (int j = 0; j < barLen; j++) cout << "\xFE";

                setColor(7);
                cout << " $" << (int)revenues[i] << endl;
            }
        }

        cout << "\n\n   [3] ENROLLMENT BY COURSE\n";
        cout << "   " << string(60, '-') << endl;

        string popQuery = "SELECT C.CourseName, COUNT(SC.StudentID) as Metric FROM COURSE C LEFT JOIN STUDENT_COURSE SC ON C.CourseID = SC.CourseID GROUP BY C.CourseID, C.CourseName ORDER BY Metric DESC";

        sql::ResultSet* r5 = stmt->executeQuery(popQuery);

        // REUSE ARRAYS (Reuse memory logic)
        // string courseNames[MAX_ITEMS] - reused
        int enrolls[MAX_ITEMS];
        count = 0;
        int maxPop = 0;

        while (r5->next()) {
            if (count >= MAX_ITEMS) break;
            courseNames[count] = r5->getString("CourseName");
            enrolls[count] = r5->getInt("Metric");
            if (enrolls[count] > maxPop) maxPop = enrolls[count];
            count++;
        }
        delete r5;

        if (count == 0) cout << "   No enrollment data available.\n";
        else {
            for (int i = 0; i < count; i++) {
                int barLen = 0;
                if (maxPop > 0) {
                    barLen = (int)(((double)enrolls[i] / maxPop) * 30.0);
                }
                cout << "   " << left << setw(20) << courseNames[i] << " |";
                if (enrolls[i] > 0) setColor(14); else setColor(8);
                for (int j = 0; j < barLen; j++) cout << "\xFE";
                setColor(7);
                cout << " " << enrolls[i] << " Students" << endl;
            }
        }
        delete stmt;
    }
    catch (sql::SQLException& e) { drawError(e.what()); }
    cout << "\n\nPress any key..."; (void)_getch();
}

void showReliabilityScore(sql::Connection* conn) {
    system("cls"); drawHeader("STUDENT RELIABILITY SCORE (SRS)", 13);
    string query = "SELECT * FROM ( SELECT S.StudentName, (SUM(CASE WHEN A.Status='Present' THEN 1.0 ELSE 0.0 END) / COUNT(A.AttendanceID)) * 100.0 AS AttRate, (SUM(SF.AmountPaid) / SUM(SF.AmountDue)) * 100.0 AS PayRate FROM STUDENT S JOIN ATTENDANCE A ON S.StudentID = A.StudentID JOIN STUDENT_FEE SF ON S.StudentID = SF.StudentID GROUP BY S.StudentID) AS T ORDER BY (AttRate + PayRate) DESC";

    try {
        sql::Statement* stmt = conn->createStatement();
        sql::ResultSet* res = stmt->executeQuery(query);

        cout << "\n   " << left << setw(25) << "Student Name" << setw(12) << "Attend %" << setw(12) << "Fees %" << setw(10) << "Score" << "Grade" << endl;
        cout << "   " << string(70, '-') << endl;

        bool found = false;
        int count = 0;
        double totalScoreSum = 0.0;

        while (res->next()) {
            found = true; count++;
            string name = res->getString("StudentName");
            if (name.length() > 22) name = name.substr(0, 19) + "...";

            double att = res->getDouble("AttRate");
            double pay = res->getDouble("PayRate");
            double score = (att + pay) / 2.0;
            totalScoreSum += score;

            string rating; int color;
            if (score >= 95) { rating = "S (Elite)"; color = 11; }
            else if (score >= 85) { rating = "A (Good)";  color = 10; }
            else if (score >= 70) { rating = "B (Avg)";   color = 14; }
            else if (score >= 50) { rating = "C (Risk)";  color = 12; }
            else { rating = "F (Fail)";  color = 4; }

            cout << "   " << left << setw(25) << name << fixed << setprecision(0) << att << "%" << setw(5) << " " << fixed << setprecision(0) << pay << "%" << setw(5) << " " << fixed << setprecision(1) << score;
            cout << "       "; setColor(color); cout << rating << endl; setColor(7);
        }
        delete res; delete stmt;

        if (!found) {
            drawError("No data found (Need Attendance + Fees).");
        }
        else {
            double globalAvg = (count > 0) ? (totalScoreSum / count) : 0.0;
            cout << "   " << string(70, '-') << endl;
            cout << "   University Average Score: ";
            if (globalAvg >= 80) setColor(10); else if (globalAvg >= 60) setColor(14); else setColor(12);
            cout << fixed << setprecision(1) << globalAvg << "/100" << endl; setColor(7);
            cout << "\n   Total Students Ranked: " << count << endl;
        }
    }
    catch (sql::SQLException& e) { drawError(e.what()); }
    cout << "\n\nPress any key..."; (void)_getch();
}

void showDebtList(sql::Connection* conn) {
    system("cls"); drawHeader("STUDENTS WITH UNPAID FEES", 12);
    string query = "SELECT S.StudentID, S.StudentName, (SELECT SUM(AmountDue - AmountPaid) FROM STUDENT_FEE WHERE StudentID = S.StudentID AND Status <> 'Paid') AS TotalDebt FROM STUDENT S WHERE S.StudentID IN (SELECT DISTINCT StudentID FROM STUDENT_FEE WHERE Status <> 'Paid') ORDER BY TotalDebt DESC";

    try {
        sql::Statement* stmt = conn->createStatement();
        sql::ResultSet* res = stmt->executeQuery(query);

        cout << "   [FILTER] Showing total outstanding debt per student.\n\n";
        cout << "   " << left << setw(5) << "ID" << setw(30) << "Student Name" << right << setw(15) << "Total Debt ($)" << endl;
        cout << "   " << string(60, '-') << endl;

        bool found = false;
        double grandTotal = 0.0;
        while (res->next()) {
            found = true;
            double debt = res->getDouble("TotalDebt");
            grandTotal += debt;
            cout << "   " << left << setw(5) << res->getInt("StudentID") << setw(30) << res->getString("StudentName") << right << setw(15) << fixed << setprecision(2) << debt << endl;
        }

        if (!found) drawSuccess("Amazing! No students owe any fees.");
        else {
            cout << "   " << string(60, '-') << endl;
            setColor(12);
            cout << "   " << left << setw(35) << "TOTAL OUTSTANDING:" << right << setw(15) << fixed << setprecision(2) << grandTotal << endl;
            setColor(7);
        }
        delete res; delete stmt;
    }
    catch (sql::SQLException& e) { drawError(e.what()); }
    cout << "\nPress any key..."; (void)_getch();
}

void addCourse(sql::Connection* conn) {
    system("cls"); drawHeader("CREATE NEW COURSE", 13);
    string name = inputString("Course Name (e.g. Cyber Security B): ");
    string credits = inputString("Credit Hours: ");
    string feeStr = inputString("Semester Fee ($): ");
    try {
        conn->setAutoCommit(false);
        sql::PreparedStatement* p = conn->prepareStatement("INSERT INTO COURSE (CourseName, CreditHours, SemesterFee) VALUES (?, ?, ?)");
        p->setString(1, name); p->setInt(2, stoi(credits)); p->setDouble(3, stod(feeStr)); p->executeUpdate(); delete p;
        sql::PreparedStatement* f = conn->prepareStatement("INSERT INTO FEE (FeeName, Amount, IsTuition) VALUES (?, ?, 1)");
        f->setString(1, "Tuition: " + name); f->setDouble(2, stod(feeStr)); f->executeUpdate(); delete f;
        conn->commit(); drawSuccess("Course & Tuition Fee Created Successfully!");
    }
    catch (sql::SQLException& e) { conn->rollback(); drawError("Failed: " + string(e.what())); }
    conn->setAutoCommit(true); (void)_getch();
}

void editCourse(sql::Connection* conn) {
    system("cls"); drawHeader("EDIT COURSE", 13);
    int cid = 0; string oldName; double oldFee;
    if (!selectCourse(conn, cid, oldName, oldFee)) return;

    cout << "\n--- Editing " << oldName << " ---\n";
    cout << "(Leave blank to keep current value)\n";

    string newName = inputString("New Name: ");
    string newCred = inputString("New Credit Hours: ");
    string newFeeStr = inputString("New Fee Amount: ");

    try {
        conn->setAutoCommit(false);
        string updateQ = "UPDATE COURSE SET ";
        bool comma = false;
        if (!newName.empty()) { updateQ += "CourseName = '" + newName + "'"; comma = true; }
        if (!newCred.empty()) { if (comma) updateQ += ", "; updateQ += "CreditHours = " + newCred; comma = true; }
        if (!newFeeStr.empty()) { if (comma) updateQ += ", "; updateQ += "SemesterFee = " + newFeeStr; }
        updateQ += " WHERE CourseID = " + to_string(cid);

        if (comma || !newFeeStr.empty()) {
            sql::Statement* s = conn->createStatement(); s->executeUpdate(updateQ); delete s;
        }

        if (!newName.empty() || !newFeeStr.empty()) {
            string targetFeeName = "Tuition: " + oldName;
            string feeUpdateQ = "UPDATE FEE SET ";
            bool fComma = false;
            if (!newName.empty()) { feeUpdateQ += "FeeName = 'Tuition: " + newName + "'"; fComma = true; }
            if (!newFeeStr.empty()) { if (fComma) feeUpdateQ += ", "; feeUpdateQ += "Amount = " + newFeeStr; }
            feeUpdateQ += " WHERE FeeName = '" + targetFeeName + "'";
            sql::Statement* s2 = conn->createStatement(); s2->executeUpdate(feeUpdateQ); delete s2;
        }
        conn->commit(); drawSuccess("Course & Linked Fees Updated Successfully!");
    }
    catch (sql::SQLException& e) { conn->rollback(); drawError("Update Failed: " + string(e.what())); }
    conn->setAutoCommit(true); (void)_getch();
}

void removeCourse(sql::Connection* conn) {
    system("cls"); drawHeader("DELETE COURSE", 12);
    int dummyID; string dummyName; double dummyFee;
    if (!selectCourse(conn, dummyID, dummyName, dummyFee)) return;
    if (inputString("\nType CONFIRM to delete this course: ") == "CONFIRM") {
        try {
            sql::PreparedStatement* p = conn->prepareStatement("DELETE FROM COURSE WHERE CourseID=?");
            p->setInt(1, dummyID); p->executeUpdate(); delete p; drawSuccess("Course Deleted.");
        }
        catch (sql::SQLException& e) { drawError(e.what()); }
    }
    (void)_getch();
}

bool selectCourse(sql::Connection* conn, int& outCourseID, string& outCourseName, double& outFee) {
    system("cls"); drawHeader("SELECT COURSE", 11);
    try {
        sql::Statement* stmt = conn->createStatement();
        string query = "SELECT C.CourseID, C.CourseName, C.SemesterFee, T.TeacherName FROM COURSE C LEFT JOIN TEACHER T ON C.Lecturer_ID = T.TeacherID ORDER BY C.CourseID ASC";
        sql::ResultSet* res = stmt->executeQuery(query);

        int cIds[MAX_ITEMS];
        string cNames[MAX_ITEMS];
        double cFees[MAX_ITEMS];
        int count = 0;

        cout << "\n   " << left << setw(5) << "ID" << setw(30) << "Course Name" << setw(25) << "Current Lecturer" << "Fee($)" << endl;
        cout << "   " << string(75, '-') << endl;

        while (res->next()) {
            if (count >= MAX_ITEMS) break;
            int id = res->getInt("CourseID");
            string name = res->getString("CourseName");
            double fee = res->getDouble("SemesterFee");
            string teacher = res->getString("TeacherName");
            if (teacher.empty()) teacher = "[OPEN]";

            cIds[count] = id;
            cNames[count] = name;
            cFees[count] = fee;

            cout << "   " << left << setw(5) << id << setw(30) << name;
            if (teacher == "[OPEN]") setColor(10); else setColor(7);
            cout << setw(25) << teacher; setColor(7);
            cout << "$" << fee << endl;
            count++;
        }
        delete res; delete stmt;

        if (count == 0) { drawError("No courses found."); return false; }

        string idStr = inputString("\nEnter Course ID: ");
        if (idStr.empty()) return false;
        int inputID = stoi(idStr);

        for (int i = 0; i < count; i++) {
            if (cIds[i] == inputID) {
                outCourseID = cIds[i];
                outCourseName = cNames[i];
                outFee = cFees[i];
                return true;
            }
        }
        drawError("Invalid Course ID."); return false;
    }
    catch (...) { drawError("Invalid input."); return false; }
}

void viewAttendance(sql::Connection* conn, int studentID) {
    system("cls"); drawHeader("MY ATTENDANCE RECORD", 11);
    try {
        string q = "SELECT A.AttendanceDate, A.Status, C.CourseName FROM ATTENDANCE A JOIN COURSE C ON A.CourseID = C.CourseID WHERE A.StudentID=? ORDER BY A.AttendanceDate DESC";
        sql::PreparedStatement* p = conn->prepareStatement(q);
        p->setInt(1, studentID);
        sql::ResultSet* r = p->executeQuery();

        int pCount = 0, aCount = 0;
        cout << left << setw(15) << "Date" << setw(10) << "Status" << "Course" << endl;
        cout << string(60, '-') << endl;
        while (r->next()) {
            string s = r->getString("Status");
            cout << left << setw(15) << r->getString("AttendanceDate") << setw(10) << s << r->getString("CourseName") << endl;
            if (s == "Present") pCount++; else aCount++;
        }
        cout << "\nSummary: Present: " << pCount << " | Absent/Late: " << aCount << endl;
        delete r; delete p;
    }
    catch (...) { drawError("Error retrieving attendance."); }
    cout << "\nPress any key..."; (void)_getch();
}

void takeAttendance(sql::Connection* conn, int teacherID) {
    int courseID = -1; string courseName = "";
    try {
        sql::PreparedStatement* p = conn->prepareStatement("SELECT CourseID, CourseName FROM COURSE WHERE Lecturer_ID = ?");
        p->setInt(1, teacherID); sql::ResultSet* r = p->executeQuery();

        int cIds[MAX_ITEMS];
        string cNames[MAX_ITEMS];
        int count = 0;

        while (r->next()) {
            if (count >= MAX_ITEMS) break;
            cIds[count] = r->getInt("CourseID");
            cNames[count] = r->getString("CourseName");
            count++;
        }
        delete r; delete p;

        if (count == 0) { drawError("No courses assigned to you."); (void)_getch(); return; }
        courseID = cIds[0]; courseName = cNames[0];
    }
    catch (sql::SQLException& e) { drawError(e.what()); return; }

    struct StudentAtt { int id; string name; string status; };
    StudentAtt students[MAX_ITEMS];
    int sCount = 0;

    try {
        sql::PreparedStatement* p = conn->prepareStatement("SELECT S.StudentID, S.StudentName FROM STUDENT S JOIN STUDENT_COURSE SC ON S.StudentID = SC.StudentID WHERE SC.CourseID = ?");
        p->setInt(1, courseID); sql::ResultSet* r = p->executeQuery();

        sql::PreparedStatement* check = conn->prepareStatement("SELECT Status FROM ATTENDANCE WHERE StudentID=? AND CourseID=? AND DATE(AttendanceDate)=CURDATE()");

        while (r->next()) {
            if (sCount >= MAX_ITEMS) break;
            int sid = r->getInt("StudentID");
            string sName = r->getString("StudentName");
            string currentStatus = "Present";
            check->setInt(1, sid);
            check->setInt(2, courseID);
            sql::ResultSet* rsCheck = check->executeQuery();
            if (rsCheck->next()) {
                currentStatus = rsCheck->getString("Status");
            }
            delete rsCheck;

            students[sCount].id = sid;
            students[sCount].name = sName;
            students[sCount].status = currentStatus;
            sCount++;
        }
        delete check; delete r; delete p;
    }
    catch (...) { return; }

    if (sCount == 0) { drawError("No students enrolled."); (void)_getch(); return; }

    int selectedIdx = 0;
    time_t t = time(0); struct tm* now = localtime(&t); char buf[80]; strftime(buf, sizeof(buf), "%Y-%m-%d", now); string todayStr = string(buf);

    system("cls");
    while (true) {
        gotoxy(0, 0);
        drawHeader("ROLL CALL: " + courseName, 11);
        setColor(14); printCentered("Date: " + todayStr); setColor(7); cout << "\n";
        cout << "   " << left << setw(5) << "No." << setw(30) << "Name" << "Status" << endl;
        cout << "   " << string(50, '-') << endl;

        for (int i = 0; i < sCount; ++i) {
            cout << "   ";
            if (i == selectedIdx) setColor(14); else setColor(7);
            cout << left << setw(5) << (i + 1) << setw(30) << students[i].name << setw(10) << students[i].status << endl;
        }
        setColor(7);
        cout << "\n   [UP/DOWN] Move  [ENTER] Toggle Status  [S] Save  [ESC] Cancel\n";

        char k = (char)_getch();
        if (k == 72) selectedIdx = (selectedIdx - 1 + sCount) % sCount;
        else if (k == 80) selectedIdx = (selectedIdx + 1) % sCount;
        else if (k == 13) {
            if (students[selectedIdx].status == "Present") students[selectedIdx].status = "Absent";
            else if (students[selectedIdx].status == "Absent") students[selectedIdx].status = "Late";
            else students[selectedIdx].status = "Present";
        }
        else if (k == 's' || k == 'S') {
            try {
                sql::PreparedStatement* upd = conn->prepareStatement("UPDATE ATTENDANCE SET Status=? WHERE StudentID=? AND CourseID=? AND DATE(AttendanceDate)=CURDATE()");
                sql::PreparedStatement* ins = conn->prepareStatement("INSERT INTO ATTENDANCE (StudentID, CourseID, AttendanceDate, Status) VALUES (?, ?, CURDATE(), ?)");

                for (int i = 0; i < sCount; i++) {
                    upd->setString(1, students[i].status);
                    upd->setInt(2, students[i].id);
                    upd->setInt(3, courseID);
                    int rows = upd->executeUpdate();
                    if (rows == 0) {
                        ins->setInt(1, students[i].id);
                        ins->setInt(2, courseID);
                        ins->setString(3, students[i].status);
                        ins->executeUpdate();
                    }
                }
                delete upd; delete ins;
                drawSuccess("Attendance Saved (Updated).");
            }
            catch (sql::SQLException& e) { drawError(e.what()); }
            (void)_getch(); return;
        }
        else if (k == 27) return;
    }
}

void payFees(sql::Connection* conn, string studentUsername) {
    system("cls"); drawHeader("PAY SCHOOL FEES", 11);
    int sid = getStudentID(conn, studentUsername);
    if (sid == -1) return;

    try {
        // Only show fees that are NOT 'Paid' yet
        sql::PreparedStatement* p = conn->prepareStatement("SELECT SF.SFID, F.FeeName, SF.AmountDue, SF.AmountPaid FROM STUDENT_FEE SF JOIN FEE F ON SF.FeeID=F.FeeID WHERE SF.StudentID=? AND SF.Status<>'Paid'");
        p->setInt(1, sid); sql::ResultSet* r = p->executeQuery();

        int ids[MAX_ITEMS];
        double dues[MAX_ITEMS];
        double paids[MAX_ITEMS];
        int count = 0;

        int idx = 1;
        while (r->next()) {
            if (count >= MAX_ITEMS) break;
            double total = r->getDouble("AmountDue");
            double paid = r->getDouble("AmountPaid");

            // Calculate what is left to pay
            cout << idx++ << ". " << r->getString("FeeName") << " | Owe: $" << fixed << setprecision(2) << (total - paid) << endl;

            ids[count] = r->getInt("SFID");
            dues[count] = total;
            paids[count] = paid;
            count++;
        }
        delete r; delete p;

        if (count == 0) { drawSuccess("No fees due!"); (void)_getch(); return; }

        string selStr = inputString("\nSelect # to pay: ");
        if (selStr.empty()) return;
        int sel = stoi(selStr);
        if (sel < 1 || sel > count) return;

        int i = sel - 1;
        double remaining = dues[i] - paids[i];

        string amtStr = inputString("Enter Amount: ");
        if (amtStr.empty()) return;
        double payAmt = stod(amtStr);

        // Validation: Don't let them pay more than they owe
        if (payAmt > remaining) {
            // drawError("Amount too high, adjusting to max."); 
            // Actually just fix it automatically
            payAmt = remaining;
        }

        conn->setAutoCommit(false); // Start transaction

        // Use time() to make a fake transaction ID
        string tref = "PAY-" + to_string(time(0));

        sql::PreparedStatement* ins = conn->prepareStatement("INSERT INTO PAYMENT (StudentID, SFID, Amount, TransactionRef) VALUES (?, ?, ?, ?)");
        ins->setInt(1, sid);
        ins->setInt(2, ids[i]);
        ins->setDouble(3, payAmt);
        ins->setString(4, tref);
        ins->executeUpdate();
        delete ins;

        // Update the main table
        double newPaid = paids[i] + payAmt;
        string status = (newPaid >= dues[i]) ? "Paid" : "Partial";

        sql::PreparedStatement* upd = conn->prepareStatement("UPDATE STUDENT_FEE SET AmountPaid=?, Status=? WHERE SFID=?");
        upd->setDouble(1, newPaid);
        upd->setString(2, status);
        upd->setInt(3, ids[i]);
        upd->executeUpdate();
        delete upd;

        conn->commit();
        conn->setAutoCommit(true);
        drawSuccess("Payment Successful! Ref: " + tref);

        string ask = inputString("   View Receipt? (Y/N): ");
        if (ask == "Y" || ask == "y") {
            // Need to fetch names again for the receipt
            string sName = "", fName = "";
            sql::PreparedStatement* q = conn->prepareStatement("SELECT S.StudentName, F.FeeName FROM STUDENT S, FEE F WHERE S.StudentID=? AND F.FeeID=(SELECT FeeID FROM STUDENT_FEE WHERE SFID=?)");
            q->setInt(1, sid); q->setInt(2, ids[i]);
            sql::ResultSet* qr = q->executeQuery();
            if (qr->next()) {
                sName = qr->getString(1);
                fName = qr->getString(2);
            }
            delete qr; delete q;

            printReceipt(tref, "Now", sName, fName, payAmt);
        }
    }
    catch (sql::SQLException& e) {
        conn->rollback();
        drawError(e.what());
    }
    (void)_getch();
}

void showPaymentHistory(sql::Connection* conn, int studentID) {
    system("cls"); drawHeader("MY PAYMENT HISTORY", 11);

    string query = "SELECT P.TransactionRef, P.Amount, P.PaymentDate, F.FeeName, S.StudentName FROM PAYMENT P JOIN STUDENT_FEE SF ON P.SFID = SF.SFID JOIN FEE F ON SF.FeeID = F.FeeID JOIN STUDENT S ON P.StudentID = S.StudentID WHERE P.StudentID = ? ORDER BY P.PaymentDate DESC";

    try {
        sql::PreparedStatement* p = conn->prepareStatement(query);
        p->setInt(1, studentID);
        sql::ResultSet* r = p->executeQuery();

        struct ReceiptData { string ref; double amt; string date; string sName; string fName; };
        ReceiptData history[MAX_ITEMS];
        int count = 0;

        cout << left << setw(5) << "#" << setw(20) << "Ref ID" << setw(15) << "Amount" << "Date" << endl;
        cout << string(60, '-') << endl;

        int row = 1;
        while (r->next()) {
            if (count >= MAX_ITEMS) break;
            history[count].ref = r->getString("TransactionRef");
            history[count].amt = r->getDouble("Amount");
            history[count].date = r->getString("PaymentDate");
            history[count].fName = r->getString("FeeName");
            history[count].sName = r->getString("StudentName");

            cout << left << setw(5) << row++ << setw(20) << history[count].ref << "$" << setw(14) << fixed << setprecision(2) << history[count].amt << history[count].date << endl;
            count++;
        }
        delete r; delete p;

        if (count == 0) {
            drawError("No payment history found.");
            (void)_getch(); return;
        }

        string input = inputString("\nEnter # to view receipt (or ENTER to back): ");
        if (!input.empty()) {
            try {
                int idx = stoi(input);
                if (idx >= 1 && idx <= count) {
                    ReceiptData t = history[idx - 1];
                    printReceipt(t.ref, t.date, t.sName, t.fName, t.amt);
                }
            }
            catch (...) {}
        }
    }
    catch (sql::SQLException& e) { drawError(e.what()); (void)_getch(); }
}

void showMyScore(sql::Connection* conn, int studentID) {
    system("cls"); drawHeader("MY PERFORMANCE REPORT", 11);
    string query = "SELECT * FROM ( SELECT S.StudentID, S.StudentName, (SUM(CASE WHEN A.Status='Present' THEN 1.0 ELSE 0.0 END) / COUNT(A.AttendanceID)) * 100.0 AS AttRate, (SUM(SF.AmountPaid) / SUM(SF.AmountDue)) * 100.0 AS PayRate FROM STUDENT S JOIN ATTENDANCE A ON S.StudentID = A.StudentID JOIN STUDENT_FEE SF ON S.StudentID = SF.StudentID GROUP BY S.StudentID) AS T WHERE StudentID = ?";

    try {
        sql::PreparedStatement* p = conn->prepareStatement(query);
        p->setInt(1, studentID);
        sql::ResultSet* res = p->executeQuery();

        if (res->next()) {
            string name = res->getString("StudentName");
            double att = res->getDouble("AttRate");
            double pay = res->getDouble("PayRate");
            double score = (att + pay) / 2.0;

            string rating; int color;
            if (score >= 95) { rating = "S (Elite)"; color = 11; }
            else if (score >= 85) { rating = "A (Good)";  color = 10; }
            else if (score >= 70) { rating = "B (Avg)";   color = 14; }
            else if (score >= 50) { rating = "C (Risk)";  color = 12; }
            else { rating = "F (Fail)";  color = 4; }

            cout << "\n   " << left << setw(20) << "Student Name:" << name << endl;
            cout << "   " << string(40, '-') << endl;
            cout << "   " << left << setw(20) << "Attendance Rate:";
            if (att >= 80) setColor(10); else setColor(12);
            cout << fixed << setprecision(0) << att << "%" << endl; setColor(7);
            cout << "   " << left << setw(20) << "Fee Payment Rate:";
            if (pay >= 80) setColor(10); else setColor(12);
            cout << fixed << setprecision(0) << pay << "%" << endl; setColor(7);
            cout << "   " << string(40, '-') << endl;
            cout << "   " << left << setw(20) << "OVERALL SCORE:";
            setColor(color); cout << fixed << setprecision(1) << score << " / 100" << endl; setColor(7);
            cout << "   " << left << setw(20) << "RATING:";
            setColor(color); cout << rating << endl; setColor(7);
            cout << "   " << string(40, '-') << endl;
            if (score < 70) cout << "\n   [TIP] To improve, attend more classes or clear outstanding fees.";
            else cout << "\n   [INFO] Keep up the great work!";
        }
        else {
            drawError("Not enough data to calculate your score yet.");
            cout << "   (You need at least 1 attendance record and 1 fee record)";
        }
        delete res; delete p;
    }
    catch (sql::SQLException& e) { drawError(e.what()); }
    cout << "\n\nPress any key..."; (void)_getch();
}

void updateStudent(sql::Connection* conn, string username) {
    system("cls"); drawHeader("UPDATE PROFILE", 13);
    string newName = inputString("New Name: ");
    string newPass = inputString("New Password: ");
    string query = "UPDATE STUDENT SET "; bool first = true;
    if (!newName.empty()) { query += "StudentName='" + newName + "'"; first = false; }
    if (!newPass.empty()) { if (!first) query += ", "; query += "Password='" + newPass + "'"; first = false; }
    query += " WHERE Username='" + username + "'";
    try { if (!first) { sql::Statement* s = conn->createStatement(); s->executeUpdate(query); delete s; drawSuccess("Updated."); } }
    catch (...) { drawError("Fail."); }
    (void)_getch();
}

void updateTeacher(sql::Connection* conn, string username) {
    system("cls"); drawHeader("UPDATE PROFILE", 13);
    string newName = inputString("New Name: ");
    string newPass = inputString("New Password: ");
    string query = "UPDATE TEACHER SET "; bool first = true;
    if (!newName.empty()) { query += "TeacherName='" + newName + "'"; first = false; }
    if (!newPass.empty()) { if (!first) query += ", "; query += "Password='" + newPass + "'"; first = false; }
    query += " WHERE Username='" + username + "'";
    try { if (!first) { sql::Statement* s = conn->createStatement(); s->executeUpdate(query); delete s; drawSuccess("Updated."); } }
    catch (...) { drawError("Fail."); }
    (void)_getch();
}

void deleteUser(sql::Connection* conn) {
    system("cls"); drawHeader("DELETE ACCOUNT", 12);
    string target = inputString("Username to DELETE: ");
    string type = inputString("Type (Teacher/Student): ");
    if (inputString("Type CONFIRM: ") != "CONFIRM") return;
    try {
        string t = (type == "Teacher") ? "TEACHER" : "STUDENT";
        sql::PreparedStatement* p = conn->prepareStatement("DELETE FROM " + t + " WHERE Username=?");
        p->setString(1, target); int r = p->executeUpdate(); delete p;
        if (r > 0) drawSuccess("Deleted."); else drawError("Not found.");
    }
    catch (...) { drawError("Fail."); }
    (void)_getch();
}

void adminMenu(sql::Connection* conn, string username) {
    string ops[] = {
        "Register Account", "View Database Records", "Delete Account",
        "Manage Courses", "Analytics Dashboard", "Logout"
    };
    int opCount = 6;
    int choice = 0;
    system("cls");

    while (true) {
        while (true) {
            drawMenuFrame("ADMIN DASHBOARD", ops, opCount, choice);
            char key = (char)_getch();
            if (key == 72) choice = (choice - 1 + opCount) % opCount;
            else if (key == 80) choice = (choice + 1) % opCount;
            else if (key == 13) break;
        }

        if (choice == 0) registerUser(conn);
        else if (choice == 1) listRecords(conn);
        else if (choice == 2) deleteUser(conn);
        else if (choice == 3) {
            system("cls");
            while (true) {
                string cops[] = { "Add New Course", "Edit Course", "Delete Course", "Back" };
                int cCount = 4;
                int cch = 0;
                while (true) {
                    drawMenuFrame("MANAGE COURSES", cops, cCount, cch);
                    char k = (char)_getch();
                    if (k == 72) cch = (cch - 1 + cCount) % cCount;
                    if (k == 80) cch = (cch + 1) % cCount;
                    if (k == 13) break;
                }
                if (cch == 3) break;
                if (cch == 0) addCourse(conn);
                if (cch == 1) editCourse(conn);
                if (cch == 2) removeCourse(conn);
                system("cls");
            }
            system("cls");
        }
        else if (choice == 4) {
            system("cls");
            while (true) {
                string aops[] = {
                    "General Reports", "Student Reliability Score", "Unpaid Fees List", "Back"
                };
                int aCount = 4;
                int ach = 0;
                while (true) {
                    drawMenuFrame("ANALYTICS", aops, aCount, ach);
                    char k = (char)_getch();
                    if (k == 72) ach = (ach - 1 + aCount) % aCount;
                    if (k == 80) ach = (ach + 1) % aCount;
                    if (k == 13) break;
                }
                if (ach == 3) break;
                if (ach == 0) showAdminStats(conn);
                if (ach == 1) showReliabilityScore(conn);
                if (ach == 2) showDebtList(conn);
                system("cls");
            }
            system("cls");
        }
        else if (choice == 5) break;
        system("cls");
    }
}

void teacherMenu(sql::Connection* conn, string username) {
    string ops[] = { "Take Attendance", "Update Profile", "Logout" };
    int opCount = 3;
    int choice = 0;
    int tid = -1;
    try {
        sql::PreparedStatement* p = conn->prepareStatement("SELECT TeacherID FROM TEACHER WHERE Username=?");
        p->setString(1, username); sql::ResultSet* r = p->executeQuery();
        if (r->next()) tid = r->getInt("TeacherID"); delete r; delete p;
    }
    catch (...) {}

    system("cls");
    while (true) {
        while (true) {
            drawMenuFrame("TEACHER DASHBOARD", ops, opCount, choice);
            char key = (char)_getch();
            if (key == 72) choice = (choice - 1 + opCount) % opCount;
            else if (key == 80) choice = (choice + 1) % opCount;
            else if (key == 13) break;
        }
        if (choice == 0) takeAttendance(conn, tid);
        else if (choice == 1) updateTeacher(conn, username);
        else if (choice == 2) break;
        system("cls");
    }
}

void studentMenu(sql::Connection* conn, string username) {
    string ops[] = {
        "My Attendance", "Pay Fees", "Payment History",
        "My Reliability Score", "Update Profile", "Logout"
    };
    int opCount = 6;
    int choice = 0;
    int sid = getStudentID(conn, username);

    system("cls");
    while (true) {
        while (true) {
            drawMenuFrame("STUDENT DASHBOARD", ops, opCount, choice);
            char key = (char)_getch();
            if (key == 72) choice = (choice - 1 + opCount) % opCount;
            else if (key == 80) choice = (choice + 1) % opCount;
            else if (key == 13) break;
        }
        if (choice == 0) viewAttendance(conn, sid);
        else if (choice == 1) payFees(conn, username);
        else if (choice == 2) showPaymentHistory(conn, sid);
        else if (choice == 3) showMyScore(conn, sid);
        else if (choice == 4) updateStudent(conn, username);
        else if (choice == 5) break;
        system("cls");
    }
}

int login(sql::Connection* conn, string role, string& outUser) {
    system("cls"); drawHeader(role + " LOGIN", 11);
    string u = inputString("Username: ");
    string p = inputString("Password: ", true);
    string q;
    if (role == "Admin") { if (u == "admin" && p == "admin") { outUser = u; return 1; } return -1; }
    else if (role == "Teacher") q = "SELECT * FROM TEACHER WHERE Username=?";
    else q = "SELECT * FROM STUDENT WHERE Username=?";
    try {
        sql::PreparedStatement* ps = conn->prepareStatement(q);
        ps->setString(1, u); sql::ResultSet* r = ps->executeQuery();
        if (r->next()) { if (r->getString("Password") == p) { outUser = u; delete r; delete ps; return 1; } }
        delete r; delete ps;
    }
    catch (...) {}
    drawError("Invalid Login"); (void)_getch(); return -1;
}

void registerUser(sql::Connection* conn) {
    string ops[] = { "Register Teacher", "Register Student", "Back" };
    int opCount = 3;
    int choice = 0;
    system("cls");
    while (true) {
        while (true) {
            drawMenuFrame("REGISTER", ops, opCount, choice);
            char key = (char)_getch();
            if (key == 72) choice = (choice - 1 + opCount) % opCount;
            else if (key == 80) choice = (choice + 1) % opCount;
            else if (key == 13) break;
        }
        if (choice == 2) return;
        system("cls"); drawHeader("REGISTRATION", 13);
        string name = inputString("Full Name: ");
        string user = inputString("Username: ");
        string pass = inputString("Password: ", true);

        if (choice == 0) {
            int cid = 0; string cname; double dummy;
            cout << "\nSelect Course to Assign:\n";
            if (!selectCourse(conn, cid, cname, dummy)) { (void)_getch(); system("cls"); continue; }
            try {
                conn->setAutoCommit(false);
                sql::PreparedStatement* p = conn->prepareStatement("INSERT INTO TEACHER (TeacherName, Username, Password) VALUES (?,?,?)");
                p->setString(1, name); p->setString(2, user); p->setString(3, pass); p->executeUpdate(); delete p;
                int tid = -1;
                sql::PreparedStatement* gp = conn->prepareStatement("SELECT TeacherID FROM TEACHER WHERE Username=?");
                gp->setString(1, user); sql::ResultSet* gr = gp->executeQuery(); if (gr->next()) tid = gr->getInt(1); delete gr; delete gp;
                if (tid != -1) {
                    sql::PreparedStatement* up = conn->prepareStatement("UPDATE COURSE SET Lecturer_ID=? WHERE CourseID=?");
                    up->setInt(1, tid); up->setInt(2, cid); up->executeUpdate(); delete up;
                }
                conn->commit(); drawSuccess("Teacher Registered & Assigned to " + cname);
            }
            catch (...) { conn->rollback(); drawError("Registration Fail."); }
            conn->setAutoCommit(true);
        }
        else {
            int cid = 0; string cname; double dummy;
            cout << "\nSelect Course for Enrollment:\n";
            if (!selectCourse(conn, cid, cname, dummy)) { (void)_getch(); system("cls"); continue; }
            try {
                conn->setAutoCommit(false);
                sql::PreparedStatement* p = conn->prepareStatement("INSERT INTO STUDENT (StudentName, Username, Password) VALUES (?,?,?)");
                p->setString(1, name); p->setString(2, user); p->setString(3, pass); p->executeUpdate(); delete p;
                int sid = getStudentID(conn, user);
                sql::PreparedStatement* e = conn->prepareStatement("INSERT INTO STUDENT_COURSE (StudentID, CourseID) VALUES (?,?)");
                e->setInt(1, sid); e->setInt(2, cid); e->executeUpdate(); delete e;
                conn->commit(); drawSuccess("Student Registered!"); cout << "   (Tuition has been automatically billed)\n";
            }
            catch (sql::SQLException& e) { conn->rollback(); drawError(e.what()); }
            conn->setAutoCommit(true);
        }
        (void)_getch();
        system("cls");
    }
}

int main() {
    HWND hwnd = GetConsoleWindow();
    if (hwnd != NULL) { ShowWindow(hwnd, SW_MAXIMIZE); }
    hideCursor();

    sql::Connection* conn = connectDB();
    drawLoadingScreen(conn);

    string ops[] = {
        "Admin Login",
        "Teacher Login",
        "Student Login",
        "Exit"
    };
    int opCount = 4;
    int choice = 0;

    system("cls");
    while (true) {
        while (true) {
            drawMenuFrame("MAIN MENU", ops, opCount, choice);
            char key = (char)_getch();
            if (key == 72) choice = (choice - 1 + opCount) % opCount;
            else if (key == 80) choice = (choice + 1) % opCount;
            else if (key == 13) break;
        }

        if (choice == 0) { string u; if (login(conn, "Admin", u) != -1) adminMenu(conn, u); }
        else if (choice == 1) { string u; if (login(conn, "Teacher", u) != -1) teacherMenu(conn, u); }
        else if (choice == 2) { string u; if (login(conn, "Student", u) != -1) studentMenu(conn, u); }
        else break;

        system("cls");
    }
    delete conn;
    return 0;
}