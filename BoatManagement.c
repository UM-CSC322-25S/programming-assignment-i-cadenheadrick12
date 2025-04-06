#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOATS 120
#define NAME_LEN 128

// Enum for the location types a boat can be stored in
typedef enum { SLIP, LAND, TRAILOR, STORAGE } BoatType;

// Union for holding specific location details depending on BoatType
// Saves memory by only using the field needed for the boat's type
typedef union {
    int slip_number;        // Valid range: 1-85
    char bay_letter;        // For land: a letter A-Z
    char trailor_tag[16];   // License plate for trailors
    int storage_number;     // Valid range: 1-50
} Location;

// Struct to represent a boat
// Holds all relevant information, including owed amount
typedef struct {
    char name[NAME_LEN];
    int length; // up to 100 feet
    BoatType type;
    Location location;
    float amount_owed;
} Boat;

Boat* marina[MAX_BOATS]; // Array of pointers to boats
int boat_count = 0;       // Tracks how many boats are currently stored

// Function declarations
void load_data(const char* filename);
void save_data(const char* filename);
void print_inventory();
void add_boat(char* csv_line);
void remove_boat(char* name);
void accept_payment(char* name, float amount);
void update_month();
void sort_boats();
int find_boat_index(const char* name);
float monthly_charge(Boat* boat);
BoatType parse_type(const char* str);
const char* type_to_str(BoatType type);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s BoatData.csv\n", argv[0]);
        return 1;
    }

    load_data(argv[1]);

    printf("Welcome to the Boat Management System\n-------------------------------------\n");
    char option[10];
    while (1) {
        printf("\n(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
        fgets(option, sizeof(option), stdin);
        char choice = toupper(option[0]);

        if (choice == 'I') {
            print_inventory();
        } else if (choice == 'A') {
            char line[256];
            printf("Please enter the boat data in CSV format                 : ");
            fgets(line, sizeof(line), stdin);
            line[strcspn(line, "\n")] = 0; // Remove newline
            add_boat(line);
        } else if (choice == 'R') {
            char name[NAME_LEN];
            printf("Please enter the boat name                               : ");
            fgets(name, sizeof(name), stdin);
            name[strcspn(name, "\n")] = 0;
            remove_boat(name);
        } else if (choice == 'P') {
            char name[NAME_LEN];
            float amount;
            printf("Please enter the boat name                               : ");
            fgets(name, sizeof(name), stdin);
            name[strcspn(name, "\n")] = 0;
            int index = find_boat_index(name);
            if (index == -1) {
                printf("No boat with that name\n");
                continue;
            }
            printf("Please enter the amount to be paid                       : ");
            scanf("%f", &amount); getchar();
            if (amount > marina[index]->amount_owed) {
                printf("That is more than the amount owed, $%.2f\n", marina[index]->amount_owed);
            } else {
                accept_payment(name, amount);
            }
        } else if (choice == 'M') {
            update_month(); // Charge all boats for a new month
        } else if (choice == 'X') {
            break; // Exit the loop
        } else {
            printf("Invalid option %c\n", choice); // Handle invalid input
        }
    }

    printf("\nExiting the Boat Management System\n");
    save_data(argv[1]);
    return 0;
}

// Load boat data from a CSV file
void load_data(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        add_boat(line);
    }
    fclose(file);
}

// Save boat data back to the CSV file on exit
void save_data(const char* filename) {
    FILE* file = fopen(filename, "w");
    for (int i = 0; i < boat_count; ++i) {
        Boat* b = marina[i];
        switch (b->type) {
            case SLIP:
                fprintf(file, "%s,%d,slip,%d,%.2f\n", b->name, b->length, b->location.slip_number, b->amount_owed); break;
            case LAND:
                fprintf(file, "%s,%d,land,%c,%.2f\n", b->name, b->length, b->location.bay_letter, b->amount_owed); break;
            case TRAILOR:
                fprintf(file, "%s,%d,trailor,%s,%.2f\n", b->name, b->length, b->location.trailor_tag, b->amount_owed); break;
            case STORAGE:
                fprintf(file, "%s,%d,storage,%d,%.2f\n", b->name, b->length, b->location.storage_number, b->amount_owed); break;
        }
    }
    fclose(file);
}

// Add a new boat using a single line of CSV input
void add_boat(char* csv_line) {
    if (boat_count >= MAX_BOATS) return;
    Boat* b = malloc(sizeof(Boat));
    char type_str[16], extra[32];
    sscanf(csv_line, "%127[^,],%d,%15[^,],%31[^,],%f",
        b->name, &b->length, type_str, extra, &b->amount_owed);
    b->type = parse_type(type_str);
    switch (b->type) {
        case SLIP: b->location.slip_number = atoi(extra); break;
        case LAND: b->location.bay_letter = extra[0]; break;
        case TRAILOR: strcpy(b->location.trailor_tag, extra); break;
        case STORAGE: b->location.storage_number = atoi(extra); break;
    }
    marina[boat_count++] = b;
    sort_boats(); // Keep inventory alphabetized
}

// Display the current list of boats
void print_inventory() {
    for (int i = 0; i < boat_count; ++i) {
        Boat* b = marina[i];
        printf("%-20s %3d' %8s ", b->name, b->length, type_to_str(b->type));
        switch (b->type) {
            case SLIP:    printf(" # %2d", b->location.slip_number); break;
            case LAND:    printf("    %c", b->location.bay_letter); break;
            case TRAILOR: printf("%6s", b->location.trailor_tag); break;
            case STORAGE: printf(" # %2d", b->location.storage_number); break;
        }
        printf("   Owes $%7.2f\n", b->amount_owed);
    }
}

// Remove a boat from the inventory by name
void remove_boat(char* name) {
    int index = find_boat_index(name);
    if (index == -1) {
        printf("No boat with that name\n");
        return;
    }
    free(marina[index]); // Free memory
    for (int i = index; i < boat_count - 1; ++i)
        marina[i] = marina[i + 1];
    boat_count--;
}

// Deduct payment from a boat's balance
void accept_payment(char* name, float amount) {
    int index = find_boat_index(name);
    if (index != -1) {
        marina[index]->amount_owed -= amount;
    }
}

// Add charges to all boats for a new month
void update_month() {
    for (int i = 0; i < boat_count; ++i) {
        marina[i]->amount_owed += monthly_charge(marina[i]);
    }
}

// Sort the boat array alphabetically by name (case-insensitive)
void sort_boats() {
    for (int i = 0; i < boat_count - 1; ++i) {
        for (int j = i + 1; j < boat_count; ++j) {
            if (strcasecmp(marina[i]->name, marina[j]->name) > 0) {
                Boat* tmp = marina[i];
                marina[i] = marina[j];
                marina[j] = tmp;
            }
        }
    }
}

// Look up a boat by name (case-insensitive)
int find_boat_index(const char* name) {
    for (int i = 0; i < boat_count; ++i) {
        if (strcasecmp(name, marina[i]->name) == 0)
            return i;
    }
    return -1;
}

// Calculate what a boat owes for one month based on location type
float monthly_charge(Boat* boat) {
    switch (boat->type) {
        case SLIP: return boat->length * 12.5f;
        case LAND: return boat->length * 14.0f;
        case TRAILOR: return boat->length * 25.0f;
        case STORAGE: return boat->length * 11.2f;
        default: return 0.0f;
    }
}

// Convert string to BoatType
BoatType parse_type(const char* str) {
    if (strcasecmp(str, "slip") == 0) return SLIP;
    if (strcasecmp(str, "land") == 0) return LAND;
    if (strcasecmp(str, "trailor") == 0) return TRAILOR;
    return STORAGE;
}

// Convert BoatType to string for display
const char* type_to_str(BoatType type) {
    switch (type) {
        case SLIP: return "slip";
        case LAND: return "land";
        case TRAILOR: return "trailor";
        case STORAGE: return "storage";
        default: return "unknown";
    }
}

