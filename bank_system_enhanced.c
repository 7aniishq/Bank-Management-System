/*
 * bank_system_enhanced.c
 * Enhanced Bank Management System (single-file reference implementation)
 * Features added beyond basic:
 *  - Admin authentication (username/password)
 *  - Transaction history (transactions.txt)
 *  - Transfer between accounts
 *  - Apply interest to Savings accounts
 *  - Sort accounts when listing (by acc_no, name, balance)
 *  - Export accounts to CSV
 *  - Simple backup/restore of data file
 *  - Improved input validation and user prompts
 *
 * Compile: gcc -o bank_system_enhanced bank_system_enhanced.c
 * Run: ./bank_system_enhanced
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define DATA_FILE "accounts.dat"
#define TRANS_FILE "transactions.txt"
#define BACKUP_FILE "accounts.bak"
#define NAME_LEN 100
#define ADDR_LEN 200
#define PHONE_LEN 20
#define ADMIN_USER "Tanishq"
#define ADMIN_PASS "1234" // change before submitting

typedef struct {
    int acc_no;
    char name[NAME_LEN];
    char type[10]; // "Savings" or "Current"
    double balance;
    char phone[PHONE_LEN];
    char address[ADDR_LEN];
    int active; // 1 = active, 0 = closed
} Account;

/* Utility prototypes */
void clear_stdin(void);
void pause(void);
int get_next_acc_no(void);
long find_account_pos(int acc_no);
int account_exists(int acc_no, long *pos_out);

/* Core operations */
int admin_login(void);
void create_account(void);
void display_account(void);
void deposit_amount(void);
void withdraw_amount(void);
void modify_account(void);
void close_account(void);
void list_accounts(void);
void transfer_amount(void);
void apply_interest(void);
void export_csv(void);
void backup_data(void);
void restore_data(void);

/* File helpers */
int write_account_at(const Account *acc, long pos);
int append_account(const Account *acc);
int read_account_at(Account *acc, long pos);
int count_accounts(void);

/* Transaction logging */
void log_transaction(int acc_no, const char *type, double amount, double balance_after);

/* Input helpers */
void read_line(char *buf, size_t size);
int read_int(void);
double read_double(void);

/* Sorting helpers */
int compare_acc_no(const void *a, const void *b);
int compare_name(const void *a, const void *b);
int compare_balance(const void *a, const void *b);

int main(void) {
    int choice;

    printf("Welcome to Enhanced Bank Management System\n");
    if (!admin_login()) {
        printf("Authentication failed. Exiting.\n");
        return 0;
    }

    do {
        printf("\n===== Bank Management System (Enhanced) =====\n");
        printf("1. Create Account\n2. Display Account\n3. Deposit\n4. Withdraw\n5. Modify Account\n6. Close Account\n7. List Accounts\n8. Transfer Funds\n9. Apply Interest to Savings\n10. Export Accounts to CSV\n11. Backup Data\n12. Restore Data from Backup\n13. Exit\n");
        printf("Enter choice: ");
        choice = read_int();
        switch (choice) {
            case 1: create_account(); break;
            case 2: display_account(); break;
            case 3: deposit_amount(); break;
            case 4: withdraw_amount(); break;
            case 5: modify_account(); break;
            case 6: close_account(); break;
            case 7: list_accounts(); break;
            case 8: transfer_amount(); break;
            case 9: apply_interest(); break;
            case 10: export_csv(); break;
            case 11: backup_data(); break;
            case 12: restore_data(); break;
            case 13: printf("Exiting...\n"); break;
            default: printf("Invalid choice. Try again.\n");
        }
    } while (choice != 13);

    return 0;
}

/* ----------------- Implementation ----------------- */

int admin_login(void) {
    char user[64], pass[64];
    printf("\nAdmin login required.\nUser: ");
    read_line(user, sizeof(user));
    printf("Password: ");
    read_line(pass, sizeof(pass));
    if (strcmp(user, ADMIN_USER) == 0 && strcmp(pass, ADMIN_PASS) == 0) {
        printf("Login successful.\n");
        return 1;
    }
    return 0;
}

void clear_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

void pause(void) {
    printf("\nPress Enter to continue...");
    getchar();
}

int get_next_acc_no(void) {
    FILE *f = fopen(DATA_FILE, "rb");
    int max = 1000; // start
    if (!f) return max + 1; // file doesn't exist yet
    Account a;
    while (fread(&a, sizeof(Account), 1, f) == 1) {
        if (a.acc_no > max) max = a.acc_no;
    }
    fclose(f);
    return max + 1;
}

long find_account_pos(int acc_no) {
    FILE *f = fopen(DATA_FILE, "rb");
    if (!f) return -1;
    Account a;
    long pos = 0;
    while (fread(&a, sizeof(Account), 1, f) == 1) {
        if (a.acc_no == acc_no) {
            fclose(f);
            return pos;
        }
        pos++;
    }
    fclose(f);
    return -1;
}

int account_exists(int acc_no, long *pos_out) {
    long p = find_account_pos(acc_no);
    if (pos_out) *pos_out = p;
    return (p >= 0);
}

int write_account_at(const Account *acc, long pos) {
    FILE *f = fopen(DATA_FILE, "r+b");
    if (!f) return 0;
    if (fseek(f, pos * sizeof(Account), SEEK_SET) != 0) { fclose(f); return 0; }
    if (fwrite(acc, sizeof(Account), 1, f) != 1) { fclose(f); return 0; }
    fclose(f);
    return 1;
}

int append_account(const Account *acc) {
    FILE *f = fopen(DATA_FILE, "ab");
    if (!f) return 0;
    if (fwrite(acc, sizeof(Account), 1, f) != 1) { fclose(f); return 0; }
    fclose(f);
    return 1;
}

int read_account_at(Account *acc, long pos) {
    FILE *f = fopen(DATA_FILE, "rb");
    if (!f) return 0;
    if (fseek(f, pos * sizeof(Account), SEEK_SET) != 0) { fclose(f); return 0; }
    if (fread(acc, sizeof(Account), 1, f) != 1) { fclose(f); return 0; }
    fclose(f);
    return 1;
}

int count_accounts(void) {
    FILE *f = fopen(DATA_FILE, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    return (int)(size / sizeof(Account));
}

void create_account(void) {
    Account acc;
    memset(&acc, 0, sizeof(acc));
    acc.acc_no = get_next_acc_no();
    printf("\n--- Create New Account ---\n");
    printf("Account Number (auto): %d\n", acc.acc_no);
    printf("Enter holder name: ");
    read_line(acc.name, NAME_LEN);
    do {
        printf("Enter account type (Savings/Current): ");
        read_line(acc.type, sizeof(acc.type));
        if (strcasecmp(acc.type, "Savings") == 0 || strcasecmp(acc.type, "Current") == 0) break;
        printf("Invalid type. Please enter 'Savings' or 'Current'.\n");
    } while (1);
    printf("Enter initial deposit amount: ");
    acc.balance = read_double();
    printf("Enter phone number: ");
    read_line(acc.phone, PHONE_LEN);
    printf("Enter address: ");
    read_line(acc.address, ADDR_LEN);
    acc.active = 1;

    if (append_account(&acc)) {
        printf("Account created successfully. Account No: %d\n", acc.acc_no);
        log_transaction(acc.acc_no, "CREATE", acc.balance, acc.balance);
    } else {
        printf("Failed to create account (file error).\n");
    }
    pause();
}

void display_account(void) {
    printf("\n--- Display Account ---\nEnter account number: ");
    int acc_no = read_int();
    long pos = find_account_pos(acc_no);
    if (pos < 0) { printf("Account not found.\n"); pause(); return; }
    Account acc;
    if (!read_account_at(&acc, pos)) { printf("Error reading account.\n"); pause(); return; }
    if (!acc.active) { printf("Account %d is closed.\n", acc_no); pause(); return; }
    printf("\nAccount No: %d\nName: %s\nType: %s\nBalance: %.2f\nPhone: %s\nAddress: %s\n",
           acc.acc_no, acc.name, acc.type, acc.balance, acc.phone, acc.address);

    // Show recent transactions for this account (last 10)
    FILE *t = fopen(TRANS_FILE, "r");
    if (t) {
        char line[512];
        int shown = 0;
        printf("\nRecent transactions (most recent last):\n");
        while (fgets(line, sizeof(line), t)) {
            int tx_acc;
            if (sscanf(line, "%d", &tx_acc) == 1 && tx_acc == acc_no) {
                printf("%s", line);
                shown++;
                if (shown >= 10) break;
            }
        }
        fclose(t);
        if (shown == 0) printf("No transactions found for this account.\n");
    }
    pause();
}

void deposit_amount(void) {
    printf("\n--- Deposit ---\nEnter account number: ");
    int acc_no = read_int();
    long pos = find_account_pos(acc_no);
    if (pos < 0) { printf("Account not found.\n"); pause(); return; }
    Account acc;
    if (!read_account_at(&acc, pos)) { printf("Error reading account.\n"); pause(); return; }
    if (!acc.active) { printf("Account is closed.\n"); pause(); return; }
    printf("Current balance: %.2f\nEnter deposit amount: ", acc.balance);
    double amt = read_double();
    if (amt <= 0) { printf("Invalid amount.\n"); pause(); return; }
    acc.balance += amt;
    if (write_account_at(&acc, pos)) {
        printf("Deposit successful. New balance: %.2f\n", acc.balance);
        log_transaction(acc.acc_no, "DEPOSIT", amt, acc.balance);
    } else printf("Failed to update account.\n");
    pause();
}

void withdraw_amount(void) {
    printf("\n--- Withdraw ---\nEnter account number: ");
    int acc_no = read_int();
    long pos = find_account_pos(acc_no);
    if (pos < 0) { printf("Account not found.\n"); pause(); return; }
    Account acc;
    if (!read_account_at(&acc, pos)) { printf("Error reading account.\n"); pause(); return; }
    if (!acc.active) { printf("Account is closed.\n"); pause(); return; }
    printf("Current balance: %.2f\nEnter withdrawal amount: ", acc.balance);
    double amt = read_double();
    if (amt <= 0) { printf("Invalid amount.\n"); pause(); return; }
    if (strcasecmp(acc.type, "Savings") == 0 && (acc.balance - amt) < 0) {
        printf("Insufficient funds for Savings account. Withdrawal aborted.\n"); pause(); return;
    }
    acc.balance -= amt;
    if (write_account_at(&acc, pos)) {
        printf("Withdrawal successful. New balance: %.2f\n", acc.balance);
        log_transaction(acc.acc_no, "WITHDRAW", amt, acc.balance);
    } else printf("Failed to update account.\n");
    pause();
}

void modify_account(void) {
    printf("\n--- Modify Account ---\nEnter account number: ");
    int acc_no = read_int();
    long pos = find_account_pos(acc_no);
    if (pos < 0) { printf("Account not found.\n"); pause(); return; }
    Account acc;
    if (!read_account_at(&acc, pos)) { printf("Error reading account.\n"); pause(); return; }
    if (!acc.active) { printf("Account is closed.\n"); pause(); return; }
    printf("Current Phone: %s\nEnter new phone (leave empty to keep): ", acc.phone);
    char tmp[PHONE_LEN]; read_line(tmp, PHONE_LEN);
    if (strlen(tmp) > 0) strncpy(acc.phone, tmp, PHONE_LEN);
    printf("Current Address: %s\nEnter new address (leave empty to keep): ", acc.address);
    char tmp2[ADDR_LEN]; read_line(tmp2, ADDR_LEN);
    if (strlen(tmp2) > 0) strncpy(acc.address, tmp2, ADDR_LEN);
    printf("Current Type: %s\nEnter new type (Savings/Current) or leave empty to keep: ", acc.type);
    char tmp3[16]; read_line(tmp3, sizeof(tmp3));
    if (strlen(tmp3) > 0) {
        if (strcasecmp(tmp3, "Savings") == 0 || strcasecmp(tmp3, "Current") == 0) strncpy(acc.type, tmp3, sizeof(acc.type));
        else printf("Invalid type entered; keeping old type.\n");
    }
    if (write_account_at(&acc, pos)) printf("Account modified successfully.\n");
    else printf("Failed to modify account.\n");
    pause();
}

void close_account(void) {
    printf("\n--- Close Account ---\nEnter account number: ");
    int acc_no = read_int();
    long pos = find_account_pos(acc_no);
    if (pos < 0) { printf("Account not found.\n"); pause(); return; }
    Account acc;
    if (!read_account_at(&acc, pos)) { printf("Error reading account.\n"); pause(); return; }
    if (!acc.active) { printf("Account already closed.\n"); pause(); return; }
    printf("Are you sure you want to close account %d? (y/n): ", acc_no);
    char c = getchar(); clear_stdin();
    if (c == 'y' || c == 'Y') {
        acc.active = 0;
        if (write_account_at(&acc, pos)) {
            printf("Account closed successfully.\n");
            log_transaction(acc.acc_no, "CLOSE", 0.0, acc.balance);
        }
        else printf("Failed to close account.\n");
    } else {
        printf("Operation cancelled.\n");
    }
    pause();
}

void list_accounts(void) {
    int n = count_accounts();
    if (n == 0) { printf("No accounts found.\n"); pause(); return; }
    Account *arr = malloc(sizeof(Account) * n);
    if (!arr) { printf("Memory error.\n"); pause(); return; }
    FILE *f = fopen(DATA_FILE, "rb");
    if (!f) { free(arr); printf("No accounts found.\n"); pause(); return; }
    fread(arr, sizeof(Account), n, f);
    fclose(f);

    printf("Choose sort order:\n1. Account Number\n2. Name\n3. Balance\nEnter choice: ");
    int so = read_int();
    if (so == 2) qsort(arr, n, sizeof(Account), compare_name);
    else if (so == 3) qsort(arr, n, sizeof(Account), compare_balance);
    else qsort(arr, n, sizeof(Account), compare_acc_no);

    printf("\n--- Accounts ---\n");
    for (int i = 0; i < n; ++i) {
        if (arr[i].active)
            printf("%d | %s | %s | %.2f\n", arr[i].acc_no, arr[i].name, arr[i].type, arr[i].balance);
    }
    free(arr);
    pause();
}

void transfer_amount(void) {
    printf("\n--- Transfer Funds ---\nFrom account number: ");
    int from = read_int();
    long pos_from = find_account_pos(from);
    if (pos_from < 0) { printf("Source account not found.\n"); pause(); return; }
    Account a_from;
    if (!read_account_at(&a_from, pos_from)) { printf("Error reading source account.\n"); pause(); return; }
    if (!a_from.active) { printf("Source account is closed.\n"); pause(); return; }

    printf("To account number: ");
    int to = read_int();
    long pos_to = find_account_pos(to);
    if (pos_to < 0) { printf("Destination account not found.\n"); pause(); return; }
    Account a_to;
    if (!read_account_at(&a_to, pos_to)) { printf("Error reading destination account.\n"); pause(); return; }
    if (!a_to.active) { printf("Destination account is closed.\n"); pause(); return; }

    printf("Enter amount to transfer: ");
    double amt = read_double();
    if (amt <= 0) { printf("Invalid amount.\n"); pause(); return; }
    if (strcasecmp(a_from.type, "Savings") == 0 && (a_from.balance - amt) < 0) {
        printf("Insufficient funds in source account.\n"); pause(); return; }

    a_from.balance -= amt;
    a_to.balance += amt;

    if (write_account_at(&a_from, pos_from) && write_account_at(&a_to, pos_to)) {
        printf("Transfer successful. New balances: %d -> %.2f, %d -> %.2f\n", a_from.acc_no, a_from.balance, a_to.acc_no, a_to.balance);
        log_transaction(a_from.acc_no, "TRANSFER_OUT", amt, a_from.balance);
        log_transaction(a_to.acc_no, "TRANSFER_IN", amt, a_to.balance);
    } else printf("Transfer failed while updating accounts.\n");
    pause();
}

void apply_interest(void) {
    printf("\n--- Apply Interest to Savings Accounts ---\nEnter annual interest rate (percent): ");
    double rate = read_double();
    if (rate <= 0) { printf("Invalid rate.\n"); pause(); return; }
    int n = count_accounts();
    if (n == 0) { printf("No accounts to update.\n"); pause(); return; }
    Account *arr = malloc(sizeof(Account) * n);
    if (!arr) { printf("Memory error.\n"); pause(); return; }
    FILE *f = fopen(DATA_FILE, "rb");
    if (!f) { free(arr); printf("No accounts found.\n"); pause(); return; }
    fread(arr, sizeof(Account), n, f);
    fclose(f);

    for (int i = 0; i < n; ++i) {
        if (arr[i].active && strcasecmp(arr[i].type, "Savings") == 0) {
            double interest = arr[i].balance * (rate / 100.0) / 12.0; // monthly interest
            arr[i].balance += interest;
            log_transaction(arr[i].acc_no, "INTEREST", interest, arr[i].balance);
        }
    }

    // write all back
    FILE *w = fopen(DATA_FILE, "r+b");
    if (!w) { free(arr); printf("Failed to open data file for writing.\n"); pause(); return; }
    fwrite(arr, sizeof(Account), n, w);
    fclose(w);
    free(arr);
    printf("Interest applied (monthly) to all savings accounts.\n");
    pause();
}

void export_csv(void) {
    FILE *f = fopen(DATA_FILE, "rb");
    if (!f) { printf("No accounts to export.\n"); pause(); return; }
    FILE *c = fopen("accounts_export.csv", "w");
    if (!c) { fclose(f); printf("Failed to create CSV.\n"); pause(); return; }
    fprintf(c, "acc_no,name,type,balance,phone,address,active\n");
    Account a;
    while (fread(&a, sizeof(Account), 1, f) == 1) {
        fprintf(c, "%d,\"%s\",%s,%.2f,\"%s\",\"%s\",%d\n", a.acc_no, a.name, a.type, a.balance, a.phone, a.address, a.active);
    }
    fclose(f); fclose(c);
    printf("Exported to accounts_export.csv\n");
    pause();
}

void backup_data(void) {
    FILE *src = fopen(DATA_FILE, "rb");
    if (!src) { printf("No data file to backup.\n"); pause(); return; }
    FILE *dst = fopen(BACKUP_FILE, "wb");
    if (!dst) { fclose(src); printf("Failed to create backup file.\n"); pause(); return; }
    char buf[4096];
    size_t r;
    while ((r = fread(buf, 1, sizeof(buf), src)) > 0) fwrite(buf, 1, r, dst);
    fclose(src); fclose(dst);
    printf("Backup created: %s\n", BACKUP_FILE);
    pause();
}

void restore_data(void) {
    FILE *src = fopen(BACKUP_FILE, "rb");
    if (!src) { printf("No backup file found.\n"); pause(); return; }
    FILE *dst = fopen(DATA_FILE, "wb");
    if (!dst) { fclose(src); printf("Failed to restore data.\n"); pause(); return; }
    char buf[4096];
    size_t r;
    while ((r = fread(buf, 1, sizeof(buf), src)) > 0) fwrite(buf, 1, r, dst);
    fclose(src); fclose(dst);
    printf("Data restored from backup.\n");
    pause();
}

void log_transaction(int acc_no, const char *type, double amount, double balance_after) {
    FILE *t = fopen(TRANS_FILE, "a");
    if (!t) return;
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", tm);
    fprintf(t, "%d, %s, %.2f, %.2f, %s\n", acc_no, type, amount, balance_after, timestr);
    fclose(t);
}

/* ----------------- Input helpers ----------------- */

void read_line(char *buf, size_t size) {
    if (!buf || size == 0) return;
    if (fgets(buf, (int)size, stdin) == NULL) { buf[0] = '\0'; return; }
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
}

int read_int(void) {
    int x;
    char line[128];
    while (1) {
        if (!fgets(line, sizeof(line), stdin)) return 0;
        char *end;
        x = (int)strtol(line, &end, 10);
        if (end != line && (*end == '\n' || *end == '\0')) return x;
        printf("Invalid integer. Try again: ");
    }
}

double read_double(void) {
    double d;
    char line[128];
    while (1) {
        if (!fgets(line, sizeof(line), stdin)) return 0.0;
        char *end;
        d = strtod(line, &end);
        if (end != line && (*end == '\n' || *end == '\0')) return d;
        printf("Invalid number. Try again: ");
    }
}

/* ----------------- Sorting ----------------- */
int compare_acc_no(const void *a, const void *b) {
    const Account *A = a, *B = b;
    return (A->acc_no - B->acc_no);
}
int compare_name(const void *a, const void *b) {
    const Account *A = a, *B = b;
    return strcasecmp(A->name, B->name);
}
int compare_balance(const void *a, const void *b) {
    const Account *A = a, *B = b;
    if (A->balance < B->balance) return -1;
    if (A->balance > B->balance) return 1;
    return 0;
}

/* ----------------- End ----------------- */
