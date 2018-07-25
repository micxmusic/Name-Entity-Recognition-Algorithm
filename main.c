/* COMP10002 Assignment 2
 * Named Entity Recognition Algorithm
 * Written by Michelle Anggana (931735), May 2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

/* constants and type definitions */
#define DIVIDER   "=========================" /* line break for stages */
#define MAX_CH      31             /* max characters in a name with null byte */
#define MAX_NAME    100            /* max number of names in a dictionary */
#define CH_NULL     '\0'           /* null byte*/
#define DIC_END     '%'            /* indicator of end break of dictionary */
#define NOT_FOUND   (-1)           /* return value if value not found */
#define FIRST_N     0              /* integer value representing first name */
#define LAST_N      1              /* integer value representing last name */
#define NOT_N       2              /* integer value representing not name */
#define SENTINEL    (-1)           /* integer value to prevent swap */

typedef struct {
    char name[MAX_CH];
    int prob[3];
} name_t;

typedef struct {
    name_t list[MAX_NAME];
    int word_count;
    int char_count;
} dictionary_t;

typedef struct node node_t;

struct node {
    char data[MAX_CH];
    int *probs;
    node_t *next;
};

typedef struct {
    int type;
    int prob;
} class_t;
/****************************************************************/

/* function prototypes */
void print_stage(int stage);
void print_name(int pos, dictionary_t *dict);
void fill(dictionary_t *dict);
dictionary_t *new_dict();
node_t *new_node();
int get_word(node_t *curr, dictionary_t *dict);
int cmp_word(char *word, name_t *target);
int binary_search(dictionary_t *dict, int lo, int hi, char *key);
void print_sentence(node_t *curr);
void free_list(node_t *head);
void classify_two(int *probability, int name_type1, int name_type2,
                  class_t *class);
void classify_three(int *probability, class_t *class);
void swap(class_t *name_info1, class_t *name_info2);
void selective_swap(class_t **classify, int i);
class_t **classifier(node_t *curr, int size);
void identify_type(class_t **classify, int size);
void print_type(node_t *curr, class_t **classify);
void free_classifier(class_t **classify, int size);

/****************************************************************/

int main(int argc, char **argv) {

    /* STAGE 1 */
    print_stage(1);
    dictionary_t *dict = new_dict();
    fill(dict);
    print_name(0, dict);


    /* STAGE 2 */
    print_stage(2);
    printf("Number of names: %d\n", dict->word_count);
    printf("Average number of characters per name: %.2f\n",
           (double) dict->char_count / dict->word_count);

    /* STAGE 3 */
    print_stage(3);
    node_t *head = new_node();
    node_t *curr = head;
    int size = 1;
    while (get_word(curr, dict)) {
        curr->next = new_node();
        curr = curr->next;
        size++;
    }

    /* STAGE 4 */
    print_stage(4);
    curr = head;
    print_sentence(curr);

    /* STAGE 5 */
    print_stage(5);
    curr = head;
    class_t **classify = classifier(curr, size);
    identify_type(classify, size);
    print_type(curr, classify);

    free_classifier(classify, size);
    free_list(head);
    free(dict);
    return 0;
}


/* function to print stage breaks */
void
print_stage(int stage) {
    if (stage > 1) {
        printf("\n");
    }
    printf("%sStage %d%s\n", DIVIDER, stage, DIVIDER);
}

/* print name and probability of a particular position in dictionary */
void
print_name(int pos, dictionary_t *dict) {
    printf("Name %d: %s\n", pos, dict->list[pos].name);
    printf("Label probabilities: %d%% %d%% %d%%\n",
           dict->list[pos].prob[FIRST_N], dict->list[pos].prob[LAST_N],
           dict->list[pos].prob[NOT_N]);
}

/* filling up the dictionary struct with all the names and corresponding
 * probabilities
 */
void
fill(dictionary_t *dict) {
    char character;
    int i = 0, j = 0;
    while (1) {
        /* check for end of word */
        while ((character = getchar()) != '\n') {
            /* if end of dictionary is reached, consume all % and return */
            if (character == DIC_END) {
                while (getchar() != '\n');
                return;
            }
            /* removes # and stores the name in the list contained in the
             * dictionary struct, as well as increase the character count
             */
            if (character != '#') {
                dict->list[j].name[i] = character;
                dict->char_count++;
                i++;
            }
        }
        /* null terminating the name at the end */
        dict->list[j].name[i] = CH_NULL;

        /* scanning the probabilities of the corresponding name into the list
         * in the dictionary struct, then increase the word count
         */
        scanf("%d %d %d\n", &dict->list[j].prob[FIRST_N],
              &dict->list[j].prob[LAST_N],
              &dict->list[j].prob[NOT_N]);
        dict->word_count++;

        i = 0;
        j++;
    }
}

/* initialize a new dictionary struct */
dictionary_t *
new_dict() {
    dictionary_t *dict = (dictionary_t *) malloc(sizeof(dictionary_t));
    assert(dict);
    dict->word_count = 0;
    dict->char_count = 0;
    return dict;
}

/* initialize a new node struct */
node_t *
new_node() {
    node_t *node = (node_t *) malloc(sizeof(node_t));
    assert(node);
    node->probs = NULL;
    node->next = NULL;
    return node;
}

/* get a word from the sentence and stores it in a node, then prints it and
 * return if the last word has been retrieved
 */
int
get_word(node_t *curr, dictionary_t *dict) {
    char character;
    int i = 0, flag = 0, loc;
    while ((character = getchar()) != EOF) {
        /* insert character while still in a word */
        if (isalpha(character)) {
            curr->data[i] = character;
            i++;
            /* if a space is read, return that the sentence has not ended and
             * break the loop
             */
        } else if (character == ' ') {
            flag = 1;
            break;
        }
    }
    /* null terminate the word read */
    curr->data[i] = CH_NULL;

    /* checks if the word inserted exists in the dictionary, and points loc to
     * the corresponding probabilities if it exists
     */
    if ((loc = binary_search(dict, 0, dict->word_count, curr->data))
            != NOT_FOUND) {
        curr->probs = dict->list[loc].prob;
    }

    /* printing the word that had just been stored */
    printf("%s\n", curr->data);
    return flag;
}

/* compare two words and returns if it is equal, larger or smaller */
int
cmp_word(char *word, name_t *target) {
    int i;

    /* looks for the longer word and stores the length in upper */
    int upper = (strlen(word) < strlen(target->name)) ?
                strlen(word) : strlen(target->name);

    /* checking for whether word is less than, more than, or equal to target */
    for (i = 0; i < upper; i++) {
        if (word[i] < target->name[i]) {
            return -1;
        } else if (word[i] > target->name[i]) {
            return 1;
        }
    }
    return 0;
}

/* adapted from Alistair Moffat's binarysearch.c found in lecture 5 slide 28
 * binary search function that searches a dictionary_t struct
 */
int
binary_search(dictionary_t *dict, int lo, int hi, char *key) {
    /* if key is in dict, it is between dict[lo] and dict[hi-1] */
    int mid, outcome;
    if (lo >= hi) {
        return NOT_FOUND;
    }
    mid = (lo + hi) / 2;
    /* if key is less than the root search the left branch */
    if ((outcome = cmp_word(key, dict->list + mid)) < 0) {
        return binary_search(dict, lo, mid, key);
    /* if key is more than the root search the right branch */
    } else if (outcome > 0) {
        return binary_search(dict, mid + 1, hi, key);
    /* return position if found */
    } else {
        return mid;

    }

}

/* print the sentence with corresponding possible labels */
void
print_sentence(node_t *curr) {
    /* stepping through the linked list until NULL is found */
    while (curr) {
        printf("%-32s", curr->data);
        /* checks the probs pointer for a value, if a value is not found then
         * print NOT_NAME, otherwise print whether word is a possible FIRST_NAME
         * or LAST_NAME
         */
        if (curr->probs) {
            if (curr->probs[FIRST_N] && curr->probs[LAST_N]) {
                printf("FIRST_NAME, LAST_NAME\n");
            } else if (curr->probs[FIRST_N]) {
                printf("FIRST_NAME\n");
            } else if (curr->probs[LAST_N]) {
                printf("LAST_NAME\n");
            }

        } else {
            printf("NOT_NAME\n");
        }
        curr = curr->next;
    }
}

/* frees the linked list */
void
free_list(node_t *head) {
    node_t *next;
    while (head) {
        next = head->next;
        free(head);
        head = next;
    }
}

/* sort names into two given types or sentinel according to probability */
void
classify_two(int *probability, int name_type1, int name_type2, class_t *class){
    /* classification based on probability value */
    if (probability[1] > probability[2]) {
        class[1].type = name_type1;
        class[1].prob = probability[name_type1];
        class[2].type = name_type2;
        class[2].prob = probability[name_type2];
    } else {
        class[1].type = name_type2;
        class[1].prob = probability[name_type2];
        class[2].type = name_type1;
        class[2].prob = probability[name_type1];
    }

    /* check for 0 probability and set type to sentinel */
    if (!probability[1] && !probability[2]) {
        class[1].type = SENTINEL;
        class[2].type = SENTINEL;
    } else if (!probability[1] || !probability[2]) {
        class[2].type = SENTINEL;
    }
}

/* sort names into the three defined types according to descending order of
 * probability
 */
void
classify_three(int *probability, class_t *class){
    /* case where first name has highest probability*/
    if (probability[FIRST_N] > probability[LAST_N] &&
        probability[FIRST_N] > probability[NOT_N]) {
        class[0].type = FIRST_N;
        class[0].prob = probability[FIRST_N];
        classify_two(probability, LAST_N, NOT_N, class);

    /* case where last name has the highest probability */
    } else if (probability[LAST_N] > probability[FIRST_N] &&
               probability[LAST_N] > probability[NOT_N]) {
        class[0].type = LAST_N;
        class[0].prob = probability[LAST_N];
        classify_two(probability, FIRST_N, NOT_N, class);

     /* case where not name has the highest probability */
    } else if (probability[NOT_N] > probability[FIRST_N] &&
               probability[NOT_N] > probability[LAST_N]) {
        class[0].type = NOT_N;
        class[0].prob = probability[NOT_N];
        classify_two(probability, LAST_N, FIRST_N, class);
    }
}

/* function to swap two class_t values */
void
swap(class_t *name_info1, class_t *name_info2){
    class_t tmp;
    tmp = *name_info1;
    *name_info1 = *name_info2;
    *name_info2 = tmp;
}

/* decision making function to possibly swap assigned name labels with the next
 * most probable type
 */
void
selective_swap(class_t **classify, int i){
    /* case where both second ranked types have a probability larger than 0,
     * swap the type of the word that has the higher probability of being the
     * second ranked type
     */
    if (classify[i][1].type != SENTINEL && classify[i + 1][1].type != SENTINEL){
        if(classify[i][1].prob > classify[i + 1][1].prob){
            swap(&classify[i][0], &classify[i][1]);
        } else if (classify[i][1].prob < classify[i + 1][1].prob){
            swap(&classify[i + 1][0], &classify[i + 1][1]);
        }
    /* if only one of the second ranked type has a probability larger than 0,
     * swap the type of the word with a valid second ranked type
     */
    } else if(classify[i+1][1].type == SENTINEL) {
        swap(&classify[i][0], &classify[i][1]);
    } else if (classify[i][1].type == SENTINEL) {
        swap(&classify[i + 1][0], &classify[i + 1][1]);
    }
}

/* initialize a classifier array and fills it with corresponding values */
class_t **
classifier(node_t *curr, int size){
    int i= 0;
    class_t **classify = (class_t **)malloc(sizeof(class_t*)*size);
    /* for each word, malloc 3 class_t and fill with probability and type */
    for (i=0; i<size; i++){
        classify[i] = (class_t *)malloc(sizeof(class_t)*3);
        if (curr->probs) {
            classify_three(curr->probs, classify[i]);
        } else {
            classify[i][0].type = NOT_N;
        }
        curr = curr->next;
    }
    return classify;
}

/* decision making function to swap certain name labels when it repeats */
void
identify_type(class_t **classify, int size){
    int i= 0;
    for (i = 0; i < size - 1; i++) {
        /* check for repeated type */
        if (classify[i][0].type == classify[i + 1][0].type) {
            /* do nothing if a not name type is repeated */
            if (classify[i][0].type == NOT_N) {
                continue;
            } else {
                selective_swap(classify, i);
            }
        }
    }
}

/* prints the sentence with single labels */
void
print_type(node_t *curr, class_t **classify){
    int i = 0;
    while (curr) {
        printf("%-32s", curr->data);
        if(classify[i][0].type == FIRST_N){
            printf("FIRST_NAME\n");
        } else if(classify[i][0].type == LAST_N){
            printf("LAST_NAME\n");
        } else {
            printf("NOT_NAME\n");
        }
        curr = curr->next;
        i++;
    }
}

/* frees the classifier */
void
free_classifier(class_t **classify, int size) {
    int i = 0;
    for (i = 0; i < size; i++) {
        free(classify[i]);
    }
    free(classify);
}

/* Algorithms are fun! */