#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __unix__
# include <unistd.h>
#elif defined _WIN32
# include <windows.h>
#define sleep(x) Sleep(1000 * (x))
#endif

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) > (b) ? (b) : (a))

// define
#define MAX_STATI 200
#define MAX_LEN_STATO 10
#define LEN_NASTRO 999
#define MAX_LINEA_DESC 200
#define MAX_NOME_FILE 25
#define MAX_TRANSIZIONI 250
#define BORDERS 1
#define SEC_BETWEEN_TRANSITION 0

typedef struct transizione {
    char stato_corrente[MAX_LEN_STATO];
    char char_corrente;
    char stato_next[MAX_LEN_STATO];
    char char_nuovo;
    int spostamento;
} transizione;

char stati[MAX_STATI][MAX_LEN_STATO];
char stati_finali[MAX_STATI][MAX_LEN_STATO];
char stato_iniziale[MAX_LEN_STATO];
char nastro[LEN_NASTRO];
char stato_corrente[MAX_LEN_STATO];

int num_stati = 0;
int num_stati_finali = 0;
int num_transizioni = 0;
int posizione_testina = 0;
transizione transizioni[MAX_TRANSIZIONI];
size_t beg;
size_t end;

void stampa_stato() {
    for (int i = 0; i < num_stati; i++) {
        if (strcmp(stato_corrente, stati[i]) == 0) {
            printf("\033[39;34m%s\033[0m ", stati[i]);
        } else {
            printf("%s ", stati[i]);
        }
    }
    printf("\n");
    fflush(stdout);
}

void inizio_fine_nastro() {
    int _beg = 0;
    while (_beg < LEN_NASTRO && nastro[_beg + BORDERS] == '_'
           && _beg + BORDERS != posizione_testina) {
        _beg++;
    }
    beg = MIN(beg, _beg);

    int _end = LEN_NASTRO - 1;
    while (_end > 0 && nastro[_end - BORDERS] == '_'
           && _end - BORDERS != posizione_testina) {
        _end--;
    }
    end = MAX(end, _end);
}


void stampa_nastro() {
    inizio_fine_nastro();

    for (size_t i = beg; i <= end; i++) {
        if (i == posizione_testina) {
            printf("\033[45;41m%c\033[0m", nastro[i]);
        } else {
            printf("%c", nastro[i]);
        }
    }
    printf("\n");
    fflush(stdout);
}

void parse_states(char *line, char target_array[MAX_STATI][MAX_LEN_STATO], int *count) {
    int k = 0;
    char *token = strtok(line, ",\r\n");
    while (token != NULL && k < MAX_STATI) {
        strncpy(target_array[k], token, MAX_LEN_STATO - 1);
        target_array[k][MAX_LEN_STATO - 1] = '\0'; // Safe termination
        k++;
        token = strtok(NULL, ",\r\n");
    }
    *count = k;
}

void inizializza_mt(const char nome_file[]) {
    char filename[MAX_NOME_FILE + 10];
    snprintf(filename, sizeof(filename), "%s_mt.txt", nome_file);

    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "Errore apertura %s\n", filename);
        exit(-1);
    }

    char line[MAX_LINEA_DESC];

    // 1. Lettura tutti gli stati
    if (fgets(line, sizeof(line), f)) {
        //rimuovi spazi
        parse_states(line, stati, &num_stati);
    }

    // 2. Lettura stato iniziale
    if (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';
        strncpy(stato_iniziale, line, MAX_LEN_STATO - 1);
        stato_iniziale[MAX_LEN_STATO - 1] = '\0';
        strcpy(stato_corrente, stato_iniziale);
    }

    // 3. Lettura stati finali
    if (fgets(line, sizeof(line), f)) {
        parse_states(line, stati_finali, &num_stati_finali);
    }

    // 4. Lettura transizioni
    int k = 0;
    while (fgets(line, sizeof(line), f) != NULL && k < MAX_TRANSIZIONI) {
        // Rimuovi spazi bianchi fastidiosi (compresi quelli tra virgole)
        int non_space = 0;
        for (int i = 0; line[i] != '\0'; i++) {
            if (line[i] != ' ' && line[i] != '\t') {
                line[non_space++] = line[i];
            }
        }
        line[non_space] = '\0';

        // Salta righe vuote
        if (line[0] == '\0' || line[0] == '\n' || line[0] == '\r') continue;

        // Trova il separatore "->"
        char *arrow = strstr(line, "->");
        if (arrow == NULL) continue; // Salta righe malformate

        // Spezza la stringa in due parti temporanee: sinistra e destra della freccia
        *arrow = '\0';
        char *sinistra = line; // Contiene "stato_corrente,char_corrente"
        char *destra = arrow + 2; // Contiene "stato_next,char_nuovo,spostamento"

        char st_curr[MAX_LEN_STATO] = {0};
        char st_next[MAX_LEN_STATO] = {0};
        char dir;

        // Estrai dalla parte sinistra
        int p_sinistra = sscanf(sinistra, "%[^,],%c", st_curr, &transizioni[k].char_corrente);

        // Estrai dalla parte destra
        int p_destra = sscanf(destra, "%c,%[^,],%c", &transizioni[k].char_nuovo, st_next, &dir);

        if (p_sinistra == 2 && p_destra == 3) {
            strncpy(transizioni[k].stato_corrente, st_curr, MAX_LEN_STATO - 1);
            transizioni[k].stato_corrente[MAX_LEN_STATO - 1] = '\0';

            strncpy(transizioni[k].stato_next, st_next, MAX_LEN_STATO - 1);
            transizioni[k].stato_next[MAX_LEN_STATO - 1] = '\0';

            if (dir == '>') transizioni[k].spostamento = 1;
            else if (dir == '<') transizioni[k].spostamento = -1;
            else transizioni[k].spostamento = 0;

            k++;
        }
    }
    num_transizioni = k;
    fclose(f);
}

int accetta() {
    for (int i = 0; i < num_stati_finali; i++) {
        if (strcmp(stati_finali[i], stato_corrente) == 0)
            return 1;
    }
    return 0;
}

int termina() {
    int count = 0;
    for (int i = 0; i < num_transizioni; i++) {
        if (strcmp(transizioni[i].stato_corrente, stato_corrente) == 0
            && nastro[posizione_testina] == transizioni[i].char_corrente) {
            count++;
        }
    }
    return count == 0;
}

void transition() {
    char current_char = nastro[posizione_testina];
    for (int i = 0; i < num_transizioni; i++) {
        if (transizioni[i].char_corrente == current_char &&
            strcmp(transizioni[i].stato_corrente, stato_corrente) == 0) {
            nastro[posizione_testina] = transizioni[i].char_nuovo;
            strcpy(stato_corrente, transizioni[i].stato_next);
            posizione_testina += transizioni[i].spostamento;
            if (posizione_testina >= LEN_NASTRO) {
                fprintf(stderr, "Errore: Tentativo di accesso alla posizione %d, nastro di lunghezza: %d.\n",
                        posizione_testina, LEN_NASTRO);
                exit(-1);
            }
            return;
        }
    }
}

int main() {
    printf("La macchina di Turing\n\n");

    char nomefile[MAX_NOME_FILE];
    printf("Inserire nome file: "); //inserire ciao o scambio
    fgets(nomefile, MAX_NOME_FILE, stdin);
    nomefile[strlen(nomefile) - 1] = '\0';

    inizializza_mt(nomefile);

    char filename_in[MAX_NOME_FILE + 12];
    char filename_out[MAX_NOME_FILE + 13];
    snprintf(filename_in, sizeof(filename_in), "%s_input.txt", nomefile);
    snprintf(filename_out, sizeof(filename_out), "%s_output.txt", nomefile);

    FILE *f_in = fopen(filename_in, "r");
    FILE *f_out = fopen(filename_out, "w"); // File nuovo e pulito

    if (f_in == NULL || f_out == NULL) {
        fprintf(stderr, "Errore apertura file\n");
        exit(-1);
    }

    char line[LEN_NASTRO];

    while (fgets(line, sizeof(line), f_in)) {
        // verificare con input vuoto...
        memset(nastro, '_', LEN_NASTRO);
        beg = LEN_NASTRO - 1;
        end = 0;
        // Troviamo dove finisce la stringa pulita escludendo \r e \n
        size_t len = strcspn(line, "\r\n");
        line[len] = '\0';
        if (len >= LEN_NASTRO) {
            fprintf(stderr, "Tentativo di scrivere input di lunghezza: %lu su un nastro di lunghezza: %d\n", len - 1,
                    LEN_NASTRO);
            exit(-1);
        }

        // Inizializziamo la testina e copiamo l'input sul nastro
        posizione_testina = (LEN_NASTRO / 2) - (len / 2);
        memcpy(&nastro[posizione_testina], line, len);
        printf("Input: %s\n", line);

        // Ripristiniamo lo stato iniziale della macchina prima di calcolare
        strcpy(stato_corrente, stato_iniziale);

        // Esecuzione della Macchina di Turing
        while (!termina()) {
            stampa_stato();
            stampa_nastro();
            transition();
            printf("\n");
            sleep(SEC_BETWEEN_TRANSITION);
        }

        stampa_stato();
        stampa_nastro();

        // Isoli l'input eliminando il \n
        line[len] = '\0';

        // Scrivi l'input iniziale + verdetto nel file di OUTPUT
        char verdetto = accetta() ? 'A' : 'R';

        char dest[LEN_NASTRO + 1];
        size_t b = 0, e = LEN_NASTRO - 1;
        while (nastro[b] == '_') b++;
        while (nastro[e] == '_' && e>0) e--;
        if (b > e) b = e = posizione_testina;
        strncpy(dest, nastro + b, e-b+1);
        dest[e - b + 1] = '\0';
        fprintf(f_out, "%s %s %c\n", line, dest, verdetto);
        fflush(f_out);
        printf("\nEsito: %c\n", verdetto);
        printf("******************\n\n");
        sleep(SEC_BETWEEN_TRANSITION + 1);
    }

    fclose(f_in);
    fclose(f_out);

    return 0;
}
