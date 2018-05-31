//
//  main.c
//  Big Data
//
//  Created by Ben Ruktantichoke on 3/21/15.
//  Copyright (c) 2015 Ben Ruktantichoke. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include "card.h"

#define BUFF 64

int main(int argc, const char * argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: ./big_data <train.csv> <test.csv> <output.csv>\n");
        return -1;
    }
    
    /* create data structures */
    struct database *database = NULL;
    if ((database = create_database()) == NULL)
        return -1;
    
    struct databank *databank = NULL;
    if ((databank = create_databank()) == NULL) {
        free(database->bucket);
        free(database);
        return -1;
    }
    
    struct rankings *rankings = NULL;
    if ((rankings = create_rankings()) == NULL) {
        free(database->bucket);
        free(database);
        free(databank->cannister);
        free(databank);
        return -1;
    }

    FILE *file = NULL;
    if ((file = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "Error with fopen\n");
        free(database->bucket);
        free(database);
        free(databank->cannister);
        free(databank);
        free(rankings->board);
        free(rankings);
        return -1;
    }
    
    fscanf(file, "%*[^\n]\n");
    
    char input[BUFF];
    while (fgets(input, BUFF, file)) {
        /* create structure to hold a hand */
        struct hand *hand = NULL;
        if ((hand = (struct hand *) calloc(1, sizeof(struct hand))) == NULL) {
            fprintf(stderr, "hand calloc failed!\n");
            continue;
        }
        hand->count = 1;
        
        /* create structure to hold cards */
        struct card *card1 = NULL, *card2 = NULL, *card3 = NULL, *card4 = NULL, *card5 = NULL;
        if ((card1 = (struct card *) calloc(1, sizeof(struct card))) == NULL) {
            fprintf(stderr, "card1 calloc failed\n");
            free(hand);
            continue;
        }
        if ((card2 = (struct card *) calloc(1, sizeof(struct card))) == NULL) {
            fprintf(stderr, "card2 calloc failed\n");
            free(hand);
            free(card1);
            continue;
        }
        if ((card3 = (struct card *) calloc(1, sizeof(struct card))) == NULL) {
            fprintf(stderr, "card3 calloc failed\n");
            free(hand);
            free(card1);
            free(card2);
            continue;
        }
        if ((card4 = (struct card *) calloc(1, sizeof(struct card))) == NULL) {
            fprintf(stderr, "card4 calloc failed\n");
            free(hand);
            free(card1);
            free(card2);
            free(card3);
            continue;
        }
        if ((card5 = (struct card *) calloc(1, sizeof(struct card))) == NULL) {
            fprintf(stderr, "card5 calloc failed\n");
            free(hand);
            free(card1);
            free(card2);
            free(card3);
            free(card4);
            continue;
        }
        hand->card1 = card1;
        hand->card2 = card2;
        hand->card3 = card3;
        hand->card4 = card4;
        hand->card5 = card5;
        
        /* assign card suit and rank */
        char *token = strtok(input, ",");
        for (int i = 0; i < 5; i++) {
            struct card *card = NULL;
            if (i == 0)
                card = hand->card1;
            else if (i == 1)
                card = hand->card2;
            else if (i == 2)
                card = hand->card3;
            else if (i == 3)
                card = hand->card4;
            else if (i == 4)
                card = hand->card5;
            
            card->suit = atoi(token);
            token = strtok(NULL, ",");
            card->rank = atoi(token);
            assign_id(card);
            token = strtok(NULL, ",");
        }
        hand->rank = atoi(token);
        
        /* Machine learning structure: Descriptive Statistics */
        update_rankings(rankings, hand);
        
        /* Machine learning structure: Identical Hand Match database considering Order */
        int del = 0;
        insert_database(database, hand, &del);
        
        /* Method to change hand into data structure for Pattern Machine Learning */
        struct pattern *pattern = NULL;
        if ((pattern = hand_to_pattern(hand)) == NULL) {
            fprintf(stderr, "creation of pattern structure from hand structure failed\n");
            if (del) {
                free(hand->card1);
                free(hand->card2);
                free(hand->card3);
                free(hand->card4);
                free(hand->card5);
                free(hand);
            }
            continue;
        }
        
        if (del) {
            free(hand->card1);
            free(hand->card2);
            free(hand->card3);
            free(hand->card4);
            free(hand->card5);
            free(hand);
        }
        
        /* Machine learning structure: Pattern databank*/
        insert_databank(databank, pattern);
    }
    
    /* Compute descriptive stastiscal values for each rank*/
    finalize_rankings(rankings);
    
    if (fclose(file) == EOF)
        fprintf(stderr, "Error with fclose\n");
    
    if ((file = fopen(argv[2], "r")) == NULL) {
        fprintf(stderr, "Error with fopen\n");
        destroy_database(database);
        destroy_databank(databank);
        return -1;
    }
    
    FILE *output = NULL;
    if ((output = fopen(argv[3], "w")) == NULL) {
        fprintf(stderr, "Error with fopen\n");
        destroy_database(database);
        destroy_databank(databank);
        return -1;
    }
    
    fscanf(file, "%*[^\n]\n");
    
    //fprintf(output, "id,hand\n");
    //int abcd = 1;
    
    /* Classification Algorithm */
    int match_count = 0, pattern_count = 0, rankings_count = 0;
    while (fgets(input, BUFF, file)) {
        /* create structure to hold hand and cards */
        struct hand *hand = NULL;
        if ((hand = (struct hand *) calloc(1, sizeof(struct hand))) == NULL) {
            fprintf(stderr, "hand calloc failed!\n");
            fprintf(output, "%d,", rankings->highest_count->rank);
            continue;
        }
        
        struct card *card1 = NULL, *card2 = NULL, *card3 = NULL, *card4 = NULL, *card5 = NULL;
        if ((card1 = (struct card *) calloc(1, sizeof(struct card))) == NULL) {
            fprintf(stderr, "card1 calloc failed\n");
            fprintf(output, "%d,", rankings->highest_count->rank);
            free(hand);
            continue;
        }
        if ((card2 = (struct card *) calloc(1, sizeof(struct card))) == NULL) {
            fprintf(stderr, "card2 calloc failed\n");
            fprintf(output, "%d,", rankings->highest_count->rank);
            free(hand);
            free(card1);
            continue;
        }
        if ((card3 = (struct card *) calloc(1, sizeof(struct card))) == NULL) {
            fprintf(stderr, "card3 calloc failed\n");
            fprintf(output, "%d,", rankings->highest_count->rank);
            free(hand);
            free(card1);
            free(card2);
            continue;
        }
        if ((card4 = (struct card *) calloc(1, sizeof(struct card))) == NULL) {
            fprintf(stderr, "card4 calloc failed\n");
            fprintf(output, "%d,", rankings->highest_count->rank);
            free(hand);
            free(card1);
            free(card2);
            free(card3);
            continue;
        }
        if ((card5 = (struct card *) calloc(1, sizeof(struct card))) == NULL) {
            fprintf(stderr, "card5 calloc failed\n");
            fprintf(output, "%d,", rankings->highest_count->rank);
            free(hand);
            free(card1);
            free(card2);
            free(card3);
            free(card4);
            continue;
        }
        hand->card1 = card1;
        hand->card2 = card2;
        hand->card3 = card3;
        hand->card4 = card4;
        hand->card5 = card5;
        
        /* assign card suit and rank */
        char *token = strtok(input, ",");
        for (int i = 0; i < 5; i++) {
            struct card *card = NULL;
            if (i == 0)
                card = hand->card1;
            else if (i == 1)
                card = hand->card2;
            else if (i == 2)
                card = hand->card3;
            else if (i == 3)
                card = hand->card4;
            else if (i == 4)
                card = hand->card5;
            
            token = strtok(NULL, ",");
            card->suit = atoi(token);
            token = strtok(NULL, ",");
            card->rank = atoi(token);
            assign_id(card);
        }

        //Machine learning: search in database for identical hand
        //                  handles order properly
        int res = 1;
        int ret = query(database, hand, &res);
        if (res) {
            fprintf(output, "%d,", /*abcd++,*/ ret);
            free(hand->card1);
            free(hand->card2);
            free(hand->card3);
            free(hand->card4);
            free(hand->card5);
            free(hand);
            match_count++;
            continue;
        }

        //Convert hand structure to pattern structure
        struct pattern *pattern = NULL;
        if ((pattern = hand_to_pattern(hand)) == NULL) {
            fprintf(stderr, "creation of pattern structure from hand structure failed\n");
            free(hand->card1);
            free(hand->card2);
            free(hand->card3);
            free(hand->card4);
            free(hand->card5);
            free(hand);
            fprintf(output, "%d,", rankings->highest_count->rank);
            continue;
        }
        
        //Machine learning: search in databank for pattern
        res = 1;
        ret = search(databank, pattern, &res);
        if (res) {
            fprintf(output, "%d,", /*abcd++,*/ ret);
            pattern_count++;
            free(hand->card1);
            free(hand->card2);
            free(hand->card3);
            free(hand->card4);
            free(hand->card5);
            free(hand);
            continue;
        }
        
        //Machine learning: search in rankings for descriptive statistical match */
        res = 1;
        ret = find(rankings, hand, &res);
        if (res) {
            fprintf(output, "%d,", /*abcd++,*/ ret);
            rankings_count++;
        }
        else //Machine learning: otherwise print highest occuring rank
            fprintf(output, "%d,", /*abcd++,*/ rankings->highest_count->rank);
        
        free(hand->card1);
        free(hand->card2);
        free(hand->card3);
        free(hand->card4);
        free(hand->card5);
        free(hand);
    }
    //fprintf(stderr, "match_count: %d\tpattern_count: %d\trankings_count: %d\n", match_count, pattern_count, rankings_count);
    
    if (fclose(file) == EOF)
        fprintf(stderr, "Error with fclose\n");
    
    if (fclose(output) == EOF)
        fprintf(stderr, "Error with fclose\n");
    
    //Free all memory
    destroy_database(database);
    destroy_databank(databank);
    destroy_rankings(rankings);
    
    return 0;
}
