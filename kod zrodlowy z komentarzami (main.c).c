/**
 * @file milionerzy.c
 * @author Kamil Lubieniecki, Marcel Pietrzak
 * @date 2025
 * @brief Implementacja gry "Milionerzy"
 * @version 1.0
 * @license MIT
 * 
 * Program wykorzystuje bibliotekę Allegro do renderowania grafiki i obsługi zdarzeń.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_native_dialog.h>

/**
 * @defgroup GameConstants Stałe gry
 * @{
 */
#define SCREEN_WIDTH 1024       /**< Szerokość ekranu w trybie okienkowym */
#define SCREEN_HEIGHT 768       /**< Wysokość ekranu w trybie okienkowym */
#define MAX_QUESTIONS 100       /**< Maksymalna liczba pytań */
#define MAX_LENGTH 256          /**< Maksymalna długość tekstu */
#define MAX_PLAYERS 100         /**< Maksymalna liczba graczy */
#define SELECTED_QUESTIONS 12   /**< Liczba pytań w grze */
#define HELP_LIMIT 3            /**< Limit pomocy w grze */
#define TIME_LIMIT 30           /**< Limit czasu na odpowiedź (w sekundach) */

#define COLOR_BACKGROUND al_map_rgb(5, 10, 30)      /**< Kolor tła */
#define COLOR_TEXT al_map_rgb(255, 255, 255)        /**< Kolor tekstu */
#define COLOR_GOLD al_map_rgb(255, 215, 0)          /**< Kolor złoty */
#define COLOR_BUTTON al_map_rgb(40, 80, 160)        /**< Kolor przycisków */
#define COLOR_CORRECT al_map_rgb(50, 200, 50)       /**< Kolor poprawnej odpowiedzi */
#define COLOR_WRONG al_map_rgb(200, 50, 50)         /**< Kolor błędnej odpowiedzi */
#define COLOR_DELETE al_map_rgb(200, 50, 50)        /**< Kolor przycisku usuwania */
/** @} */

/**
 * @brief Nagrody za kolejne pytania
 */
const char* rewards[SELECTED_QUESTIONS] = {
    "500 zł", "1,000 zł", "2,000 zł", "5,000 zł",
    "10,000 zł", "20,000 zł", "40,000 zł", "75,000 zł",
    "125,000 zł", "250,000 zł", "500,000 zł", "1,000,000 zł"
};

/**
 * @brief Struktura reprezentująca gracza
 */
typedef struct {
    int id;                     /**< Unikalny identyfikator gracza */
    char name[MAX_LENGTH];      /**< Nazwa gracza */
    int wins;                   /**< Liczba wygranych gier */
    int losses;                 /**< Liczba przegranych gier */
} Player;

/**
 * @brief Struktura reprezentująca pytanie
 */
typedef struct {
    char text[MAX_LENGTH];      /**< Treść pytania */
    char answers[4][MAX_LENGTH];/**< Tablica odpowiedzi */
    int correct;                /**< Indeks poprawnej odpowiedzi */
} Question;

/**
 * @brief Struktura przechowująca zasoby gry
 */
typedef struct {
    ALLEGRO_DISPLAY* display;       /**< Wskaźnik na ekran */
    ALLEGRO_EVENT_QUEUE* event_queue; /**< Kolejka zdarzeń */
    ALLEGRO_FONT* font_large;       /**< Duża czcionka */
    ALLEGRO_FONT* font_medium;      /**< Średnia czcionka */
    ALLEGRO_FONT* font_small;       /**< Mała czcionka */
    ALLEGRO_BITMAP* background;     /**< Obraz tła */
    ALLEGRO_SAMPLE* click_sound;    /**< Dźwięk kliknięcia */
    ALLEGRO_SAMPLE* correct_sound;  /**< Dźwięk poprawnej odpowiedzi */
    ALLEGRO_SAMPLE* wrong_sound;    /**< Dźwięk błędnej odpowiedzi */
    ALLEGRO_SAMPLE* win_sound;      /**< Dźwięk wygranej */
    int display_width;              /**< Aktualna szerokość ekranu */
    int display_height;             /**< Aktualna wysokość ekranu */
} GameResources;

/** Globalna tablica graczy */
Player all_players[MAX_PLAYERS];
/** Liczba załadowanych graczy */
int player_count = 0;
/** Tryb pełnoekranowy */
bool fullscreen_mode = false;

/**
 * @defgroup UtilityFunctions Funkcje pomocnicze
 * @{
 */

/**
 * @brief Usuwa cudzysłowy z początku i końca stringa
 * @param[in,out] str Łańcuch znaków do oczyszczenia
 */
void clean_string(char* str) {
    if (str[0] == '"') memmove(str, str + 1, strlen(str));
    int len = strlen(str);
    if (len > 0 && str[len-1] == '"') str[len-1] = '\0';
}

/**
 * @brief Przełącza tryb pełnoekranowy
 * @param[in,out] res Wskaźnik na zasoby gry
 */
void toggle_fullscreen(GameResources* res) {
    fullscreen_mode = !fullscreen_mode;
    al_set_display_flag(res->display, ALLEGRO_FULLSCREEN_WINDOW, fullscreen_mode);

    if (fullscreen_mode) {
        ALLEGRO_MONITOR_INFO info;
        al_get_monitor_info(0, &info);
        res->display_width = info.x2 - info.x1;
        res->display_height = info.y2 - info.y1;
    } else {
        res->display_width = SCREEN_WIDTH;
        res->display_height = SCREEN_HEIGHT;
    }

    al_acknowledge_resize(res->display);
}

/**
 * @brief Skaluje współrzędną X do aktualnego rozmiaru ekranu
 * @param res Wskaźnik na zasoby gry
 * @param x Współrzędna X do przeskalowania
 * @return Przeskalowana współrzędna X
 */
float scale_x(GameResources* res, float x) {
    return x * (float)res->display_width / SCREEN_WIDTH;
}

/**
 * @brief Skaluje współrzędną Y do aktualnego rozmiaru ekranu
 * @param res Wskaźnik na zasoby gry
 * @param y Współrzędna Y do przeskalowania
 * @return Przeskalowana współrzędna Y
 */
float scale_y(GameResources* res, float y) {
    return y * (float)res->display_height / SCREEN_HEIGHT;
}

/**
 * @brief Rysuje tekst wycentrowany na ekranie
 * @param res Wskaźnik na zasoby gry
 * @param font Czcionka do użycia
 * @param text Tekst do wyświetlenia
 * @param y Pozycja Y tekstu
 * @param color Kolor tekstu
 */
void draw_text_centered(GameResources* res, ALLEGRO_FONT* font, const char* text, float y, ALLEGRO_COLOR color) {
    int width = al_get_text_width(font, text);
    al_draw_text(font, color, res->display_width/2 - width/2, scale_y(res, y), 0, text);
}

/**
 * @brief Rysuje przycisk
 * @param res Wskaźnik na zasoby gry
 * @param font Czcionka do użycia
 * @param text Tekst przycisku
 * @param x Pozycja X przycisku
 * @param y Pozycja Y przycisku
 * @param w Szerokość przycisku
 * @param h Wysokość przycisku
 * @param bg Kolor tła przycisku
 * @param text_color Kolor tekstu przycisku
 */
void draw_button(GameResources* res, ALLEGRO_FONT* font, const char* text, float x, float y, float w, float h,
                ALLEGRO_COLOR bg, ALLEGRO_COLOR text_color) {
    float sx = scale_x(res, x);
    float sy = scale_y(res, y);
    float sw = scale_x(res, w);
    float sh = scale_y(res, h);

    al_draw_filled_rounded_rectangle(sx, sy, sx+sw, sy+sh, 10, 10, bg);
    al_draw_rounded_rectangle(sx, sy, sx+sw, sy+sh, 10, 10, COLOR_TEXT, 2);

    int text_height = al_get_font_line_height(font);
    int text_width = al_get_text_width(font, text);
    al_draw_text(font, text_color, sx + sw/2 - text_width/2, sy + sh/2 - text_height/2, 0, text);
}

/**
 * @brief Sprawdza czy kursor myszy jest nad przyciskiem
 * @param res Wskaźnik na zasoby gry
 * @param mx Pozycja X kursora
 * @param my Pozycja Y kursora
 * @param x Pozycja X przycisku
 * @param y Pozycja Y przycisku
 * @param w Szerokość przycisku
 * @param h Wysokość przycisku
 * @return true jeśli kursor jest nad przyciskiem, false w przeciwnym wypadku
 */
bool is_mouse_over_button(GameResources* res, float mx, float my, float x, float y, float w, float h) {
    float sx = scale_x(res, x);
    float sy = scale_y(res, y);
    float sw = scale_x(res, w);
    float sh = scale_y(res, h);

    return (mx >= sx && mx <= sx+sw && my >= sy && my <= sy+sh);
}
/** @} */

/**
 * @defgroup FileOperations Funkcje operacji na plikach
 * @{
 */

/**
 * @brief Wczytuje pytania z pliku
 * @param filename Nazwa pliku z pytaniami
 * @param count Wskaźnik na liczbę wczytanych pytań
 * @return Wskaźnik na tablicę pytań lub NULL w przypadku błędu
 */
Question* load_questions(const char* filename, int* count) {
    FILE* file = fopen(filename, "r");
    if (!file) return NULL;

    Question* questions = malloc(MAX_QUESTIONS * sizeof(Question));
    if (!questions) {
        fclose(file);
        return NULL;
    }

    *count = 0;
    char line[1024];

    while (fgets(line, sizeof(line), file) && *count < MAX_QUESTIONS) {
        char* token = strtok(line, ";");
        clean_string(token);
        strncpy(questions[*count].text, token, MAX_LENGTH);

        for (int i = 0; i < 4; i++) {
            token = strtok(NULL, ";");
            clean_string(token);
            strncpy(questions[*count].answers[i], token, MAX_LENGTH);
        }

        token = strtok(NULL, ";");
        questions[*count].correct = atoi(token);
        (*count)++;
    }

    fclose(file);
    return questions;
}

/**
 * @brief Zapisuje listę graczy do pliku
 */
void save_players() {
    FILE* file = fopen("profiles.txt", "w");
    if (!file) return;

    for (int i = 0; i < player_count; i++) {
        fprintf(file, "%d;%s;%d;%d\n",
               all_players[i].id,
               all_players[i].name,
               all_players[i].wins,
               all_players[i].losses);
    }
    fclose(file);
}

/**
 * @brief Wczytuje listę graczy z pliku
 * @return Liczba wczytanych graczy
 */
int load_players() {
    FILE* file = fopen("profiles.txt", "r");
    if (!file) return 0;

    player_count = 0;
    while (fscanf(file, "%d;%[^;];%d;%d\n",
                &all_players[player_count].id,
                all_players[player_count].name,
                &all_players[player_count].wins,
                &all_players[player_count].losses) == 4) {
        player_count++;
        if (player_count >= MAX_PLAYERS) break;
    }

    fclose(file);
    return player_count;
}

/**
 * @brief Wyszukuje gracza po nazwie
 * @param name Nazwa gracza do znalezienia
 * @return Indeks gracza w tablicy lub -1 jeśli nie znaleziono
 */
int find_player_by_name(const char* name) {
    for (int i = 0; i < player_count; i++) {
        if (strcmp(all_players[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Generuje nowe ID dla gracza
 * @return Nowe unikalne ID gracza
 */
int get_next_player_id() {
    int max_id = 0;
    for (int i = 0; i < player_count; i++) {
        if (all_players[i].id > max_id) {
            max_id = all_players[i].id;
        }
    }
    return max_id + 1;
}
/** @} */

/**
 * @defgroup GameFunctions Funkcje gry
 * @{
 */

/**
 * @brief Inicjalizuje zasoby gry
 * @param res Wskaźnik na zasoby gry do zainicjalizowania
 * @return true jeśli inicjalizacja się powiodła, false w przeciwnym wypadku
 */
bool init_resources(GameResources* res) {
    memset(res, 0, sizeof(GameResources));
    res->display_width = SCREEN_WIDTH;
    res->display_height = SCREEN_HEIGHT;

    if (!al_init()) return false;
    if (!al_install_keyboard()) return false;
    if (!al_install_mouse()) return false;
    if (!al_init_image_addon()) return false;
    if (!al_init_font_addon()) return false;
    if (!al_init_ttf_addon()) return false;
    if (!al_init_primitives_addon()) return false;
    if (!al_install_audio()) return false;
    if (!al_init_acodec_addon()) return false;
    if (!al_reserve_samples(4)) return false;

    res->display = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!res->display) return false;

    res->event_queue = al_create_event_queue();
    if (!res->event_queue) {
        al_destroy_display(res->display);
        return false;
    }

    al_set_window_title(res->display, "Milionerzy");

    res->font_large = al_load_ttf_font("Arial.ttf", 48, 0);
    res->font_medium = al_load_ttf_font("Arial.ttf", 36, 0);
    res->font_small = al_load_ttf_font("Arial.ttf", 24, 0);

    if (!res->font_large || !res->font_medium || !res->font_small) {
        return false;
    }

    res->background = al_load_bitmap("background.png");
    if (!res->background) {
        printf("Nie udalo sie zaladowac tla!\n");
    }

    res->click_sound = al_load_sample("click.wav");
    res->correct_sound = al_load_sample("correct.wav");
    res->wrong_sound = al_load_sample("wrong.wav");
    res->win_sound = al_load_sample("win.wav");

    al_register_event_source(res->event_queue, al_get_display_event_source(res->display));
    al_register_event_source(res->event_queue, al_get_keyboard_event_source());
    al_register_event_source(res->event_queue, al_get_mouse_event_source());

    return true;
}

/**
 * @brief Zwalnia zasoby gry
 * @param res Wskaźnik na zasoby gry do zwolnienia
 */
void cleanup_resources(GameResources* res) {
    if (res->font_large) al_destroy_font(res->font_large);
    if (res->font_medium) al_destroy_font(res->font_medium);
    if (res->font_small) al_destroy_font(res->font_small);
    if (res->background) al_destroy_bitmap(res->background);
    if (res->click_sound) al_destroy_sample(res->click_sound);
    if (res->correct_sound) al_destroy_sample(res->correct_sound);
    if (res->wrong_sound) al_destroy_sample(res->wrong_sound);
    if (res->win_sound) al_destroy_sample(res->win_sound);
    if (res->event_queue) al_destroy_event_queue(res->event_queue);
    if (res->display) al_destroy_display(res->display);

    al_uninstall_audio();
}

/**
 * @brief Wyświetla ekran powitalny
 * @param res Wskaźnik na zasoby gry
 */
void show_welcome_screen(GameResources* res) {
    bool done = false;
    ALLEGRO_EVENT event;

    while (!done) {
        al_wait_for_event(res->event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            exit(0);
        }
        else if (event.type == ALLEGRO_EVENT_KEY_DOWN ||
                 event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            done = true;
        }

        if (res->background) {
            al_draw_scaled_bitmap(res->background, 0, 0,
                                al_get_bitmap_width(res->background),
                                al_get_bitmap_height(res->background),
                                0, 0, res->display_width, res->display_height, 0);
        } else {
            al_clear_to_color(COLOR_BACKGROUND);
        }

        draw_text_centered(res, res->font_large, "PROJEKT - PODSTAWY PROGRAMOWANIA 2", 150, COLOR_GOLD);
        draw_text_centered(res, res->font_large, "\"Gra - Milionerzy\"", 250, COLOR_GOLD);
        draw_text_centered(res, res->font_medium, "Stworzone przez:", 350, COLOR_TEXT);
        draw_text_centered(res, res->font_medium, "Kamil Lubieniecki i Marcel Pietrzak", 400, COLOR_TEXT);
        draw_text_centered(res, res->font_small, "Naciśnij dowolny przycisk aby kontynuować...", 600, al_map_rgb(200, 200, 200));

        al_flip_display();
    }
}

/**
 * @brief Wyświetla główne menu gry
 * @param res Wskaźnik na zasoby gry
 * @return Wybrana opcja menu (1 - Nowa gra, 2 - Wczytaj gracza, 0 - Wyjście)
 */
int show_main_menu(GameResources* res) {
    bool done = false;
    ALLEGRO_EVENT event;
    int selected = 0;
    const char* buttons[] = {"NOWA GRA", "WCZYTAJ GRACZA"};

    while (!done) {
        al_wait_for_event(res->event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            return 0;
        }
        else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            switch (event.keyboard.keycode) {
                case ALLEGRO_KEY_UP:
                    selected = (selected - 1 + 2) % 2;
                    break;
                case ALLEGRO_KEY_DOWN:
                    selected = (selected + 1) % 2;
                    break;
                case ALLEGRO_KEY_ENTER:
                    if (res->click_sound) al_play_sample(res->click_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    return selected + 1;
                case ALLEGRO_KEY_ESCAPE:
                    return 0;
                case ALLEGRO_KEY_F:
                    toggle_fullscreen(res);
                    break;
            }
        }
        else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            float mx = event.mouse.x;
            float my = event.mouse.y;

            for (int i = 0; i < 2; i++) {
                if (is_mouse_over_button(res, mx, my, SCREEN_WIDTH/2 - 200, 200 + i*100, 400, 60)) {
                    if (res->click_sound) al_play_sample(res->click_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    return i + 1;
                }
            }
        }

        if (res->background) {
            al_draw_scaled_bitmap(res->background, 0, 0,
                                al_get_bitmap_width(res->background),
                                al_get_bitmap_height(res->background),
                                0, 0, res->display_width, res->display_height, 0);
        } else {
            al_clear_to_color(COLOR_BACKGROUND);
        }

        draw_text_centered(res, res->font_large, "MENU GŁÓWNE", 100, COLOR_GOLD);

        for (int i = 0; i < 2; i++) {
            ALLEGRO_COLOR bg = (i == selected) ? al_map_rgb(60, 120, 200) : COLOR_BUTTON;
            draw_button(res, res->font_medium, buttons[i],
                       SCREEN_WIDTH/2 - 200, 200 + i*100, 400, 60,
                       bg, COLOR_TEXT);
        }

        draw_text_centered(res, res->font_small, "Użyj strzałek lub myszki aby wybrać", 600, al_map_rgb(200, 200, 200));
        draw_text_centered(res, res->font_small, "F - Przełącz tryb pełnoekranowy", 650, al_map_rgb(200, 200, 200));

        al_flip_display();
    }

    return 0;
}

/**
 * @brief Wyświetla menu wyboru trudności
 * @param res Wskaźnik na zasoby gry
 * @return Wybrany poziom trudności (1 - Łatwy, 2 - Średni, 3 - Trudny)
 */
int show_difficulty_menu(GameResources* res) {
    bool done = false;
    ALLEGRO_EVENT event;
    int selected = 0;
    const char* difficulties[] = {"ŁATWY", "ŚREDNI", "TRUDNY"};
    ALLEGRO_COLOR colors[] = {COLOR_CORRECT, al_map_rgb(255, 165, 0), COLOR_WRONG};

    while (!done) {
        al_wait_for_event(res->event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            return 1;
        }
        else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            switch (event.keyboard.keycode) {
                case ALLEGRO_KEY_UP:
                    selected = (selected - 1 + 3) % 3;
                    break;
                case ALLEGRO_KEY_DOWN:
                    selected = (selected + 1) % 3;
                    break;
                case ALLEGRO_KEY_ENTER:
                    if (res->click_sound) al_play_sample(res->click_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    return selected + 1;
                case ALLEGRO_KEY_ESCAPE:
                    return 1;
                case ALLEGRO_KEY_F:
                    toggle_fullscreen(res);
                    break;
            }
        }
        else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            float mx = event.mouse.x;
            float my = event.mouse.y;

            for (int i = 0; i < 3; i++) {
                if (is_mouse_over_button(res, mx, my, SCREEN_WIDTH/2 - 200, 200 + i*100, 400, 60)) {
                    if (res->click_sound) al_play_sample(res->click_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    return i + 1;
                }
            }
        }

        if (res->background) {
            al_draw_scaled_bitmap(res->background, 0, 0,
                                al_get_bitmap_width(res->background),
                                al_get_bitmap_height(res->background),
                                0, 0, res->display_width, res->display_height, 0);
        } else {
            al_clear_to_color(COLOR_BACKGROUND);
        }

        draw_text_centered(res, res->font_large, "WYBIERZ POZIOM TRUDNOŚCI", 100, COLOR_GOLD);

        for (int i = 0; i < 3; i++) {
            ALLEGRO_COLOR bg = (i == selected) ? al_map_rgb(colors[i].r+50, colors[i].g+50, colors[i].b+50) : colors[i];
            draw_button(res, res->font_medium, difficulties[i],
                       SCREEN_WIDTH/2 - 200, 200 + i*100, 400, 60,
                       bg, COLOR_TEXT);
        }

        al_flip_display();
    }

    return 1;
}

/**
 * @brief Pobiera nazwę gracza z terminala
 * @param player Wskaźnik na strukturę gracza do wypełnienia
 * @return true jeśli udało się pobrać nazwę, false w przeciwnym wypadku
 */
bool get_player_name_from_terminal(Player* player) {
    printf("Podaj swoja nazwe gracza (maksymalnie %d znakow): ", MAX_LENGTH-1);
    if (fgets(player->name, MAX_LENGTH, stdin) == NULL) {
        return false;
    }

    player->name[strcspn(player->name, "\n")] = '\0';

    if (strlen(player->name) == 0) {
        printf("Nazwa nie moze byc pusta!\n");
        return false;
    }

    int existing = find_player_by_name(player->name);
    if (existing >= 0) {
        printf("Gracz o takiej nazwie juz istnieje!\n");
        return false;
    }

    player->id = get_next_player_id();
    player->wins = 0;
    player->losses = 0;

    all_players[player_count++] = *player;
    save_players();
    return true;
}

/**
 * @brief Wczytuje istniejącego gracza
 * @param player Wskaźnik na strukturę gracza do wypełnienia
 * @param res Wskaźnik na zasoby gry
 * @return true jeśli udało się wczytać gracza, false w przeciwnym wypadku
 */
bool load_existing_player(Player* player, GameResources* res) {
    load_players();

    if (player_count == 0) {
        al_show_native_message_box(res->display, "Brak graczy", "Nie znaleziono zapisanych graczy!", NULL, NULL, ALLEGRO_MESSAGEBOX_ERROR);
        return false;
    }

    int selected = 0;
    bool done = false;
    bool delete_mode = false;

    while (!done) {
        ALLEGRO_EVENT event;
        al_wait_for_event(res->event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            return false;
        }
        else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event.keyboard.keycode == ALLEGRO_KEY_UP) {
                selected = (selected - 1 + player_count) % player_count;
            } else if (event.keyboard.keycode == ALLEGRO_KEY_DOWN) {
                selected = (selected + 1) % player_count;
            } else if (event.keyboard.keycode == ALLEGRO_KEY_ENTER) {
                *player = all_players[selected];
                done = true;
                return true;
            } else if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                done = true;
            } else if (event.keyboard.keycode == ALLEGRO_KEY_DELETE || event.keyboard.keycode == ALLEGRO_KEY_D) {
                delete_mode = true;
            } else if (event.keyboard.keycode == ALLEGRO_KEY_F) {
                toggle_fullscreen(res);
            }
        }
        else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            float mx = event.mouse.x;
            float my = event.mouse.y;

            if (is_mouse_over_button(res, mx, my, SCREEN_WIDTH - 250, 700, 200, 50)) {
                delete_mode = true;
            }
            else {
                for (int i = 0; i < player_count; i++) {
                    if (is_mouse_over_button(res, mx, my, SCREEN_WIDTH/2 - 200, 150 + i * 50 - 25, 400, 50)) {
                        *player = all_players[i];
                        done = true;
                        return true;
                    }
                }
            }
        }

        if (res->background) {
            al_draw_scaled_bitmap(res->background, 0, 0,
                                al_get_bitmap_width(res->background),
                                al_get_bitmap_height(res->background),
                                0, 0, res->display_width, res->display_height, 0);
        } else {
            al_clear_to_color(COLOR_BACKGROUND);
        }

        draw_text_centered(res, res->font_large, "WCZYTAJ GRACZA", 50, COLOR_GOLD);

        for (int i = 0; i < player_count; i++) {
            char player_info[256];
            snprintf(player_info, sizeof(player_info), "%s (W: %d, P: %d",
                    all_players[i].name, all_players[i].wins, all_players[i].losses);

            ALLEGRO_COLOR color = (i == selected) ? COLOR_GOLD : COLOR_TEXT;
            draw_button(res, res->font_medium, player_info,
                       SCREEN_WIDTH/2 - 200, 150 + i * 50 - 25, 400, 50,
                       COLOR_BUTTON, color);
        }

        draw_button(res, res->font_medium, "USUŃ GRACZA (D)",
                   SCREEN_WIDTH - 250, 700, 200, 50,
                   COLOR_DELETE, COLOR_TEXT);

        draw_text_centered(res, res->font_small, "Użyj strzałek lub myszki aby wybrać", 600, al_map_rgb(200, 200, 200));
        draw_text_centered(res, res->font_small, "ENTER aby potwierdzić, ESC aby anulować", 650, al_map_rgb(200, 200, 200));
        draw_text_centered(res, res->font_small, "F - Przełącz tryb pełnoekranowy", 700, al_map_rgb(200, 200, 200));

        al_flip_display();

        if (delete_mode) {
            if (al_show_native_message_box(res->display, "Potwierdzenie",
                "Czy na pewno chcesz usunąć gracza?",
                all_players[selected].name,
                NULL, ALLEGRO_MESSAGEBOX_YES_NO) == 1) {

                for (int i = selected; i < player_count - 1; i++) {
                    all_players[i] = all_players[i + 1];
                }
                player_count--;
                save_players();

                if (player_count == 0) {
                    return false;
                }

                if (selected >= player_count) {
                    selected = player_count - 1;
                }
            }
            delete_mode = false;
        }
    }

    return false;
}

/**
 * @brief Wyświetla ekran wygranej
 * @param res Wskaźnik na zasoby gry
 * @param player Wskaźnik na strukturę gracza
 */
void show_win_screen(GameResources* res, Player* player) {
    bool done = false;
    ALLEGRO_EVENT event;

    if (res->win_sound) {
        al_play_sample(res->win_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
    }

    while (!done) {
        al_wait_for_event(res->event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE ||
            event.type == ALLEGRO_EVENT_KEY_DOWN ||
            event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            done = true;
        }

        if (res->background) {
            al_draw_scaled_bitmap(res->background, 0, 0,
                                al_get_bitmap_width(res->background),
                                al_get_bitmap_height(res->background),
                                0, 0, res->display_width, res->display_height, 0);
        } else {
            al_clear_to_color(COLOR_BACKGROUND);
        }

        draw_text_centered(res, res->font_large, "WYGRANA!", 150, COLOR_GOLD);
        draw_text_centered(res, res->font_medium, "Zdobywasz 1,000,000 zł!", 250, COLOR_TEXT);

        char stats[256];
        snprintf(stats, sizeof(stats), "Gracz: %s | Wygrane: %d | Przegrane: %d",
                player->name, player->wins, player->losses);
        draw_text_centered(res, res->font_medium, stats, 350, COLOR_TEXT);

        draw_text_centered(res, res->font_small, "Naciśnij dowolny przycisk aby kontynuować...", 600, al_map_rgb(200, 200, 200));

        al_flip_display();
    }
}

/**
 * @brief Wyświetla ekran przegranej
 * @param res Wskaźnik na zasoby gry
 * @param player Wskaźnik na strukturę gracza
 */
void show_lose_screen(GameResources* res, Player* player) {
    bool done = false;
    ALLEGRO_EVENT event;

    if (res->wrong_sound) {
        al_play_sample(res->wrong_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
    }

    while (!done) {
        al_wait_for_event(res->event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE ||
            event.type == ALLEGRO_EVENT_KEY_DOWN ||
            event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            done = true;
        }

        if (res->background) {
            al_draw_scaled_bitmap(res->background, 0, 0,
                                al_get_bitmap_width(res->background),
                                al_get_bitmap_height(res->background),
                                0, 0, res->display_width, res->display_height, 0);
        } else {
            al_clear_to_color(COLOR_BACKGROUND);
        }

        draw_text_centered(res, res->font_large, "KONIEC GRY", 150, COLOR_WRONG);
        draw_text_centered(res, res->font_medium, "Niestety, przegrałeś!", 250, COLOR_TEXT);

        char stats[256];
        snprintf(stats, sizeof(stats), "Gracz: %s | Wygrane: %d | Przegrane: %d",
                player->name, player->wins, player->losses);
        draw_text_centered(res, res->font_medium, stats, 350, COLOR_TEXT);

        draw_text_centered(res, res->font_small, "Naciśnij dowolny przycisk aby kontynuować...", 600, al_map_rgb(200, 200, 200));

        al_flip_display();
    }
}

/**
 * @brief Wyświetla pomoc "telefon do przyjaciela"
 * @param res Wskaźnik na zasoby gry
 * @param question Wskaźnik na aktualne pytanie
 */
void show_phone_help(GameResources* res, Question* question) {
    char message[256];
    if (rand() % 100 < 80) {
        snprintf(message, sizeof(message), "Przyjaciel mówi: 'Myślę, że to %s'",
                 question->answers[question->correct]);
    } else {
        strcpy(message, "Przyjaciel mówi: 'Nie jestem pewien...'");
    }

    ALLEGRO_EVENT_QUEUE* temp_queue = al_create_event_queue();
    al_register_event_source(temp_queue, al_get_keyboard_event_source());
    al_register_event_source(temp_queue, al_get_mouse_event_source());

    bool done = false;
    while (!done) {
        ALLEGRO_EVENT event;
        al_wait_for_event(temp_queue, &event);

        if (event.type == ALLEGRO_EVENT_KEY_DOWN ||
            event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            done = true;
        }

        if (res->background) {
            al_draw_scaled_bitmap(res->background, 0, 0,
                                al_get_bitmap_width(res->background),
                                al_get_bitmap_height(res->background),
                                0, 0, res->display_width, res->display_height, 0);
        } else {
            al_clear_to_color(COLOR_BACKGROUND);
        }

        draw_text_centered(res, res->font_large, "TELEFON DO PRZYJACIELA", 100, COLOR_GOLD);
        draw_text_centered(res, res->font_medium, message, 300, COLOR_TEXT);
        draw_text_centered(res, res->font_small, "Naciśnij dowolny przycisk aby kontynuować...", 600, al_map_rgb(200, 200, 200));

        al_flip_display();
    }

    al_destroy_event_queue(temp_queue);
}

/**
 * @brief Rysuje ekran z pytaniem
 * @param res Wskaźnik na zasoby gry
 * @param player Wskaźnik na strukturę gracza
 * @param question Wskaźnik na aktualne pytanie
 * @param hidden Tablica określająca które odpowiedzi są ukryte
 * @param question_num Numer aktualnego pytania
 * @param start_time Czas rozpoczęcia pytania
 */
void draw_question_screen(GameResources* res, Player* player, Question* question, int* hidden,
                         int question_num, time_t start_time) {
    if (res->background) {
        al_draw_scaled_bitmap(res->background, 0, 0,
                            al_get_bitmap_width(res->background),
                            al_get_bitmap_height(res->background),
                            0, 0, res->display_width, res->display_height, 0);
    } else {
        al_clear_to_color(COLOR_BACKGROUND);
    }

    char player_info[256];
    snprintf(player_info, sizeof(player_info), "Gracz: %s | Pytanie %d za %s",
             player->name, question_num + 1, rewards[question_num]);
    al_draw_text(res->font_small, COLOR_TEXT, scale_x(res, 50), scale_y(res, 50), 0, player_info);

    time_t now = time(NULL);
    int time_remaining = TIME_LIMIT - (int)difftime(now, start_time);
    char time_str[32];
    snprintf(time_str, sizeof(time_str), "Czas: %ds", time_remaining);
    al_draw_text(res->font_small, COLOR_TEXT, res->display_width - scale_x(res, 150), scale_y(res, 50), 0, time_str);

    draw_text_centered(res, res->font_medium, question->text, 150, COLOR_TEXT);

    for (int i = 0; i < 4; i++) {
        if (!hidden[i]) {
            char answer_text[256];
            snprintf(answer_text, sizeof(answer_text), "%c) %s", 'A' + i, question->answers[i]);

            ALLEGRO_COLOR bg;
            if (i == 0) bg = al_map_rgb(0, 100, 200);
            else if (i == 1) bg = al_map_rgb(200, 0, 0);
            else if (i == 2) bg = al_map_rgb(0, 200, 0);
            else bg = al_map_rgb(200, 200, 0);

            draw_button(res, res->font_small, answer_text,
                       SCREEN_WIDTH/2 - 300, 250 + i*100, 600, 80,
                       bg, COLOR_TEXT);
        }
    }

    draw_text_centered(res, res->font_small, "[T] Telefon do przyjaciela", SCREEN_HEIGHT - 100, COLOR_TEXT);
    draw_text_centered(res, res->font_small, "[5] 50:50", SCREEN_HEIGHT - 70, COLOR_TEXT);
    draw_text_centered(res, res->font_small, "[F] Przełącz tryb pełnoekranowy", SCREEN_HEIGHT - 40, COLOR_TEXT);

    al_flip_display();
}

/**
 * @brief Główna funkcja gry
 * @param res Wskaźnik na zasoby gry
 * @param player Wskaźnik na strukturę gracza
 * @param questions Tablica pytań
 * @param total_questions Całkowita liczba pytań
 */
void play_game(GameResources* res, Player* player, Question* questions, int total_questions) {
    int used_help = 0;
    int selected_questions[SELECTED_QUESTIONS] = {0};
    int selected_count = 0;

    // Losowanie unikalnych pytań
    while (selected_count < SELECTED_QUESTIONS) {
        int r = rand() % total_questions;
        int duplicate = 0;

        for (int i = 0; i < selected_count; i++) {
            if (selected_questions[i] == r) {
                duplicate = 1;
                break;
            }
        }

        if (!duplicate) {
            selected_questions[selected_count++] = r;
        }
    }

    // Główna pętla gry
    for (int i = 0; i < SELECTED_QUESTIONS; i++) {
        Question* current = &questions[selected_questions[i]];
        int hidden[4] = {0};
        int phone_used = 0;
        int fifty_used = 0;
        bool answered = false;
        time_t start_time = time(NULL);

        while (!answered) {
            ALLEGRO_EVENT event;
            al_wait_for_event(res->event_queue, &event);

            if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                return;
            }

            if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
                if (res->click_sound) {
                    al_play_sample(res->click_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                }

                if (event.keyboard.keycode == ALLEGRO_KEY_A && !hidden[0]) {
                    answered = (0 == current->correct);
                    if (!answered) {
                        if (res->wrong_sound) al_play_sample(res->wrong_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                        player->losses++;
                        show_lose_screen(res, player);
                        return;
                    } else {
                        if (res->correct_sound) al_play_sample(res->correct_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    }
                }
                else if (event.keyboard.keycode == ALLEGRO_KEY_B && !hidden[1]) {
                    answered = (1 == current->correct);
                    if (!answered) {
                        if (res->wrong_sound) al_play_sample(res->wrong_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                        player->losses++;
                        show_lose_screen(res, player);
                        return;
                    } else {
                        if (res->correct_sound) al_play_sample(res->correct_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    }
                }
                else if (event.keyboard.keycode == ALLEGRO_KEY_C && !hidden[2]) {
                    answered = (2 == current->correct);
                    if (!answered) {
                        if (res->wrong_sound) al_play_sample(res->wrong_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                        player->losses++;
                        show_lose_screen(res, player);
                        return;
                    } else {
                        if (res->correct_sound) al_play_sample(res->correct_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    }
                }
                else if (event.keyboard.keycode == ALLEGRO_KEY_D && !hidden[3]) {
                    answered = (3 == current->correct);
                    if (!answered) {
                        if (res->wrong_sound) al_play_sample(res->wrong_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                        player->losses++;
                        show_lose_screen(res, player);
                        return;
                    } else {
                        if (res->correct_sound) al_play_sample(res->correct_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    }
                }
                else if (event.keyboard.keycode == ALLEGRO_KEY_T && !phone_used && used_help < HELP_LIMIT) {
                    show_phone_help(res, current);
                    phone_used = 1;
                    used_help++;
                }
                else if (event.keyboard.keycode == ALLEGRO_KEY_5 && !fifty_used && used_help < HELP_LIMIT) {
                    fifty_used = 1;
                    used_help++;

                    int removed = 0;
                    while (removed < 2) {
                        int r = rand() % 4;
                        if (r != current->correct && !hidden[r]) {
                            hidden[r] = 1;
                            removed++;
                        }
                    }
                }
                else if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    return;
                }
                else if (event.keyboard.keycode == ALLEGRO_KEY_F) {
                    toggle_fullscreen(res);
                }
            }
            else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
                float mx = event.mouse.x;
                float my = event.mouse.y;

                for (int j = 0; j < 4; j++) {
                    if (!hidden[j] && is_mouse_over_button(res, mx, my, SCREEN_WIDTH/2 - 300, 250 + j*100, 600, 80)) {
                        answered = (j == current->correct);
                        if (!answered) {
                            if (res->wrong_sound) al_play_sample(res->wrong_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                            player->losses++;
                            show_lose_screen(res, player);
                            return;
                        } else {
                            if (res->correct_sound) al_play_sample(res->correct_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                        }
                        break;
                    }
                }
            }

            draw_question_screen(res, player, current, hidden, i, start_time);

            if (difftime(time(NULL), start_time) >= TIME_LIMIT) {
                if (res->wrong_sound) al_play_sample(res->wrong_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                player->losses++;
                show_lose_screen(res, player);
                return;
            }
        }
    }

    if (res->win_sound) {
        al_play_sample(res->win_sound, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
    }
    player->wins++;
    show_win_screen(res, player);
}
/** @} */

/**
 * @brief Funkcja główna programu
 * @return Kod wyjścia (0 - sukces)
 */
int main() {
    srand(time(NULL));
    GameResources resources;
    Player current_player;

    if (!init_resources(&resources)) {
        al_show_native_message_box(NULL, "Błąd", "Nie można zainicjować gry!",
                                  "Sprawdź czy masz zainstalowane wszystkie wymagane biblioteki.",
                                  NULL, ALLEGRO_MESSAGEBOX_ERROR);
        return -1;
    }

    load_players();
    show_welcome_screen(&resources);

    bool running = true;
    while (running) {
        int menu_result = show_main_menu(&resources);

        if (menu_result == 0) {
            running = false;
        } else {
            bool player_selected = false;

            if (menu_result == 1) {
                printf("Wprowadzanie nazwy gracza w terminalu...\n");
                player_selected = get_player_name_from_terminal(&current_player);
                if (player_selected) {
                    printf("Witaj, %s! Powodzenia w grze!\n", current_player.name);
                } else {
                    printf("Nie udalo sie utworzyc gracza. Sprobuj ponownie.\n");
                }
            }
            else if (menu_result == 2) {
                player_selected = load_existing_player(&current_player, &resources);
            }

            if (player_selected) {
                int difficulty = show_difficulty_menu(&resources);
                const char* question_file;

                switch (difficulty) {
                    case 1: question_file = "latwe.txt"; break;
                    case 2: question_file = "srednie.txt"; break;
                    case 3: question_file = "trudne.txt"; break;
                    default: question_file = "latwe.txt"; break;
                }

                int question_count = 0;
                Question* questions = load_questions(question_file, &question_count);

                if (questions && question_count > 0) {
                    play_game(&resources, &current_player, questions, question_count);

                    int idx = find_player_by_name(current_player.name);
                    if (idx >= 0) {
                        all_players[idx] = current_player;
                    } else {
                        all_players[player_count++] = current_player;
                    }
                    save_players();
                } else {
                    al_show_native_message_box(resources.display, "Błąd",
                                              "Nie udało się załadować pytań!",
                                              "Sprawdź czy plik z pytaniami istnieje.",
                                              NULL, ALLEGRO_MESSAGEBOX_ERROR);
                }

                if (questions) {
                    free(questions);
                }
            }
        }
    }

    cleanup_resources(&resources);
    return 0;
}