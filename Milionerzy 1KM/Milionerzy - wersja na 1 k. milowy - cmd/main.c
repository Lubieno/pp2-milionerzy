#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#define MAX_QUESTIONS 100
#define MAX_LENGTH 256
#define SELECTED_QUESTIONS 12
#define HELP_LIMIT 3
#define TIME_LIMIT 60

// Struktura przechowująca dane gracza
typedef struct {
    int id;                 // Unikalny identyfikator gracza
    char name[MAX_LENGTH];  // Nazwa gracza
    int wins;               // Liczba wygranych gier
    int losses;             // Liczba przegranych gier
} Player;

// Struktura przechowująca pytanie i odpowiedzi
typedef struct {
    char text[MAX_LENGTH];      // Treść pytania
    char answers[4][MAX_LENGTH]; // 4 możliwe odpowiedzi
    int correct;                // Indeks poprawnej odpowiedzi (0-3)
} Question;

// Tablica nagród za kolejne pytania
const char* rewards[SELECTED_QUESTIONS] = {
    "500 zl", "1,000 zl", "2,000 zl", "5,000 zl",
    "10,000 zl", "20,000 zl", "40,000 zl", "75,000 zl",
    "125,000 zl", "250,000 zl", "500,000 zl", "1,000,000 zl"
};

// Wyświetla ekran powitalny gry
void showWelcomeScreen() {
    clearScreen();
    printf("========================================\n");
    printf(" PROJEKT - PODSTAWY PROGRAMOWANIA 2\n");
    printf("      \"Gra - Milionerzy\"\n");
    printf("----------------------------------------\n");
    printf(" Stworzone przez:\n");
    printf(" Kamil Lubieniecki i Marcel Pietrzak\n");
    printf("========================================\n");
    printf("\n Nacisnij dowolny przycisk aby kontynuowac...");
    _getch();
}

// Czyści ekran konsoli
void clearScreen() {
    system("cls");
}

// Usuwa cudzysłowy z początku i końca stringa
void cleanString(char* str) {
    if (str[0] == '"') memmove(str, str + 1, strlen(str));
    int len = strlen(str);
    if (len > 0 && str[len - 1] == '"') str[len - 1] = '\0';
}

// Wczytuje pytania z pliku do tablicy
int loadQuestions(Question q[], const char* file) {
    FILE* f = fopen(file, "r");
    if (!f) {
        printf("Blad otwarcia pliku %s\n", file);
        getchar(); getchar();
        exit(1);
    }

    int count = 0;
    char line[1024];
    // Czyta plik linia po linii
    while (fgets(line, sizeof(line), f) && count < MAX_QUESTIONS) {
        // Dzieli linię na części rozdzielone średnikami
        char* token = strtok(line, ";");
        cleanString(token);
        strncpy(q[count].text, token, MAX_LENGTH);

        // Wczytuje 4 odpowiedzi
        for (int i = 0; i < 4; i++) {
            token = strtok(NULL, ";");
            cleanString(token);
            strncpy(q[count].answers[i], token, MAX_LENGTH);
        }

        // Wczytuje indeks poprawnej odpowiedzi
        token = strtok(NULL, ";");
        q[count].correct = atoi(token);
        count++;
    }

    fclose(f);
    return count;
}

// Zapisuje listę graczy do pliku
void savePlayers(Player* p, int count) {
    FILE* f = fopen("profiles.txt", "w");
    for (int i = 0; i < count; i++) {
        fprintf(f, "%d;%s;%d;%d\n", p[i].id, p[i].name, p[i].wins, p[i].losses);
    }
    fclose(f);
}

// Wczytuje listę graczy z pliku
int loadPlayers(Player* p) {
    FILE* f = fopen("profiles.txt", "r");
    if (!f) return 0;
    int count = 0;
    // Wczytuje dane graczy aż do końca pliku
    while (fscanf(f, "%d;%[^;];%d;%d\n", &p[count].id, p[count].name, &p[count].wins, &p[count].losses) == 4) {
        count++;
    }
    fclose(f);
    return count;
}

// Znajduje gracza po nazwie
int findPlayerByName(Player* p, int count, const char* name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(p[i].name, name) == 0) return i;
    }
    return -1;
}

// Generuje nowe ID dla gracza
int getNextPlayerId(Player* p, int count) {
    int maxId = 0;
    for (int i = 0; i < count; i++) {
        if (p[i].id > maxId) maxId = p[i].id;
    }
    return maxId + 1;
}

// Wyświetla listę wszystkich graczy
void showPlayerList(Player* p, int count) {
    clearScreen();
    printf("=== LISTA GRACZY ===\n\n");
    printf("ID  | Indeks | Nazwa gracza | Wygrane | Przegrane\n");
    printf("-------------------------------------------------\n");

    for (int i = 0; i < count; i++) {
        printf("%3d | %6d | %-12s | %7d | %8d\n",
               p[i].id, i+1, p[i].name, p[i].wins, p[i].losses);
    }
    printf("\n");
}

// Usuwa gracza z listy
void deletePlayer(Player* p, int* count) {
    showPlayerList(p, *count);
    printf("Podaj ID gracza do usuniecia: ");
    int id;
    scanf("%d", &id);

    int idx = -1;
    // Szuka gracza o podanym ID
    for (int i = 0; i < *count; i++) {
        if (p[i].id == id) {
            idx = i;
            break;
        }
    }

    if (idx >= 0) {
        printf("Czy na pewno chcesz usunac gracza %s? (T/N): ", p[idx].name);
        char confirm;
        scanf(" %c", &confirm);

        if (confirm == 'T' || confirm == 't') {
            // Przesuwa elementy tablicy, aby usunąć gracza
            for (int i = idx; i < *count - 1; i++) {
                p[i] = p[i + 1];
            }
            (*count)--;
            savePlayers(p, *count);
            printf("Usunieto gracza\n");
        } else {
            printf("Anulowano usuwanie\n");
        }
    } else {
        printf("Nie znaleziono gracza o podanym ID\n");
    }
    getchar(); getchar();
}

// Główna funkcja odpowiedzialna za rozgrywkę
void playGame(Player* p, Question* q, int total) {
    int usedHelp = 0;
    int selected[SELECTED_QUESTIONS] = {0};
    int selectedCount = 0;

    // Wybór unikalnych pytań - lista cykliczna !!!
    // Losuje pytania aż do uzyskania wymaganej liczby unikalnych pytań
    while (selectedCount < SELECTED_QUESTIONS) {
        int r = rand() % total;
        int duplicate = 0;
        // Sprawdza czy pytanie już zostało wybrane
        for (int i = 0; i < selectedCount; i++) {
            if (selected[i] == r) {
                duplicate = 1;
                break;
            }
        }
        if (!duplicate) {
            selected[selectedCount++] = r;
        }
    }

    // Główna pętla gry dla każdego pytania
    for (int i = 0; i < SELECTED_QUESTIONS; i++) {
        Question* current = &q[selected[i]];
        int hidden[4] = {0};    // Flagi ukrytych odpowiedzi (do pomocy 50:50)
        int phoneUsed = 0;      // Flaga użycia telefonu
        int fiftyUsed = 0;      // Flaga użycia 50:50
        char input = 0;         // Wejście od użytkownika

        time_t start = time(NULL);
        time_t last = start;

        // Pętla dla pojedynczego pytania
        while (1) {
            time_t now = time(NULL);
            if (now - last >= 1 || input != 0) {
                clearScreen();
                printf("Gracz: %s | Pytanie %d za %s\n\n", p->name, i + 1, rewards[i]);
                printf("%s\n", current->text);
                for (int j = 0; j < 4; j++) {
                    if (!hidden[j]) printf("%c) %s\n", 'A' + j, current->answers[j]);
                    else printf("%c) ---\n", 'A' + j);
                }
                printf("\n[T] Telefon    [5] 50/50   (Pozostalo %d pomocy)\n", HELP_LIMIT - usedHelp);
                printf("Czas: %lds\n", TIME_LIMIT - (now - start));
                last = now;
            }

            if (_kbhit()) {
                input = _getch();
                if (input >= 'a' && input <= 'z') input -= 32;
            }

            if (input != 0) {
                // Obsługa pomocy - telefon do przyjaciela
                if (input == 'T' && !phoneUsed && usedHelp < HELP_LIMIT) {
                    phoneUsed = 1;
                    usedHelp++;
                    clearScreen();
                    printf("Gracz: %s | Pytanie %d za %s\n\n", p->name, i + 1, rewards[i]);
                    printf("%s\n", current->text);
                    for (int j = 0; j < 4; j++) {
                        if (!hidden[j]) printf("%c) %s\n", 'A' + j, current->answers[j]);
                        else printf("%c) ---\n", 'A' + j);
                    }
                    printf("\n[T] Telefon    [5] 50/50   (Pozostalo %d pomocy)\n", HELP_LIMIT - usedHelp);
                    printf("Czas: %lds\n", TIME_LIMIT - (now - start));

                    if (rand() % 100 < 80)
                        printf("\nPrzyjaciel: Mysle, ze to %c\n", 'A' + current->correct);
                    else
                        printf("\nPrzyjaciel: Nie wiem, sorry\n");
                    getchar();
                    input = 0;
                    continue;
                }
                // Obsługa pomocy - 50:50
                else if (input == '5' && !fiftyUsed && usedHelp < HELP_LIMIT) {
                    fiftyUsed = 1;
                    usedHelp++;
                    int removed = 0;
                    // Losowo usuwa 2 błędne odpowiedzi
                    while (removed < 2) {
                        int r = rand() % 4;
                        if (r != current->correct && !hidden[r]) {
                            hidden[r] = 1;
                            removed++;
                        }
                    }
                    input = 0;
                    continue;
                }
                // Sprawdzenie odpowiedzi gracza
                else if (input >= 'A' && input <= 'D') {
                    int answer = input - 'A';
                    if (hidden[answer]) {
                        printf("\nTa odpowiedz jest ukryta\n");
                        getchar(); getchar();
                        input = 0;
                        continue;
                    }
                    if (answer == current->correct) {
                        printf("\nPoprawna odpowiedz! Wygrywasz: %s\n", rewards[i]);
                    } else {
                        printf("\nZle! Poprawna: %c\n", 'A' + current->correct);
                        p->losses++;
                        return;
                    }
                    getchar(); getchar();
                    break;
                } else {
                    printf("\nNieprawidlowy wybor\n");
                    getchar(); getchar();
                    input = 0;
                    continue;
                }
            }

            // Sprawdzenie upływu czasu
            if (now - start >= TIME_LIMIT) {
                printf("\nKoniec czasu!\n");
                p->losses++;
                return;
            }

            Sleep(100);
        }
    }

    // Gracz wygrał milion!
    p->wins++;
    printf("\nWYGRANA! Milion zlotych!\n");
    printf("\nStatystyki:\n");
    printf("Gracz: %s\n", p->name);
    printf("Wygrane: %d\n", p->wins);
    printf("Przegrane: %d\n", p->losses);
    getchar();
}

// Wyświetla menu główne i obsługuje wybory użytkownika
int showMenu(Player* player) {
    Player all[100];
    int count = loadPlayers(all);

    // Pętla menu głównego
    while (1) {
        clearScreen();
        printf("=== MILIONERZY ===\n");
        printf("1. Nowa gra (nowy gracz)\n");
        printf("2. Wczytaj gracza\n");
        printf("3. Usun gracza\n");
        printf("0. Wyjscie\n");
        printf("Wybor: ");
        char choice;
        scanf(" %c", &choice);

        if (choice == '1') {
            printf("Podaj imie gracza: ");
            char name[MAX_LENGTH];
            scanf("%s", name);

            int idx = findPlayerByName(all, count, name);
            if (idx >= 0) {
                printf("Gracz o takiej nazwie juz istnieje!\n");
                getchar(); getchar();
                continue;
            }

            strcpy(player->name, name);
            player->wins = 0;
            player->losses = 0;
            player->id = getNextPlayerId(all, count);

            all[count++] = *player;
            savePlayers(all, count);
            return 1;
        } else if (choice == '2') {
            if (count == 0) {
                printf("Brak zapisanych graczy!\n");
                getchar(); getchar();
                continue;
            }

            showPlayerList(all, count);
            printf("Wybierz ID gracza: ");
            int id;
            scanf("%d", &id);

            int idx = -1;
            for (int i = 0; i < count; i++) {
                if (all[i].id == id) {
                    idx = i;
                    break;
                }
            }

            if (idx >= 0) {
                *player = all[idx];
                printf("Wczytano gracza: %s\n", player->name);
                printf("Statystyki: %d wygranych, %d przegranych\n",
                       player->wins, player->losses);
                getchar(); getchar();
                return 1;
            } else {
                printf("Nie znaleziono gracza o podanym ID\n");
                getchar(); getchar();
            }
        } else if (choice == '3') {
            if (count == 0) {
                printf("Brak graczy do usuniecia!\n");
                getchar(); getchar();
                continue;
            }
            deletePlayer(all, &count);
        } else if (choice == '0') {
            return 0;
        }
    }
}

// Główna funkcja programu
int main() {
    srand(time(NULL));
    Player player;

    showWelcomeScreen();

    // Główna pętla programu
    while (1) {
        if (!showMenu(&player)) break;

        char file[32];
        printf("\nWybierz poziom:\n1 - latwe\n2 - srednie\n3 - trudne\nWybor: ");
        char diff;
        scanf(" %c", &diff);
        switch (diff) {
            case '1': strcpy(file, "latwe.txt"); break;
            case '2': strcpy(file, "srednie.txt"); break;
            case '3': strcpy(file, "trudne.txt"); break;
            default: strcpy(file, "latwe.txt"); break;
        }

        Question questions[MAX_QUESTIONS];
        int total = loadQuestions(questions, file);

        playGame(&player, questions, total);

        Player all[100];
        int count = loadPlayers(all);
        int idx = findPlayerByName(all, count, player.name);
        if (idx >= 0) all[idx] = player;
        else all[count++] = player;
        savePlayers(all, count);
    }

    printf("\nKoniec programu\n");
    return 0;
}