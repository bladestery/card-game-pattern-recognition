//
//  card.c
//  Big Data
//
//  Created by Ben Ruktantichoke on 3/21/15.
//  Copyright (c) 2015 Ben Ruktantichoke. All rights reserved.
//

#include "card.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define NUM_BUCKETS 16384         //2^14 buckets, important number for compute_hash
#define NUM_PATTERN_BUCKETS 28561 //13^4 buckets (each char contains 8 bits to represent all suites of two ranks)
                                  //Important number for calc_index
                                  //Considering pattern matching algorithm, first set bit will always be in index 0
#define PATTERN_STRICTNESS -1     //HIGHEST:4 (all suits must match) LOWEST:-1 (suit irrelevant)
#define DEV_STRICTNESS 0.01       //Variance in standard deviation allowed for assigning std_dev feature to rank

/* assign id to card */
void assign_id(struct card *card)
{
    if (card == NULL) {
        fprintf(stderr, "NULL card pointer passed to assign_id\n");
        return;
    }
    
    card->id = (card->suit) + (card->rank - 1) * 4;
}

/* Create entry with added hash*/
struct entry *create_entry(struct hand *hand, int id)
{
    struct entry *entry = NULL;
    
    if (hand == NULL) {
        fprintf(stderr, "NULL hand pointer passed to create_entry\n");
        return entry;
    }
    
    if ((entry = (struct entry *) calloc(1, sizeof(struct entry))) == NULL) {
        fprintf(stderr, "Memory allocation failed in create_entry\n");
        return entry;
    }
    
    entry->id = id;
    entry->hash = id % NUM_BUCKETS;
    entry->hand = hand;
    entry->count = 1;
    
    return entry;
}

/* compute hash of hand of 5 cards */
int compute_id(struct hand *hand)
{
    if (hand == NULL) {
        fprintf(stderr, "Null pointer passed to compute_id\n");
        return -1;
    }

    return (hand->card1->id * hand->card2->id *
            hand->card3->id * hand->card4->id *
            hand->card5->id);
}

/* create database for training data */
struct database *create_database(void)
{
    struct database *ret = NULL;
    
    if ((ret = (struct database *) calloc(1, sizeof(struct database))) == NULL) {
        fprintf(stderr, "Allocation of memory failed in create_database\n");
        return ret;
    }
    
    if ((ret->bucket = (struct entry **) calloc(NUM_BUCKETS, sizeof(struct entry *))) == NULL) {
        fprintf(stderr, "allocation of memory failed in create_database\n");
        free(ret);
    }
    
    return ret;
}

/* compares two hands, and returns 1 if both hands contain the same cards */
int compare_hand(struct hand *hand1, struct hand *hand2)
{
    if (hand1 == NULL || hand2 == NULL) {
        fprintf(stderr, "NUll pointer passed to compare_hand\n");
        return 0;
    }
    
    if ((hand1->card1->id == hand2->card1->id || hand1->card1->id == hand2->card2->id ||
         hand1->card1->id == hand2->card3->id || hand1->card1->id == hand2->card4->id ||
         hand1->card1->id == hand2->card5->id) &&
        (hand1->card2->id == hand2->card1->id || hand1->card2->id == hand2->card2->id ||
         hand1->card2->id == hand2->card3->id || hand1->card2->id == hand2->card4->id ||
         hand1->card2->id == hand2->card5->id) &&
        (hand1->card3->id == hand2->card1->id || hand1->card3->id == hand2->card2->id ||
         hand1->card3->id == hand2->card3->id || hand1->card3->id == hand2->card4->id ||
         hand1->card3->id == hand2->card5->id) &&
        (hand1->card4->id == hand2->card1->id || hand1->card4->id == hand2->card2->id ||
         hand1->card4->id == hand2->card3->id || hand1->card4->id == hand2->card4->id ||
         hand1->card4->id == hand2->card5->id) &&
        (hand1->card5->id == hand2->card1->id || hand1->card5->id == hand2->card2->id ||
         hand1->card5->id == hand2->card3->id || hand1->card5->id == hand2->card4->id ||
         hand1->card5->id == hand2->card5->id))
        return 1;
    
    return 0;
}

/* returns 1 if hand1 and hand2 are identical, otherwise 0 */
int exact_comparison(struct hand *hand1, struct hand *hand2, int *count)
{
    *count = 0;
    
    if (hand1 == NULL || hand2 == NULL) {
        fprintf(stderr, "Null pointer passed to exact_comparison\n");
        return 0;
    }
    
    if (hand1->card1->id == hand2->card1->id)
        (*count)++;
    if (hand1->card2->id == hand2->card2->id)
        (*count)++;
    if (hand1->card3->id == hand2->card3->id)
        (*count)++;
    if (hand1->card4->id == hand2->card4->id)
        (*count)++;
    if (hand1->card5->id == hand2->card5->id)
        (*count)++;
    
    return (*count == 5) ? 1 : 0;
}

/* basic insertion of new training data into database */
void insert_database(struct database *database, struct hand *hand, int *del)
{
    if (database == NULL || hand == NULL) {
        fprintf(stderr, "NUll pointer passed to insert_database\n");
        return;
    }

    int id = compute_id(hand);
    if (id == -1) {
        *del = 1;
        return;
    }
    int hash = id % NUM_BUCKETS;

    struct hand *hand_temp = NULL, *prev_hand = NULL;
    struct entry *entry = NULL;
    
    //Prior entry in hash table does not exist
    if (database->bucket[hash] == NULL) {
        entry = create_entry(hand, id);
        database->bucket[hash] = entry;
    }
    //Prior entry in hash table exists
    else {
        struct entry *temp = database->bucket[hash];
        //While there is still one more entry after current entry
        while (temp->next != NULL) {
            if (id > temp->id) //bucket[hash] is sorted in ascending order
                temp = temp->next;
            else if (id == temp->id) { //case: matching id
                if (compare_hand(temp->hand, hand)) { //identical cards in hand
                    int count;
                    hand_temp = temp->hand;
                    
                    //Look to see if there is identical hand
                    do {
                        if (exact_comparison(hand_temp, hand, &count)) { //identical order found
                            temp->hand->count++;
                            temp->count++;
                            *del = 1;
                            return;
                        }
                        
                        prev_hand = hand_temp;
                        hand_temp = hand_temp->next;
                    } while (hand_temp != NULL);

                    //Identical hand not found, append at end
                    prev_hand->next = hand;
                    temp->count++;
                    return;
                }
                else //Hand is different
                    temp = temp->next;
            }
            else { //current bucket hash is less than new entry
                entry = create_entry(hand, id);
                entry->prev = temp->prev;
                entry->next = temp;
                if (temp->prev != NULL)
                    temp->prev->next = entry;
                else
                    database->bucket[hash] = entry;
                temp->prev = entry;
                
                return;
            }
        }
 
        //Either last entry or there is only one entry in bucket[hash]
        if (id > temp->id) {
            entry = create_entry(hand, id);
            temp->next = entry;
            entry->prev = temp;
            
            if (entry->next != NULL)
                fprintf(stderr, "entry->next should be NULL!\n");
        }
        else if (id == temp->id && compare_hand(hand, temp->hand)) {
            int count;
            hand_temp = temp->hand;
            
            do {
                if (exact_comparison(hand_temp, hand, &count)) {
                    temp->hand->count++;
                    temp->count++;
                    *del = 1;
                    return;
                }
                
                prev_hand = hand_temp;
                hand_temp = hand_temp->next;
            } while (hand_temp != NULL);
            
            prev_hand->next = hand;
            temp->count++;
        }
        else {
            entry = create_entry(hand, id);
            entry->next = temp;
            if (temp->prev != NULL) {
                temp->prev->next = entry;
                entry->prev = temp->prev;
            }
            else
                database->bucket[hash] = entry;
            temp->prev = entry;
        }
    }
    
    return;
}

/* free all memory held by database */
void destroy_database(struct database *database)
{
    if (database == NULL) {
        fprintf(stderr, "Null pointer passed to destroy_datbase\n");
        return;
    }
    
    struct entry *bucket;
    for (int i = 0; i < NUM_BUCKETS; i++) {
         bucket = database->bucket[i];
        
        if (bucket != NULL) {
            struct entry *temp = NULL;
            do {
                temp = bucket->next;
                
                struct hand *hand = bucket->hand, *temp_hand = NULL;
                do {
                    temp_hand = hand->next;
                    free(hand->card1);
                    free(hand->card2);
                    free(hand->card3);
                    free(hand->card4);
                    free(hand->card5);
                    free(hand);
                    hand = temp_hand;
                } while (hand != NULL);

                free(bucket);
                bucket = temp;
            } while (bucket != NULL);
        }
    }
    
    free(database->bucket);
    free(database);
    return;
}

//Testing required
/* queries the database for hand
 succesful if perfect match or match and order doesn't matter
 returns rank of hand, otherwise sets res = 0 */
int query(struct database *database, struct hand *hand, int *res)
{
    if (database == NULL || hand == NULL) {
        fprintf(stderr, "Null pointer passed to query\n");
        *res = 0;
        return 0;
    }
    
    int id = compute_id(hand);
    if (id == -1) {
        *res = 0;
        return 0;
    }
    int hash = id % NUM_BUCKETS;
    
    struct entry *entry = NULL;
    if ((entry = database->bucket[hash]) == NULL) {
        *res = 0;
        return 0;
    }
    
    do {
        if (compare_hand(hand, entry->hand)) {
            if (entry->count > 1) { /* determine if order matters for hands with the same card */
                int order = 0, p_count = 0, n_count;
                struct hand *ret = entry->hand;
                
                for (struct hand *temp = entry->hand, *prev = NULL ;temp != NULL; temp = temp->next) {
                    if (exact_comparison(hand, temp, &n_count)) /* perfect match */
                        return temp->rank;
                    
                    if (n_count > p_count) {
                        p_count = n_count;
                        ret = temp;
                    }
                    
                    if (prev != NULL) {
                        if (prev->rank != temp->rank)
                            order++;
                    }
                    prev = temp;
                }
                
                return (!order) ?
                entry->hand->rank /* order doesn't seem to matter */ :
                ret->rank /* closest match */;
            }
            else /* no other hands to determine if order matters */
                return entry->hand->rank;
        }
        entry = entry->next;
    } while (entry != NULL);
    
    //Add further search here!
    
    *res = 0;
    return 0;
}

/* creates pattern structure from hand structure */
struct pattern *hand_to_pattern(struct hand *hand)
{
    if (hand == NULL) {
        fprintf(stderr, "NULL pointer passed to hand_to_pattern\n");
        return NULL;
    }
    
    struct item *item = NULL;
    if ((item = (struct item *) calloc(1, sizeof(struct item))) == NULL) {
        fprintf(stderr, "Memory allocation failed in hand_to_pattern\n");
        return NULL;
    }
    item->rank = hand->rank;
    item->count = 1;
    
    struct pattern *pattern = NULL;
    if ((pattern = (struct pattern *) calloc(1, sizeof(struct pattern))) == NULL) {
        fprintf(stderr, "Memory allocation failed in hand_to_pattern\n");
        free(item);
        return NULL;
    }
    pattern->item = item;
    pattern->order = 0;
   
    struct card *prev = NULL, *temp = NULL;
    for (int idx = 0; idx < 5; idx++) {
        int card_num = 0;
    
        if (prev == NULL) {
            if (hand->card1->id < hand->card2->id) {
                temp = hand->card1;
                card_num = 1;
            }
            else {
                temp = hand->card2;
                card_num = 2;
            }
            if (temp->id > hand->card3->id) {
                temp = hand->card3;
                card_num = 3;
            }
            if (temp->id > hand->card4->id) {
                temp = hand->card4;
                card_num = 4;
            }
            if (temp->id > hand->card5->id) {
                temp = hand->card5;
                card_num = 5;
            }
        }
        else {
            if (hand->card1->id > prev->id) {
                temp = hand->card1;
                card_num = 1;
            }
            else if (hand->card2->id > prev->id) {
                temp = hand->card2;
                card_num = 2;
            }
            else if (hand->card3->id > prev->id) {
                temp = hand->card3;
                card_num = 3;
            }
            else if (hand->card4->id > prev->id) {
                temp = hand->card4;
                card_num = 4;
            }
            else if (hand->card5->id > prev->id) {
                temp = hand->card5;
                card_num = 5;
            }
            
            if (temp->id > hand->card1->id && hand->card1->id > prev->id) {
                temp = hand->card1;
                card_num = 1;
            }
            if (temp->id > hand->card2->id && hand->card2->id > prev->id) {
                temp = hand->card2;
                card_num = 2;
            }
            if (temp->id > hand->card3->id && hand->card3->id > prev->id) {
                temp = hand->card3;
                card_num = 3;
            }
            if (temp->id > hand->card4->id && hand->card4->id > prev->id) {
                temp = hand->card4;
                card_num = 4;
            }
            if (temp->id > hand->card5->id && hand->card5->id > prev->id) {
                temp = hand->card5;
                card_num = 5;
            }
        }
        
        if (card_num == 0)
            fprintf(stderr, "something went wrong dewd\n");
        
        int res = (temp->id - 1) / 8;
        int loc = (temp->id - 1) % 8;
        uint8_t ins = 0x80 >> (loc);
        pattern->bitmap[res] |= ins;
        item->order[idx] = card_num;
        prev = temp;
    }
    
    int i;
    for (i = 0; i < 6 && pattern->bitmap[i] == 0; i++);

    if ((pattern->bitmap[i] & 0xF0) == 0) {
        int j = i;
        for (int next_i = i + 1; j < 6; j++, next_i++) {
            pattern->bitmap[j] <<= 4;
            uint8_t new = ((uint8_t) (pattern->bitmap[next_i] & 0xF0)) >> 4;
            pattern->bitmap[j] |= new;
        }
        pattern->bitmap[j] = 0;
    }
    
    if (i != 0) {
        memmove(pattern->bitmap, pattern->bitmap + i /* i should be in bytes */,
                sizeof(pattern->bitmap) - i);

        memset(pattern->bitmap + (sizeof(pattern->bitmap) - i), 0, i);
    }
    
    return pattern;
}

/* Creates databank strcuture to hold all patterns */
struct databank *create_databank(void)
{
    struct databank *ret = NULL;
    
    if ((ret = (struct databank *)  calloc(1, sizeof(struct databank))) == NULL)
        fprintf(stderr, "memory allocation failed in create_databank\n");
    
    if ((ret->cannister = (struct pattern **) calloc(NUM_PATTERN_BUCKETS, sizeof(struct pattern *))) == NULL) {
        fprintf(stderr, "Memory allocation failed in create_databank\n");
        free(ret);
    }
    
    return ret;
}

/* Calculates the hash index into the databank for a pattern structure */
/* pattern->bitmap[0] oooo (j=0) oooo (j=1) */
/* pattern->bitmap[1] oooo (j=2) oooo (j=3) */
/*               .                          */
/*               .                          */
/* pattern->bitmap[6] oooo (j=12)           */
int calc_index(struct pattern *pattern)
{
    if (pattern == NULL) {
        fprintf(stderr, "NULL pointer passed to calc_index\n");
        return -1;
    }
    
    int idx[5];
    
    for (int i = 0, j = 0; i < 5 && j < 7; j++) {
        uint8_t temp = pattern->bitmap[j] & 0xF0;
        uint8_t temp2 = pattern->bitmap[j] & 0x0F;
        
        while (temp) {
            idx[i++] = j*2; /* sets index according to the magnitude of the card */
            temp &= temp - 1;
        }
        
        while (temp2 && j<6) {
            idx[i++] = j*2 + 1;
            temp2 &= temp2 - 1;
        }
    }
    
    if (idx[0] != 0)
        fprintf(stderr, "Something went wrong in calc_index, pattern not in correct format\n");
    
    return idx[1] * 2197 + idx[2] * 169 + idx[3] * 13 + idx[4];
}

/* Inserts pattern in databank */
void insert_databank(struct databank *databank, struct pattern *pattern)
{
    if (databank == NULL || pattern == NULL) {
        fprintf(stderr, "NULL pointer passed to insert_databank\n");
        return;
    }
    
    int idx;
    if ((idx = calc_index(pattern)) == -1) {
        fprintf(stderr, "An error occured in insert_databank(calc_index)\n");
        return;
    }
    
    if (databank->cannister[idx] == NULL)
        databank->cannister[idx] = pattern;
    else {
        struct pattern *temp = NULL;
        do {
            if (temp == NULL)
                temp = databank->cannister[idx];
            else
                temp = temp->next;
            
            if (memcmp(pattern->bitmap, temp->bitmap, sizeof(pattern->bitmap)) == 0) {
                struct item *item = NULL;
                do {
                    if (item == NULL)
                        item = temp->item;
                    else
                        item = item->next;
                    
                    if (memcmp(item->order, pattern->item->order, sizeof(item->order)) == 0) {
                        if (item->rank != pattern->item->rank)
                            fprintf(stderr, "same pattern but differen rank\n"); //possible + rotation?
                        
                        item->count++;
                        
                        free(pattern->item);
                        free(pattern);
                        return;
                    }
                    
                    if (item->rank != pattern->item->rank)
                        databank->cannister[idx]->order = 1;
                } while (item->next != NULL);

                item->next = pattern->item;
                free(pattern);

                return;
            }
            
        } while (temp->next != NULL);
        
        temp->next = pattern;
    }
    
    return;
}

/* Frees all memory held by databank */
void destroy_databank(struct databank *databank)
{
    if (databank == NULL) {
        fprintf(stderr, "NULL pointer passed to destroy_databank\n");
        return;
    }

    for (int idx = 0; idx < NUM_PATTERN_BUCKETS; idx++) {
        struct pattern *pattern = NULL;
        if ((pattern = databank->cannister[idx])== NULL)
            continue;
        
        for (struct pattern *temp = pattern->next; pattern != NULL; pattern = temp) {
            temp = pattern->next;
            
            struct item *item = pattern->item;
            for (struct item *next = item->next; item != NULL; item = next) {
                next = item->next;
                free(item);
            }
            
            free(pattern);
        }
    }
    
    free(databank->cannister);
    free(databank);
    return;
}

int compare_order(struct item *item1, struct item *item2, int *num)
{
    *num = 0;
    
    if (item1 == NULL || item2 == NULL) {
        fprintf(stderr, "Null pointer passed to compare_order\n");
        return 0;
    }
    
    if (item1->order[0] == item2->order[0])
        (*num)++;
    if (item1->order[1] == item2->order[1])
        (*num)++;
    if (item1->order[2] == item2->order[2])
        (*num)++;
    if (item1->order[3] == item2->order[3])
        (*num)++;
    if (item1->order[4] == item2->order[4])
        (*num)++;
    
    return (*num == 5) ? 1 : 0;
    
}

int compare_suit(struct pattern *pattern1, struct pattern *pattern2)
{
    if (pattern1 == NULL || pattern2 == NULL) {
        fprintf(stderr, "NULL pointer passed to compare_suit\n");
        return -1;
    }
    
    uint8_t temp[7];
    int ret = 0;
    
    for (int i = 0; i < 7; i++) {
        temp[i] = pattern1->bitmap[i] & pattern2->bitmap[i];
        while (temp[i]) {
            ret++;
            temp[i] &= temp[i] - 1;
        }
    }
    
    return ret;
}

//Searches for pattern in databank
//returns valid rank if res is not set to 0
//otherwise sets res to 0 and returns 0 rank
int search(struct databank *databank, struct pattern *pattern, int *res)
{
    if (databank == NULL || pattern == NULL) {
        fprintf(stderr, "NULL pointer passed to search\n");
        *res = 0;
        return 0;
    }
    
    int idx;
    if ((idx = calc_index(pattern)) == -1) {
        fprintf(stderr, "An error occured in search(calc_index)\n");
        *res = 0;
        return 0;
    }
    
    struct pattern *entry = NULL;
    if ((entry = databank->cannister[idx]) != NULL) {
        int suit_match = 0, suit_count;
        struct pattern *suit_ret = NULL;
        
        do {
            if (memcmp(pattern->bitmap, entry->bitmap, sizeof(pattern->bitmap)) == 0) {
                if (entry->order) {
                    int count, p_count = 0;
                    struct item *ret = entry->item;
                    
                    for (struct item *item = entry->item; item != NULL; item = item->next) {
                        if (compare_order(item, pattern->item, &count))
                            return item->rank;
                        
                        if (count > p_count) {
                            count = p_count;
                            ret = item;
                        }
                    }
                    
                    return ret->rank;
                }
                else
                    return entry->item->rank;
            }
            
            if (suit_ret == NULL) {
                suit_ret = entry;
                suit_match = compare_suit(pattern, suit_ret);
            }
            else {
                suit_count = compare_suit(pattern, suit_ret);
                if (suit_count > suit_match) {
                    suit_match = suit_count;
                    suit_ret = entry;
                }
            }

            entry = entry->next;
        } while (entry != NULL);
        
        if (suit_match > PATTERN_STRICTNESS) {
            if (suit_ret->order) {
                int count, p_count = 0;
                struct item *ret = suit_ret->item;
                
                for (struct item *item = suit_ret->item; item != NULL; item = item->next) {
                    if (compare_order(item, pattern->item, &count))
                        return item->rank;
                    
                    if (count > p_count) {
                        count = p_count;
                        ret = item;
                    }
                }
                
                return ret->rank;
            }
            else
                return suit_ret->item->rank;
        }

    }
    
    *res = 0;
    return 0;
}

//Creates ranking structure
struct rankings *create_rankings(void)
{
    struct rankings *ret = NULL;
    
    if ((ret = (struct rankings *)  calloc(1, sizeof(struct rankings))) == NULL)
        fprintf(stderr, "memory allocation failed in create_rankings\n");
    
    for (int i = 0; i < NUM_RANKS; i++) {
        if ((ret->board[i] = (struct rank_info *) calloc(NUM_RANKS, sizeof(struct rank_info))) == NULL) {
            fprintf(stderr, "Memory allocation failed in create_rankings\n");
            
            while (i > 0)
                free(ret->board[--i]);
            
            free(ret);
            return ret;
        }
        ret->board[i]->rank = i;
    }

    ret->highest_count = ret->board[0];
    
    return ret;
}

//Free all memory held by rankings
void destroy_rankings(struct rankings *rankings)
{
    if (rankings == NULL) {
        fprintf(stderr, "NULL pointer passed to destroy_rankings\n");
        return;
    }
    
    for (int i = 0; i < NUM_RANKS ; i++) {
        struct hand_info *temp = rankings->board[i]->first;
        while (temp != NULL) {
            struct hand_info *next = temp->next;
            free(temp);
            temp = next;
        }
        
        free(rankings->board[i]);
    }
    
    free(rankings);
    return;
}

int multiplicity(struct hand *hand)
{
    if (hand == NULL) {
        fprintf(stderr, "NULL pointer passed to multiplicity");
        return -1;
    }
    
    int ret = 0;
    
    
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
        
        for (int j = 0; j < 5; j++) {
            if (i == j)
                continue;
            
            struct card *temp = NULL;
            if (j == 0)
                temp = hand->card1;
            else if (j == 1)
                temp = hand->card2;
            else if (j == 2)
                temp = hand->card3;
            else if (j == 3)
                temp = hand->card4;
            else if (j == 4)
                temp = hand->card5;
            
            if (card->rank == temp->rank)
                ret++;
        }
    }

    return ret;
}

int calc_suit(struct hand *hand)
{
    if (hand == NULL) {
        fprintf(stderr, "NULL pointer passed to multiplicity");
        return -1;
    }
    
    int ret = 0;
    
    
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
        
        for (int j = 0; j < 5; j++) {
            if (i == j)
                continue;
            
            struct card *temp = NULL;
            if (j == 0)
                temp = hand->card1;
            else if (j == 1)
                temp = hand->card2;
            else if (j == 2)
                temp = hand->card3;
            else if (j == 3)
                temp = hand->card4;
            else if (j == 4)
                temp = hand->card5;
            
            if (card->suit == temp->suit)
                ret++;
        }
    }
    
    return ret;
}

float calc_stddev(struct hand *hand)
{
    if (hand == NULL) {
        fprintf(stderr, "NULL pointer passed to multiplicity");
        return -1;
    }

    
    float mean = ((float) (hand->card1->id +
                           hand->card2->id +
                           hand->card3->id +
                           hand->card4->id +
                           hand->card5->id)) / 5.0;
    
    return sqrtf((powf((float) hand->card1->id - mean, 2) +
                  powf((float) hand->card2->id - mean, 2) +
                  powf((float) hand->card3->id - mean, 2) +
                  powf((float) hand->card4->id - mean, 2) +
                  powf((float) hand->card5->id - mean, 2)) / 5.0);
}

//Add descriptive statistic of hand as a datapoint in rankings
void update_rankings(struct rankings *rankings, struct hand *hand)
{
    if (rankings == NULL || hand == NULL) {
        fprintf(stderr, "NULL pointer passed to update_rankings\n");
        return;
    }
    
    struct hand_info *hand_info = NULL;
    if ((hand_info = (struct hand_info *) calloc(1, sizeof(struct hand_info))) == NULL) {
        fprintf(stderr, "Failed to allocate memory in update_rankings");
        return;
    }
    
    int rank = hand->rank;
    rankings->board[rank]->count++;
    if (rankings->board[rank]->count > rankings->highest_count->count)
        rankings->highest_count = rankings->board[rank];
    
    hand_info->mult = multiplicity(hand);
    hand_info->suit_mult = calc_suit(hand);
    hand_info->std_dev = calc_stddev(hand);
    
    if (hand_info->mult == -1 || hand_info->suit_mult == -1 || hand_info->std_dev == -1) {
        free(hand_info);
        return;
    }
    
    if (rankings->board[rank]->first == NULL) {
        rankings->board[rank]->first = hand_info;
        rankings->board[rank]->last = hand_info;
    }
    else {
        rankings->board[rank]->last->next = hand_info;
        rankings->board[rank]->last = hand_info;
    }
    
    return;
}

//Compute descriptive statistics
void finalize_rankings(struct rankings *rankings)
{
    if (rankings == NULL) {
        fprintf(stderr, "NULL pointer passed to finalize_rankings\n");
        return;
    }
    
    for (int i = 0; i < NUM_RANKS; i++) {
        int sum_mult = 0, sum_suit = 0;
        float sum_dev = 0.0;
        
        struct rank_info *rank_info = rankings->board[i];
        for (struct hand_info *hand_info = rankings->board[i]->first;
             hand_info != NULL;
             hand_info = hand_info->next) {
            sum_mult += hand_info->mult;
            sum_suit += hand_info->suit_mult;
            sum_dev  += hand_info->std_dev;
        }
        
        float mean_mult = (float) sum_mult / rank_info->count;
        float mean_suit = (float) sum_suit / rank_info->count;
        float mean_dev  = (float) sum_dev  / rank_info->count;
        
        //fprintf(stderr, "rankings->board[%d]: mean_mult:%f\tmean_suit:%f\tmean_dev:%f\n", i, mean_mult, mean_suit, mean_dev);
        
        float res_mult = 0.0, res_suit = 0.0, res_dev = 0.0;
        for (struct hand_info *hand_info = rankings->board[i]->first;
             hand_info != NULL;
             hand_info = hand_info->next) {
            res_mult += powf((float) hand_info->mult - mean_mult, 2);
            res_suit += powf((float) hand_info->suit_mult - mean_suit, 2);
            res_dev  += powf((float) hand_info->std_dev - mean_dev, 2);
        }
        
        rank_info->dev_mult = sqrtf(res_mult / (float) rank_info->count);
        rank_info->dev_suit = sqrtf(res_suit / (float) rank_info->count);
        rank_info->dev_dev  = sqrtf(res_dev  / (float) rank_info->count);
        
        //fprintf(stderr, "rankings->board[%d]: dev_mult:%f\tdev_suit:%f\tdev_dev:%f\n", i, rank_info->dev_mult, rank_info->dev_suit, rank_info->dev_dev);
        
        //Needs optimization
        if (rank_info->dev_mult == 0.0) {
            rank_info->mult = (int) mean_mult;
            rank_info->valid_mult = 1;
        }
        
        if (rank_info->dev_suit == 0.0){
            rank_info->suit_mult = (int) mean_suit;
            rank_info->valid_suit = 1;
        }
        
        if (rank_info->dev_dev < DEV_STRICTNESS) {
            rank_info->std_dev = mean_dev;
            rank_info->valid_dev = 1;
        }
    }
    
    rankings->finalized = 1;
}

//Needs optimization
//Searches for descriptive satistical matches in rankings with hand structure
int find(struct rankings *rankings, struct hand *hand, int *res)
{
    if (rankings == NULL || hand == NULL) {
        fprintf(stderr, "NULL pointer passed to find\n");
        *res = 0;
        return 0;
    }
    
    int mult = multiplicity(hand);
    int suit = calc_suit(hand);
    float dev= calc_stddev(hand);
    
    for (int i = 0; i < NUM_RANKS; i++) {
        struct rank_info *rank_info = rankings->board[i];
        if (rank_info->valid_dev && rank_info->valid_suit && rank_info->valid_mult &&
            (rank_info->std_dev - dev < 3 * rank_info->dev_dev && rank_info->std_dev - dev > 3 * rank_info->dev_dev) &&
            suit == rank_info->suit_mult &&
            mult == rank_info->mult)
            return rank_info->rank;
    }
    for (int i = 0; i < NUM_RANKS; i++) {
        struct rank_info *rank_info = rankings->board[i];
        if (rank_info->valid_dev && rank_info->valid_suit && !(rank_info->valid_mult) &&
            (rank_info->std_dev - dev < 3 * rank_info->dev_dev && rank_info->std_dev - dev > 3 * rank_info->dev_dev) &&
            suit == rank_info->suit_mult)
            return rank_info->rank;
    }
    for (int i = 0; i < NUM_RANKS; i++) {
        struct rank_info *rank_info = rankings->board[i];
        if (rank_info->valid_dev && !(rank_info->valid_suit) && rank_info->valid_mult &&
            (rank_info->std_dev - dev < 3 * rank_info->dev_dev && rank_info->std_dev - dev > 3 * rank_info->dev_dev) &&
            mult == rank_info->mult)
            return rank_info->rank;
    }
    for (int i = 0; i < NUM_RANKS; i++) {
        struct rank_info *rank_info = rankings->board[i];
        if (rank_info->valid_dev && !(rank_info->valid_suit) && !(rank_info->valid_mult) &&
            (rank_info->std_dev - dev < 3 * rank_info->dev_dev && rank_info->std_dev - dev > 3 * rank_info->dev_dev))
            return rank_info->rank;
    }
    for (int i = 0; i < NUM_RANKS; i++) {
        struct rank_info *rank_info = rankings->board[i];
        if (!(rank_info->valid_dev) && rank_info->valid_suit && rank_info->valid_mult &&
            suit == rank_info->suit_mult &&
            mult == rank_info->mult)
            return rank_info->rank;
    }
    for (int i = 0; i < NUM_RANKS; i++) {
        struct rank_info *rank_info = rankings->board[i];
        if (!(rank_info->valid_dev) && rank_info->valid_suit && !(rank_info->valid_mult) &&
            suit == rank_info->suit_mult)
            return rank_info->rank;
    }
    for (int i = 0; i < NUM_RANKS; i++) {
        struct rank_info *rank_info = rankings->board[i];
        if (!(rank_info->valid_dev) && !(rank_info->valid_suit) && rank_info->valid_mult &&
            mult == rank_info->mult)
            return rank_info->rank;
    }
    
    *res = 0;
    return 0;
}




