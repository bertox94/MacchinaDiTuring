#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_STATI 200
#define MAX_LEN_STATO 10
#define LEN_NASTRO 999
#define MAX_LINEA_DESC 200
#define MAX_NOME_FILE 25
#define MAX_TRANSIZIONI 250

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct transizione {
    char stato_corrente[MAX_LEN_STATO];
    char char_corrente;
    char stato_next[MAX_LEN_STATO];
    char char_nuovo;
    int spostamento;
} transizione, *t;

char stati[MAX_STATI][MAX_LEN_STATO];
char stati_finali[MAX_STATI][MAX_LEN_STATO];
char stato_iniziale[MAX_LEN_STATO];
char nastro[LEN_NASTRO];
char stato_corrente[MAX_LEN_STATO];
int num_stati;
int num_stati_finali;
int num_transizioni;
int posizione_testina = 0;
transizione transizioni[MAX_TRANSIZIONI];

void stampa_stato() {
    for (int i = 0; i < num_stati; i++) {
        if (!strcmp(stato_corrente, stati[i])) {
            printf("\033[39;34m%s\033[0m ", stati[i]);
        } else
            printf("%s ", stati[i]);
    }
    printf("\n");
    fflush(stdout);
}

void stampa_nastro() {
    int beg = 0, end = LEN_NASTRO - 1;
    for (; beg < LEN_NASTRO && nastro[beg] == '_'; beg++) {
    }
    beg = MAX(0, beg - 2);
    for (; end >= 0 && nastro[end] == '_'; end--) {
    }
    end = MIN(end + 2, LEN_NASTRO - 1);

    for (; beg <= end; beg++) {
        if (beg == posizione_testina)
            printf("\033[45;41m%c\033[0m", nastro[beg]);
        else
            printf("%c", nastro[beg]);
    }
    printf("\n");
    fflush(stdout);
}

void inizializza_mt(char nome_file[]) {
    char filename[MAX_NOME_FILE + strlen("_mt.txt")] = "";
    strcat(filename, nome_file);
    strcat(filename, "_mt.txt");
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "Errore apertura _mt.txt\n");
        exit(-1);
    }

    char line[MAX_LINEA_DESC];

    // Lettura stati
    fgets(line, sizeof(line), f);
    line[strcspn(line, "\r\n")] = 0;

    int j = 0, k = 0;
    for (int i = 0; i < strlen(line); i++) {
        for (j = i; j < strlen(line) && line[j] != ','; j++) {
        }
        strncpy(stati[k], line+i, j-i);
        i = j;
        k++;
    }

    num_stati = k;

    //Lettura stato iniziale
    fgets(line, sizeof(line), f);
    line[strcspn(line, "\r\n")] = 0;
    strcpy(stato_iniziale, line);
    strcpy(stato_corrente, line);

    //Lettura stato finale
    fgets(line, sizeof(line), f);
    line[strcspn(line, "\r\n")] = 0;
    k = 0;
    for (int i = 0; i < strlen(line); i++) {
        for (j = i; j < strlen(line) && line[j] != ','; j++) {
        }
        strncpy(stati_finali[k], line+i, j-i);
        i = j;
        k++;
    }
    num_stati_finali = k;

    k = 0;
    while (fgets(line, sizeof(line), f) != NULL) {
        line[strcspn(line, "\r\n")] = 0;
        //remove whitespace
        int non_space_count = 0;
        for (int i = 0; line[i] != '\0'; i++) {
            if (line[i] != ' ') {
                line[non_space_count] = line[i];
                non_space_count++; //non_space_count incremented
            }
        }

        // verify the transition is syntactically correct

        int pos = (int) (strchr(line, ',') - line);
        strncpy(transizioni[k].stato_corrente, line, pos);
        memmove(line, line+(pos+1), strlen(line)-(pos+1));
        transizioni[k].char_corrente = line[0];
        memmove(line, line+3, strlen(line)-3);
        //next state
        pos = (int) (strchr(line, ',') - line);
        strncpy(transizioni[k].stato_next, line, pos);
        memmove(line, line+pos+1, strlen(line)-(pos+1));
        transizioni[k].char_nuovo = line[0];
        if (line[2] == '>')
            transizioni[k].spostamento = 1;
        else if (line[2] == '<')
            transizioni[k].spostamento = -1;
        else
            transizioni[k].spostamento = 0;
        k++;
    }
    num_transizioni = k;

    fclose(f);
}

void inizializza_nastro(char nome_file[]) {
    memset(nastro, '_', LEN_NASTRO);

    char filename[MAX_NOME_FILE + strlen("_input.txt")] = "";
    strcat(filename, nome_file);
    strcat(filename, "_input.txt");
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "Errore apertura _input.txt\n");
        exit(-1);
    }

    char line[LEN_NASTRO];

    // Lettura stati
    fgets(line, sizeof(line), f);
    line[strcspn(line, "\r\n")] = 0;

    posizione_testina = LEN_NASTRO / 2 - strlen(line) / 2;

    for (int i = 0; i < strlen(line); i++) {
        nastro[LEN_NASTRO / 2 - strlen(line) / 2 + i] = line[i];
    }
}

int accetta() {
    for (int i = 0; i < num_stati_finali; i++) {
        if (!strcmp(stati_finali[i], stato_corrente))
            return 1;
    }
    return 0;
}

void transition() {
    for (int i = 0; i < num_transizioni; i++) {
        if (strcmp(transizioni[i].stato_corrente, stato_corrente) == 0
            && transizioni[i].char_corrente == nastro[posizione_testina]) {
            nastro[posizione_testina] = transizioni[i].char_nuovo;
            strcpy(stato_corrente, transizioni[i].stato_next);
            posizione_testina += transizioni[i].spostamento;
            return;
        }
    }
    fprintf(stderr, "Non è stat trovata una transizione adeguata.");
    exit(-1);
}


int main() {
    printf("La macchina di Turing\n\n");

    char nomefile[MAX_NOME_FILE] = "ciao";
    //printf("Inserire nome file: ");
    //fgets(nomefile, sizeof(nomefile), stdin);
    //nomefile[strcspn(nomefile, "\r\n")] = 0;

    inizializza_mt(nomefile);
    inizializza_nastro(nomefile);
    while (!accetta()) {
        stampa_stato();
        stampa_nastro();
        transition();
        printf("\n");
        sleep(2);
    }
    stampa_stato();
    stampa_nastro();

    return 0;
}
